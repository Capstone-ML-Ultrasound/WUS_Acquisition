#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "LeapC.h"
#include "ExampleConnection.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  float w, x, y, z;
} Quat;

typedef struct {
  int valid;
  Quat arm_q0;          /* neutral arm orientation in world/device frame */
  Quat palm_rel_q0;     /* neutral palm orientation relative to arm */

  Vec3 forearm_axis0;   /* neutral forearm axis in world/device frame */
  Vec3 radial_axis0;    /* neutral radial axis in arm-local frame */
  Vec3 dorsal_axis0;    /* neutral dorsal axis in arm-local frame */
  Vec3 hand_dir0;       /* neutral hand direction in arm-local frame */
} NeutralPose;

static int64_t lastFrameID = 0;
static NeutralPose g_neutral = {0};

/* ---------- timing ---------- */

static uint64_t now_ms(void) {
#ifdef _WIN32
  return GetTickCount64();
#else
  return 0;
#endif
}

static uint64_t now_unix_ns(void) {
#ifdef _WIN32
  typedef VOID (WINAPI *GetSystemTimePreciseAsFileTimeFn)(LPFILETIME);
  FILETIME file_time;
  ULARGE_INTEGER ticks;
  HMODULE kernel32_module = GetModuleHandleW(L"kernel32.dll");
  GetSystemTimePreciseAsFileTimeFn get_precise_time = NULL;

  if (kernel32_module) {
    get_precise_time = (GetSystemTimePreciseAsFileTimeFn)GetProcAddress(
      kernel32_module,
      "GetSystemTimePreciseAsFileTime"
    );
  }

  if (get_precise_time) {
    get_precise_time(&file_time);
  } else {
    GetSystemTimeAsFileTime(&file_time);
  }

  ticks.LowPart = file_time.dwLowDateTime;
  ticks.HighPart = file_time.dwHighDateTime;

  return (uint64_t)((ticks.QuadPart - 116444736000000000ULL) * 100ULL);
#else
  struct timespec ts;

  if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    return 0;

  return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
#endif
}

/* ---------- vec helpers ---------- */

static Vec3 v3(float x, float y, float z) {
  Vec3 v = { x, y, z };
  return v;
}

