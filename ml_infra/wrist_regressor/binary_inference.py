import time
import threading
from collections import deque
from pathlib import Path

import joblib
import matplotlib.pyplot as plt
import numpy as np


class OnlineBinaryFeaturePipeline:
    def __init__(
        self,
        scaler,
        pca,
        crop_first=100,
        crop_last=100,
        unstable_remove=20,
        gauss_sigma=2.0,
        win=10,
        step=5,
        b_thresh=3.0,
    ):
        self.scaler = scaler
        self.pca = pca

        self.crop_first = int(crop_first)
        self.crop_last = int(crop_last)
        self.unstable_remove = int(unstable_remove)
        self.win = int(win)
        self.step = int(step)
        self.b_thresh = float(b_thresh)

        self.kernel = self._gaussian_kernel1d(gauss_sigma).astype(np.float32)
        self.pad = len(self.kernel) // 2

        self.starts = None
        self.feature_dim = None

    def init_for_depth(self, raw_depth: int):
        left = self.crop_first + self.unstable_remove
        right = raw_depth - self.crop_last - self.unstable_remove
        if right <= left:
            raise ValueError("Cropping removes all depth bins.")

        D_final = right - left
        if D_final < self.win:
            raise ValueError("Final depth smaller than sliding window.")

        self.depth_slice = slice(left, right)
        self.starts = np.arange(0, D_final - self.win + 1, self.step, dtype=np.int32)
        self.feature_dim = len(self.starts) * 3

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
            m = w.mean(dtype=np.float32)
            v = ((w - m) ** 2).mean(dtype=np.float32)
            energy = np.sqrt((w * w).sum(dtype=np.float32), dtype=np.float32)
            es = self._sigmoid(energy - self.b_thresh)

            feats[j] = m
            feats[j + 1] = v
            feats[j + 2] = es
            j += 3

        return feats

    def transform_frame_to_model_input(self, frame):
        x = np.asarray(frame, dtype=np.float32)
        x = self._crop_and_trim(x)
        x = self._gaussian_filter_depth(x)
        feats = self._extract_features(x).reshape(1, -1)

        feats = self.scaler.transform(feats).astype(np.float32)
        if self.pca is not None:
            feats = self.pca.transform(feats).astype(np.float32)

        return feats

    def predict_frame(self, frame, model, threshold=0.5):
        x_model = self.transform_frame_to_model_input(frame)
        p_open = float(model.predict_proba(x_model)[0, 1])
        pred_class = int(p_open >= threshold)
        return pred_class, p_open


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


def load_artifacts(model_path: str):
    bundle = joblib.load(model_path)
    model = bundle["model"]
    scaler = bundle["scaler"]
    pca = bundle["pca"]
    cfg = bundle["preprocess_config"]
    class_mapping = bundle.get("class_mapping", {0: "closed", 1: "open"})
    return cfg, model, scaler, pca, class_mapping


def gaussian_kernel1d(sigma, radius=None):
    sigma = float(sigma)
    if sigma <= 0:
        raise ValueError("sigma must be > 0")
    if radius is None:
        radius = int(np.ceil(3 * sigma))
    x = np.arange(-radius, radius + 1, dtype=np.float32)
    kernel = np.exp(-(x * x) / (2.0 * sigma * sigma)).astype(np.float32)
    kernel /= kernel.sum() + 1e-12
    return kernel


def gaussian_filter_1d(y, sigma):
    y = np.asarray(y, dtype=np.float32)
    if sigma <= 0:
        return y.copy()

    kernel = gaussian_kernel1d(sigma)
    pad = len(kernel) // 2
    valid = np.isfinite(y)
    if not np.any(valid):
        return y.copy()

    filled = np.where(valid, y, 0.0)
    weights = valid.astype(np.float32)

    filled_pad = np.pad(filled, (pad, pad), mode="reflect")
    weights_pad = np.pad(weights, (pad, pad), mode="reflect")

    smooth = np.convolve(filled_pad, kernel, mode="valid")
    norm = np.convolve(weights_pad, kernel, mode="valid")

    out = np.divide(smooth, norm, out=np.full_like(smooth, np.nan), where=norm > 1e-6)
    out[~valid] = np.nan
    return out.astype(np.float32)


