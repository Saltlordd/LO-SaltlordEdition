# Battle camera spin at 90/120 FPS (#117) — 2026-10-01

Status: fixed in `LostOdysseyRecomp/patches/battle_camera_smoother.cpp`. A model of the game's algorithm confirms the fix; it has not yet been checked in the game.

## Symptom

At 90 and 120 FPS, after some attacks the battle view jumps between unrelated angles for up to about two seconds. At 60 FPS this does not happen. A per-frame trace of the scene camera (`LO_TRACE_HIT_CAMERA`, diagnostic only) showed:

- The eye position kept moving smoothly.
- On some frames the orientation took a random yaw and pitch (roll stayed 0), and the next frame was often normal again.
- In one episode the second camera of the attack kept doing this for 1.2 s.

## Where the angle comes from

- **Battle camera.** The battle manager at `0x832CA0E0` holds the battle camera object at `+232` (`0x832CA1C8`, vtable `0x8201CE68`). Its per-frame update is slot 3, `sub_82390A70`. The final POV (location, `FRotator`, FOV; 7 words) sits at camera `+168`. The script camera `AbcCamera` copies it through slot 9 in `GetCameraAttribute`.
- **How the writer was found.** Hardware write breakpoints on the POV pitch and yaw logged every writer and its guest back trace. `sub_82390A70` writes the POV twice per frame:
  1. The raw rotation computed by `sub_82396FC0`.
  2. Raw rotation + offset × `camera+4812`, normalized to 16 bits. The offset comes from a rotation smoother at camera `+1088`: `SetInput` (`sub_82397198`, POV and camera-cut flag), then `Update` (`sub_82397840` → `sub_82397FA8` → `sub_82398458`). `sub_82399518` reads it.
- **Where the bad values appeared.** On the bad frames only the second write was wrong. The smoother's history held values such as -1127247 and +8785489 (16-bit angle units).

## Root cause

The smoother keeps the last 100 frame deltas and rotations, newest first. `sub_82398458` resamples them at fixed 12.5 ms steps, then averages the first 31 offsets from the raw rotation. Each step works like this:

1. It consumes at most one history entry, and only while the time left before the step is below 12.5 ms.
2. It moves toward that entry with weight `0.0125 / time left`.
3. It subtracts 12.5 ms from the time left.

This is a linear interpolation only while every entry spans at least one step, that is, while every frame takes 12.5 ms or longer (80 FPS or below). On a camera cut the history is refilled with 1/30 s entries, the game's native rate.

With shorter frames each step leaves a deficit:

- The weight rises past 1, so the step extrapolates instead of interpolating.
- The time left falls through zero, where the weight is a division by nearly zero.
- The resampled rotations explode and wrap to random 16-bit angles.

Whether a frame is hit depends on its exact delta sequence, which is why good and bad frames alternate. It is visible only when the camera rotates and the camera's offset scale is non-zero, as in attack cameras. A second resampling pass over the smoothed history in the same function has the same pattern.

## Fix

`patches/battle_camera_smoother.cpp` hooks `SetInput` and `Update`:

- They run only once at least 12.5 ms has accumulated since the smoother's last update, and `Update` receives the accumulated delta. Every history entry therefore spans a full step.
- On frames in between, `sub_82390A70` keeps adding the last offset to the current raw rotation.
- A camera cut always updates immediately and records at least one step.
- A smoother that has not been fed for 250 ms (between battles) updates on its next frame.
- At 80 FPS and below every frame still updates, so 30 and 60 FPS behave exactly as before.

`LO_BATTLE_CAMERA_SMOOTHER_GATE=0` restores the per-frame updates for A/B runs.

## Validation

- A Python model of the first resampling loop was run 50 times per frame rate, each run a camera cut followed by an accelerating 0.6 s pan with ±15% frame-time jitter.

  | Frame rate | Worst offset without the gate | Worst offset with the gate |
  |---|---|---|
  | 30, 60 FPS | about 21° (normal smoothing lag) | about 21° (identical) |
  | 90, 120, 144 FPS | diverges (thousands of degrees or more) | 20.4–20.5° |

- In-game check at 120 FPS with the reporter's kind of attack: pending. Same build with `LO_BATTLE_CAMERA_SMOOTHER_GATE=0` for the A/B.