static Vec3 v_sub(Vec3 a, Vec3 b) {
  return v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static Vec3 v_scale(Vec3 v, float s) {
  return v3(v.x * s, v.y * s, v.z * s);
}

static float v_dot(Vec3 a, Vec3 b) {
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

static Vec3 v_cross(Vec3 a, Vec3 b) {
  return v3(
    a.y*b.z - a.z*b.y,
    a.z*b.x - a.x*b.z,
    a.x*b.y - a.y*b.x
  );
}

static float v_norm(Vec3 v) {
  return sqrtf(v_dot(v, v));
}

static Vec3 v_normalize(Vec3 v) {
  float n = v_norm(v);
  if (n < 1e-6f) return v3(0.0f, 0.0f, 0.0f);
  return v_scale(v, 1.0f / n);
}

static Vec3 v_project_plane(Vec3 v, Vec3 n_unit) {
  return v_sub(v, v_scale(n_unit, v_dot(v, n_unit)));
}

static float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

static float rad2deg(float r) {
  return r * (180.0f / (float)M_PI);
}

static float signed_angle_about_axis(Vec3 u, Vec3 v, Vec3 axis_unit) {
  Vec3 up = v_normalize(v_project_plane(u, axis_unit));
  Vec3 vp = v_normalize(v_project_plane(v, axis_unit));

  if (v_norm(up) < 1e-6f || v_norm(vp) < 1e-6f)
    return 0.0f;

  {
    float c = clampf(v_dot(up, vp), -1.0f, 1.0f);
    float s = v_dot(axis_unit, v_cross(up, vp));
    return atan2f(s, c);
  }
}

/* ---------- quat helpers ---------- */

static Quat quat_identity(void) {
  Quat q = { 1.0f, 0.0f, 0.0f, 0.0f };
  return q;
}

static Quat quat_from_leap(LEAP_QUATERNION q) {
  Quat out = { q.w, q.x, q.y, q.z };
  return out;
}

static Quat quat_conj(Quat q) {
  Quat out = { q.w, -q.x, -q.y, -q.z };
  return out;
}

static Quat quat_mul(Quat a, Quat b) {
  Quat out;
  out.w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;
  out.x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y;
  out.y = a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x;
  out.z = a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w;
  return out;
}

static Quat quat_normalize(Quat q) {
  float n = sqrtf(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
  if (n < 1e-6f) return quat_identity();
  {
    Quat out = { q.w/n, q.x/n, q.y/n, q.z/n };
    return out;
  }
}

static Vec3 quat_rotate_vec(Quat q, Vec3 v) {
  Quat p = { 0.0f, v.x, v.y, v.z };
  Quat qc = quat_conj(q);
  Quat r = quat_mul(quat_mul(q, p), qc);
  return v3(r.x, r.y, r.z);
}

static Quat quat_twist_about_axis(Quat q, Vec3 axis_unit) {
  Vec3 v = v3(q.x, q.y, q.z);
  Vec3 proj = v_scale(axis_unit, v_dot(v, axis_unit));
  Quat twist = { q.w, proj.x, proj.y, proj.z };
  return quat_normalize(twist);
}

/* ---------- calibration ---------- */

static int calibrate_neutral(const LEAP_HAND* hand) {
  Quat q_arm = quat_normalize(quat_from_leap(hand->arm.rotation));
  Quat q_palm = quat_normalize(quat_from_leap(hand->palm.orientation));
  Quat q_arm_inv = quat_conj(q_arm);

  Vec3 arm_prev = v3(hand->arm.prev_joint.x, hand->arm.prev_joint.y, hand->arm.prev_joint.z);
  Vec3 arm_next = v3(hand->arm.next_joint.x, hand->arm.next_joint.y, hand->arm.next_joint.z);
  Vec3 forearm_world = v_normalize(v_sub(arm_next, arm_prev));

  Vec3 hand_dir_world = v_normalize(
    v3(hand->palm.direction.x, hand->palm.direction.y, hand->palm.direction.z)
  );
  Vec3 palm_norm_world = v_normalize(
    v3(hand->palm.normal.x, hand->palm.normal.y, hand->palm.normal.z)
  );

  Vec3 hand_dir_arm = v_normalize(quat_rotate_vec(q_arm_inv, hand_dir_world));
  Vec3 palm_norm_arm = v_normalize(quat_rotate_vec(q_arm_inv, palm_norm_world));
  Vec3 forearm_arm = v_normalize(quat_rotate_vec(q_arm_inv, forearm_world));

  Vec3 palm_proj = v_normalize(v_project_plane(palm_norm_arm, forearm_arm));
  Vec3 radial = v_normalize(v_cross(palm_proj, forearm_arm));
  Vec3 dorsal = v_normalize(v_cross(forearm_arm, radial));

  if (v_norm(hand_dir_arm) < 1e-6f ||
      v_norm(forearm_arm) < 1e-6f ||
      v_norm(radial) < 1e-6f ||
      v_norm(dorsal) < 1e-6f) {
    return 0;
  }

  g_neutral.valid = 1;
  g_neutral.arm_q0 = q_arm;
  g_neutral.palm_rel_q0 = quat_mul(q_arm_inv, q_palm);
  g_neutral.forearm_axis0 = forearm_world;
  g_neutral.radial_axis0 = radial;
  g_neutral.dorsal_axis0 = dorsal;
  g_neutral.hand_dir0 = hand_dir_arm;
  return 1;
}

/* ---------- angle computation ---------- */

static int compute_wrist_angles_deg(
  const LEAP_HAND* hand,
  float* pronation_deg,
  float* flexion_deg,
  float* deviation_deg
) {
  if (!g_neutral.valid)
    return 0;

  {
    Quat q_arm = quat_normalize(quat_from_leap(hand->arm.rotation));
    Quat q_palm = quat_normalize(quat_from_leap(hand->palm.orientation));
    Quat q_arm_inv = quat_conj(q_arm);

    Vec3 hand_dir_world = v_normalize(
      v3(hand->palm.direction.x, hand->palm.direction.y, hand->palm.direction.z)
    );
    Vec3 hand_dir_arm = v_normalize(quat_rotate_vec(q_arm_inv, hand_dir_world));

    /* pron/sup from arm rotation relative to neutral */
    Quat q_arm_rel = quat_mul(quat_conj(g_neutral.arm_q0), q_arm);
    q_arm_rel = quat_normalize(q_arm_rel);

    /* use neutral forearm axis in world frame for twist extraction */
    Quat q_twist = quat_twist_about_axis(q_arm_rel, g_neutral.forearm_axis0);

    float twist_scalar = q_twist.x * g_neutral.forearm_axis0.x +
                         q_twist.y * g_neutral.forearm_axis0.y +
                         q_twist.z * g_neutral.forearm_axis0.z;

    float pron = 2.0f * atan2f(twist_scalar, q_twist.w);

    /* flex/dev from hand direction relative to arm */
    float flex = signed_angle_about_axis(
      g_neutral.hand_dir0,
      hand_dir_arm,
      g_neutral.radial_axis0
    );

    float dev = signed_angle_about_axis(
      g_neutral.hand_dir0,
      hand_dir_arm,
      g_neutral.dorsal_axis0
    );

    *pronation_deg = rad2deg(pron);
    *flexion_deg = rad2deg(flex);
    *deviation_deg = rad2deg(dev);
    return 1;
  }
}

/* ---------- main ---------- */

int main(void) {
  FILE* fp;
  int neutral_countdown_started = 0;
  int neutral_locked = 0;
  uint64_t neutral_start_ms = 0;
  uint64_t last_countdown_print_ms = 0;
  const uint64_t neutral_hold_ms = 5000;

  CloseConnection();
  millisleep(500);
  OpenConnection();

  while (!IsConnected)
    millisleep(100);

  printf("Connected.\n");
  printf("Place your hand in neutral and hold still for 5 seconds.\n");

  fp = fopen("wrist_angles.csv", "w");
  if (!fp) {
    perror("fopen");
    return 1;
  }

  fprintf(
    fp,
    "frame_id,timestamp_unix_ns,hand_id,hand_type,"
    "pronation_supination_deg,flexion_extension_deg,deviation_deg\n"
  );
  fflush(fp);

  for (;;) {
    LEAP_TRACKING_EVENT* frame = GetFrame();

    if (frame && frame->tracking_frame_id > lastFrameID) {
      uint32_t h;
      uint64_t timestamp_unix_ns;
      lastFrameID = frame->tracking_frame_id;

      if (!neutral_locked) {
        if (frame->nHands > 0) {
          uint64_t t_now = now_ms();

          if (!neutral_countdown_started) {
            neutral_countdown_started = 1;
            neutral_start_ms = t_now;
            last_countdown_print_ms = 0;
            printf("Hand detected. Starting 5 second neutral hold...\n");
          }

          if (t_now - last_countdown_print_ms >= 1000) {
            uint64_t elapsed = t_now - neutral_start_ms;
            uint64_t remaining_ms = (elapsed >= neutral_hold_ms) ? 0 : (neutral_hold_ms - elapsed);
            printf("Hold still... %.1f s remaining\n", remaining_ms / 1000.0);
            last_countdown_print_ms = t_now;
          }

          if (t_now - neutral_start_ms >= neutral_hold_ms) {
            if (calibrate_neutral(&frame->pHands[0])) {
              neutral_locked = 1;
              printf("Neutral calibrated. Starting acquisition.\n");
            } else {
              neutral_countdown_started = 0;
              printf("Calibration failed. Try again.\n");
            }
          }
        } else if (neutral_countdown_started) {
          neutral_countdown_started = 0;
          printf("Hand lost. Countdown reset.\n");
        }

        millisleep(5);
        continue;
      }

      timestamp_unix_ns = now_unix_ns();

      for (h = 0; h < frame->nHands; h++) {
        LEAP_HAND* hand = &frame->pHands[h];
        const char* handType =
          (hand->type == eLeapHandType_Left ? "left" : "right");
        float pron = 0.0f, flex = 0.0f, dev = 0.0f;

        if (!compute_wrist_angles_deg(hand, &pron, &flex, &dev))
          continue;

        printf(
          "Frame %lld | %s | pron/sup %.1f | flex/ext %.1f | deviation %.1f\n",
          (long long)frame->tracking_frame_id,
          handType,
          pron,
          flex,
          dev
        );

        fprintf(
          fp,
          "%lld,%llu,%u,%s,%.6f,%.6f,%.6f\n",
          (long long)frame->tracking_frame_id,
          (unsigned long long)timestamp_unix_ns,
          hand->id,
          handType,
          pron,
          flex,
          dev
        );
      }

      fflush(fp);
    }

    millisleep(5);
  }

  fclose(fp);
  return 0;
}