def majority_filter_1d(arr01, k=5):
    arr01 = np.asarray(arr01)
    k = int(k)
    if k < 1:
        return arr01.astype(np.uint8, copy=True)
    if k % 2 == 0:
        k += 1

    pad = k // 2
    a = np.pad(arr01.astype(np.int32), (pad, pad), mode="edge")
    s = np.convolve(a, np.ones(k, dtype=np.int32), mode="valid")
    return (s >= (k // 2 + 1)).astype(np.uint8)


def build_pipeline(model_path: str, raw_depth: int):
    cfg, model, scaler, pca, class_mapping = load_artifacts(model_path)

    pipeline = OnlineBinaryFeaturePipeline(
        scaler=scaler,
        pca=pca,
        crop_first=cfg["crop_first"],
        crop_last=cfg["crop_last"],
        unstable_remove=cfg["unstable_remove"],
        gauss_sigma=cfg["gauss_sigma"],
        win=cfg["win"],
        step=cfg["step"],
        b_thresh=cfg["b_thresh"],
    )
    pipeline.init_for_depth(raw_depth)

    return pipeline, model, cfg, class_mapping


def run_session_inference(
    model_path,
    csv_path,
    producer_sleep_s=0.0,
    prob_gauss_sigma=1.0,
    threshold=0.5,
    majority_k=5,
    out_dir=None,
):
    csv_path = Path(csv_path)
    out_dir = Path(out_dir) if out_dir is not None else csv_path.parent / f"inference_{csv_path.stem}"
    out_dir.mkdir(parents=True, exist_ok=True)

    data_preview = np.loadtxt(csv_path, delimiter=",", dtype=np.float32)
    if data_preview.ndim != 2:
        raise ValueError(f"Expected 2D CSV, got shape {data_preview.shape}")

    raw_depth = data_preview.shape[0]

    pipeline, model, cfg, class_mapping = build_pipeline(model_path=model_path, raw_depth=raw_depth)

    print("loaded config:")
    print(cfg)
    print(f"raw depth: {raw_depth}")
    print(f"feature dim before scaler/pca: {pipeline.feature_dim}")
    print(f"threshold: {threshold}")
    print(f"class mapping: {class_mapping}")

    frame_buffer = KafkaFrameBuffer(maxlen=30000)
    stop_event = threading.Event()

    producer = CSVToKafkaProducer(
        csv_path=csv_path,
        frame_buffer=frame_buffer,
        stop_event=stop_event,
        sleep_s=producer_sleep_s,
    )
    producer.start()

    pred_classes = []
    p_open_list = []
    infer_times_ms = []
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
            frame_indices.append(frame_idx)

            t0 = time.perf_counter()
            pred_class, p_open = pipeline.predict_frame(frame, model, threshold=threshold)
            t1 = time.perf_counter()

            dt_ms = (t1 - t0) * 1000.0

            pred_classes.append(float(pred_class))
            p_open_list.append(p_open)
            infer_times_ms.append(dt_ms)

            if frame_idx % 100 == 0:
                state_name = class_mapping.get(pred_class, str(pred_class))
                print(
                    f"frame {frame_idx:06d} | "
                    f"p_open={p_open:.6f} | pred={pred_class} ({state_name}) | "
                    f"infer_time_ms={dt_ms:.3f} | buffer_len={len(frame_buffer)}"
                )

            frame_idx += 1

    finally:
        stop_event.set()
        producer.join(timeout=1.0)

    pred_classes = np.asarray(pred_classes, dtype=np.float32)
    p_open_list = np.asarray(p_open_list, dtype=np.float32)
    infer_times_ms = np.asarray(infer_times_ms, dtype=np.float32)
    frame_indices = np.asarray(frame_indices, dtype=np.int32)

    p_open_smooth = gaussian_filter_1d(p_open_list, sigma=prob_gauss_sigma) if prob_gauss_sigma > 0 else p_open_list.copy()
    pred_classes_smooth = (p_open_smooth >= threshold).astype(np.float32)

    if majority_k and majority_k > 1:
        pred_classes_majority = majority_filter_1d(pred_classes_smooth, k=majority_k).astype(np.float32)
    else:
        pred_classes_majority = pred_classes_smooth.copy()

    pred_csv = out_dir / "predictions.csv"
    with pred_csv.open("w", encoding="utf-8") as f:
        f.write("frame_idx,p_open,p_open_smooth,pred_class,pred_class_majority,infer_time_ms\n")
        for i, p, ps, c, cm, dt in zip(
            frame_indices,
            p_open_list,
            p_open_smooth,
            pred_classes_smooth,
            pred_classes_majority,
            infer_times_ms,
        ):
            f.write(f"{i},{p:.8f},{ps:.8f},{int(c)},{int(cm)},{dt:.6f}\n")

    plt.figure(figsize=(12, 5))
    plt.plot(frame_indices, p_open_list, label="p(open)", alpha=0.6)
    plt.plot(frame_indices, p_open_smooth, label="p(open) smooth")
    plt.axhline(threshold, linestyle="--", label="threshold")
    plt.xlabel("Frame index")
    plt.ylabel("Probability")
    plt.title("Open probability across time")
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "p_open_over_time.png", dpi=150)
    plt.close()

    plt.figure(figsize=(12, 4))
    plt.step(frame_indices, pred_classes_majority, where="post")
    plt.ylim(-0.1, 1.1)
    plt.xlabel("Frame index")
    plt.ylabel("Predicted state")
    plt.title("Binary state over time (0=closed, 1=open)")
    plt.tight_layout()
    plt.savefig(out_dir / "predicted_state_over_time.png", dpi=150)
    plt.close()

    print(f"done | frames={len(p_open_list)} | frac_open={float(np.mean(pred_classes_majority)):.6f} | mean_infer_ms={float(np.mean(infer_times_ms)):.4f}")
    print(f"saved predictions csv: {pred_csv}")


if __name__ == "__main__":
    model_path = r"C:\Users\leode\decision_trees_capstone\models\binary_classifier\binary_open_close_pipeline.joblib"
    csv_path = r"C:\Users\leode\decision_trees_capstone\data\dataset_two\raw_us\Raw\concatenated_sessions\for_binary_classifier\val\session_020_full.csv"

    run_session_inference(
        model_path=model_path,
        csv_path=csv_path,
        producer_sleep_s=0.0,
        prob_gauss_sigma=1.0,
        threshold=0.5,
        majority_k=5,
        out_dir=None,
    )