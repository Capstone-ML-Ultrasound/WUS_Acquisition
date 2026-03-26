import json
import time
import threading
from collections import deque
from pathlib import Path

import joblib
import matplotlib.pyplot as plt
import numpy as np


class OnlineDepthCalibrator:
    def __init__(self, depth, eps=1e-6, freeze_after=None):
        self.depth = int(depth)
        self.eps = float(eps)
        self.freeze_after = None if freeze_after is None else int(freeze_after)

        self.n = 0
        self.mean = np.zeros(self.depth, dtype=np.float32)
        self.M2 = np.zeros(self.depth, dtype=np.float32)

    def update(self, frame: np.ndarray):
        if self.freeze_after is not None and self.n >= self.freeze_after:
            return

        x = np.asarray(frame, dtype=np.float32)
        if x.shape != (self.depth,):
            raise ValueError(f"Expected frame shape ({self.depth},), got {x.shape}")

        self.n += 1
        delta = x - self.mean
        self.mean += delta / self.n
        delta2 = x - self.mean
        self.M2 += delta * delta2

    def ready(self, min_frames=30):
        return self.n >= int(min_frames)

    def get_mean_std(self):
        if self.n < 2:
            std = np.ones(self.depth, dtype=np.float32)
        else:
            var = self.M2 / max(self.n - 1, 1)
            std = np.sqrt(np.maximum(var, self.eps)).astype(np.float32)
        return self.mean.astype(np.float32), std


class OnlineFeaturePipeline:
    def __init__(
        self,
        depth_mean,
        depth_std,
        scaler,
        pca,
        crop_first=100,
        crop_last=100,
        unstable_remove=20,
        gauss_sigma=2.0,
        win=10,
        step=5,
        b=3.0,
        include_mean=True,
        include_var=True,
        include_energy_sigmoid=True,
        eps=1e-6,
    ):
        self.depth_mean = None if depth_mean is None else np.asarray(depth_mean, dtype=np.float32)
        self.depth_std = None if depth_std is None else np.asarray(depth_std, dtype=np.float32)
        self.scaler = scaler
        self.pca = pca

        self.crop_first = int(crop_first)
        self.crop_last = int(crop_last)
        self.unstable_remove = int(unstable_remove)
        self.win = int(win)
        self.step = int(step)
        self.b = float(b)
        self.include_mean = bool(include_mean)
        self.include_var = bool(include_var)
        self.include_energy_sigmoid = bool(include_energy_sigmoid)
        self.eps = float(eps)

        self.kernel = self._gaussian_kernel1d(gauss_sigma).astype(np.float32)
        self.pad = len(self.kernel) // 2

        if self.depth_mean is not None:
            d0 = self.depth_mean.shape[0]
        elif self.depth_std is not None:
            d0 = self.depth_std.shape[0]
        else:
            raise ValueError("Need depth_mean/depth_std or set them later after construction.")

        left = self.crop_first + self.unstable_remove
        right = d0 - self.crop_last - self.unstable_remove
        if right <= left:
            raise ValueError("Cropping removes all depth bins.")

        self.depth_slice = slice(left, right)
        self.D_final = right - left

        if self.D_final < self.win:
            raise ValueError("Final depth smaller than sliding window.")

        self.starts = np.arange(0, self.D_final - self.win + 1, self.step, dtype=np.int32)

        self.n_features_per_window = (
            int(self.include_mean)
            + int(self.include_var)
            + int(self.include_energy_sigmoid)
        )
        if self.n_features_per_window == 0:
            raise ValueError("No features enabled.")

        self.feature_dim = len(self.starts) * self.n_features_per_window

    def set_calibration(self, depth_mean, depth_std):
        self.depth_mean = np.asarray(depth_mean, dtype=np.float32)
        self.depth_std = np.asarray(depth_std, dtype=np.float32)

    @staticmethod
    def _gaussian_kernel1d(sigma, radius=None):
        sigma = float(sigma)
        if sigma <= 0:
            raise ValueError("sigma must be > 0")
        if radius is None:
            radius = int(np.ceil(3 * sigma))
        x = np.arange(-radius, radius + 1, dtype=np.float32)
        k = np.exp(-(x * x) / (2.0 * sigma * sigma)).astype(np.float32)
        k /= (k.sum() + 1e-12)
        return k

    @staticmethod
    def _sigmoid(x):
        return 1.0 / (1.0 + np.exp(-x))

    def _normalize_per_depth(self, frame):
        if self.depth_mean is None or self.depth_std is None:
            raise RuntimeError("Calibration not set yet.")
        return (frame - self.depth_mean) / (self.depth_std + self.eps)

    def _crop_and_trim(self, frame):
        return frame[self.depth_slice]

    def _gaussian_filter_depth(self, frame):
        xpad = np.pad(frame, (self.pad, self.pad), mode="reflect")
        return np.convolve(xpad, self.kernel, mode="valid").astype(np.float32)

    def _extract_features(self, frame):
        feats = np.empty(self.feature_dim, dtype=np.float32)
        j = 0

        for s in self.starts:
            w = frame[s:s + self.win]

            if self.include_mean or self.include_var:
                m = w.mean(dtype=np.float32)
            if self.include_var:
                v = ((w - m) ** 2).mean(dtype=np.float32)
            if self.include_energy_sigmoid:
                energy = np.sqrt((w * w).sum(dtype=np.float32), dtype=np.float32)
                es = self._sigmoid(energy - self.b)

            if self.include_mean:
                feats[j] = m
                j += 1
            if self.include_var:
                feats[j] = v
                j += 1
            if self.include_energy_sigmoid:
                feats[j] = es
                j += 1

        return feats

    def transform_frame_to_model_input(self, frame):
        x = np.asarray(frame, dtype=np.float32)
        x = self._normalize_per_depth(x)
        x = self._crop_and_trim(x)
        x = self._gaussian_filter_depth(x)
        feats = self._extract_features(x).reshape(1, -1)

        feats = self.scaler.transform(feats).astype(np.float32)
        if self.pca is not None:
            feats = self.pca.transform(feats).astype(np.float32)
        return feats

    def predict_frame(self, frame, model):
        x_model = self.transform_frame_to_model_input(frame)
        pred = model.predict(x_model)
        return float(pred[0])


class KafkaFrameBuffer:
    def __init__(self, maxlen=30000):
        self.buffer = deque(maxlen=maxlen)
        self.lock = threading.Lock()

    def push(self, frame: np.ndarray):
        with self.lock:
            self.buffer.append(frame)

    def pop(self):
        with self.lock:
            if not self.buffer:
                return None
            return self.buffer.popleft()

    def __len__(self):
        with self.lock:
            return len(self.buffer)


class CSVToKafkaProducer(threading.Thread):
    """
    Simulates a Kafka producer by reading a full-session CSV and pushing one frame
    (one column) at a time into the buffer.
    """
    def __init__(self, csv_path, frame_buffer, stop_event, sleep_s=0.0, log_every=500):
        super().__init__(daemon=True)
        self.csv_path = Path(csv_path)
        self.frame_buffer = frame_buffer
        self.stop_event = stop_event
        self.sleep_s = float(sleep_s)
        self.log_every = int(log_every)

    def run(self):
        data = np.loadtxt(self.csv_path, delimiter=",", dtype=np.float32)
        if data.ndim != 2:
            raise ValueError(f"Expected 2D CSV, got shape {data.shape}")

        depth, n_frames = data.shape
        print(f"CSV loaded | depth={depth} | n_frames={n_frames}")

        for i in range(n_frames):
            if self.stop_event.is_set():
                break

            frame = data[:, i].astype(np.float32, copy=False)
            self.frame_buffer.push(frame)

            if i % self.log_every == 0:
                print(f"producer pushed frame {i}/{n_frames} | buffer_len={len(self.frame_buffer)}")

            if self.sleep_s > 0:
                time.sleep(self.sleep_s)

        print("producer finished")


def load_artifacts(model_dir: str):
    model_dir = Path(model_dir)

    cfg = json.loads((model_dir / "best_config.json").read_text(encoding="utf-8"))
    model = joblib.load(model_dir / "best_model.joblib")
    scaler = joblib.load(model_dir / "best_scaler.joblib")

    pca_path = model_dir / "best_pca.joblib"
    pca = joblib.load(pca_path) if pca_path.exists() else None

    return cfg, model, scaler, pca


def build_pipeline(model_dir: str, raw_depth: int):
    cfg, model, scaler, pca = load_artifacts(model_dir)

    dummy_mean = np.zeros(raw_depth, dtype=np.float32)
    dummy_std = np.ones(raw_depth, dtype=np.float32)

    pipeline = OnlineFeaturePipeline(
        depth_mean=dummy_mean,
        depth_std=dummy_std,
        scaler=scaler,
        pca=pca,
        crop_first=cfg["crop_first"],
        crop_last=cfg["crop_last"],
        unstable_remove=cfg["unstable_remove"],
        gauss_sigma=cfg["gauss_sigma"],
        win=cfg["win"],
        step=cfg["step"],
        b=cfg["b"],
        include_mean=cfg["include_mean"],
        include_var=cfg["include_var"],
        include_energy_sigmoid=cfg.get("include_energy_sigmoid", True),
    )

    return pipeline, model, cfg


def run_session_inference(
    model_dir,
    csv_path,
    warmup_frames=50,
    freeze_after=200,
    producer_sleep_s=0.0,
    out_dir=None,
):
    csv_path = Path(csv_path)
    out_dir = Path(out_dir) if out_dir is not None else csv_path.parent / f"inference_{csv_path.stem}"
    out_dir.mkdir(parents=True, exist_ok=True)

    # sniff depth cheaply before loading artifacts into the pipeline
    # load once to get the true matrix shape:
    # rows = depth bins, cols = frames
    data_preview = np.loadtxt(csv_path, delimiter=",", dtype=np.float32)
    if data_preview.ndim != 2:
        raise ValueError(f"Expected 2D CSV, got shape {data_preview.shape}")

    raw_depth = data_preview.shape[0]

    pipeline, model, cfg = build_pipeline(model_dir=model_dir, raw_depth=raw_depth)
    calibrator = OnlineDepthCalibrator(depth=raw_depth, freeze_after=freeze_after)

    print("loaded config:")
    print(json.dumps(cfg, indent=2))
    print(f"raw depth: {raw_depth}")
    print(f"feature dim before scaler/pca: {pipeline.feature_dim}")
    print(f"warmup_frames: {warmup_frames}")
    print(f"freeze_after: {freeze_after}")

    frame_buffer = KafkaFrameBuffer(maxlen=30000)
    stop_event = threading.Event()

    producer = CSVToKafkaProducer(
        csv_path=csv_path,
        frame_buffer=frame_buffer,
        stop_event=stop_event,
        sleep_s=producer_sleep_s,
    )
    producer.start()

    predictions = []
    infer_times_ms = []
    calib_counts = []
    frame_indices = []

    frame_idx = 0
    idle_loops = 0

    try:
        while True:
            frame = frame_buffer.pop()
            if frame is None:
                if not producer.is_alive():
                    break
                idle_loops += 1
                if idle_loops % 1000 == 0:
                    print(f"waiting for frames... buffer_len={len(frame_buffer)}")
                time.sleep(0.001)
                continue

            idle_loops = 0
            calibrator.update(frame)
            calib_counts.append(calibrator.n)
            frame_indices.append(frame_idx)

            if not calibrator.ready(min_frames=warmup_frames):
                predictions.append(np.nan)
                infer_times_ms.append(np.nan)
                if frame_idx % 100 == 0:
                    print(f"frame {frame_idx:06d} | calibrating... n={calibrator.n}")
                frame_idx += 1
                continue

            depth_mean, depth_std = calibrator.get_mean_std()
            pipeline.set_calibration(depth_mean, depth_std)

            t0 = time.perf_counter()
            pred = pipeline.predict_frame(frame, model)
            t1 = time.perf_counter()

            dt_ms = (t1 - t0) * 1000.0
            predictions.append(pred)
            infer_times_ms.append(dt_ms)

            if frame_idx % 100 == 0:
                print(
                    f"frame {frame_idx:06d} | pred={pred:.6f} | "
                    f"infer_time_ms={dt_ms:.3f} | calib_n={calibrator.n} | buffer_len={len(frame_buffer)}"
                )
            frame_idx += 1

    finally:
        stop_event.set()
        producer.join(timeout=1.0)

    predictions = np.asarray(predictions, dtype=np.float32)
    infer_times_ms = np.asarray(infer_times_ms, dtype=np.float32)
    frame_indices = np.asarray(frame_indices, dtype=np.int32)
    calib_counts = np.asarray(calib_counts, dtype=np.int32)

    pred_csv = out_dir / "predictions.csv"
    with pred_csv.open("w", encoding="utf-8") as f:
        f.write("frame_idx,prediction,infer_time_ms,calib_n\n")
        for i, pred, dt, n in zip(frame_indices, predictions, infer_times_ms, calib_counts):
            pred_str = "" if np.isnan(pred) else f"{pred:.8f}"
            dt_str = "" if np.isnan(dt) else f"{dt:.6f}"
            f.write(f"{i},{pred_str},{dt_str},{n}\n")

    plt.figure(figsize=(12, 5))
    plt.plot(frame_indices, predictions)
    plt.axvline(warmup_frames - 1, linestyle="--")
    plt.xlabel("Frame index")
    plt.ylabel("Prediction")
    plt.title("Predictions across time")
    plt.tight_layout()
    pred_png = out_dir / "predictions_over_time.png"
    plt.savefig(pred_png, dpi=150)
    plt.close()

    valid = ~np.isnan(predictions)
    if np.any(valid):
        print(
            "done | "
            f"frames={len(predictions)} | "
            f"valid_preds={int(valid.sum())} | "
            f"mean_pred={float(np.nanmean(predictions)):.6f} | "
            f"mean_infer_ms={float(np.nanmean(infer_times_ms)):.4f}"
        )
    else:
        print("done | no valid predictions produced")

    print(f"saved predictions csv: {pred_csv}")
    print(f"saved prediction plot: {pred_png}")


if __name__ == "__main__":
    BASE_DIR = Path.cwd()
    model_dir = BASE_DIR / "models" / "boosted_tree_regression_sessionnorm"
    csv_path = BASE_DIR / "data" / "raw_us" / "session_006_full.csv"

    run_session_inference(
        model_dir=model_dir,
        csv_path=csv_path,
        warmup_frames=0,
        freeze_after=2000,
        producer_sleep_s=0.0,
        out_dir=None,
    )
