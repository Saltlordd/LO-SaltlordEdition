# Battle dialogue timing at 90/120 FPS (#148)

Status: source fix added in `LostOdysseyRecomp/patches/battle_script_timer.cpp`; the focused guest fixture and disabled negative control pass. Full runtime build, real-scene acceptance and player confirmation remain pending. Issue [#148](https://github.com/freefrank/LostOdysseyRecomp/issues/148) remains open (2026-10-02).

## Symptom and diagnosis

At native 90/120 FPS, battle dialogue waits can make no progress. The original guest code was written for a 30 FPS update loop but consumes a frame-like integer step. It converted each actual script update's elapsed time to a 60-FPS-equivalent value with `float(delta * 60)`, then used `fctiwz`; at high refresh rates the fractional value was discarded and the integer step was usually zero.

Independent reverse-engineering checks traced the elapsed-time path through `sub_8238AC40`, where the game subtracts the previous absolute millisecond value from the current one, and through `sub_82E74250`, which reads that delta. Its caller, `sub_8238A640`, multiplies the delta by `0.001`. Static inspection of the original PPC confirmed wait opcode `0x04` handled by `sub_82A9BF40`; it subtracts the integer step, which was therefore usually zero at 90/120 FPS. No runtime trace of this opcode was used.

## Fix

`patches/battle_script_timer.cpp` hooks `sub_8238AE08` and only acts when the link register is `8238AD64`. It uses `f31` to carry fractional 60-Hz ticks between actual script updates and replace the integer step immediately before the statically identified wait consumer, preserving the rest of the script path. The script init path `sub_82A9DBB8` resets the state. The release path `sub_82A9E3B8` clears the pointer and resets the state only when release succeeds; a failed release retains the fractional remainder. `LO_BATTLE_SCRIPT_TIMER=0` disables the correction for an A/B comparison.

The patch is scoped to the statically identified consumer and delta chain. It does not claim that every frame-based battle timer has been audited.

## Available evidence

- Source-side reverse engineering independently confirmed the discarded-fraction diagnosis and the `sub_8238AC40` → `sub_82E74250` delta path, with the `0.001` multiplication in caller `sub_8238A640`; static original PPC inspection confirmed handler `sub_82A9BF40` for wait opcode `0x04`.
- The supplied v0.7.35 revision `95f2c8968d0c` Direct3D 12 log was captured at 90 FPS. Two bounded measurements from phase 1 ran from `808.643` to `896.836` (`88.193 s`) and from `1841.174` to `1906.246` (`65.072 s`). These spans include global idle gaps of `27.676 s` and `9.377 s`, plus an F1 capture, so they are not dialogue-wait durations.
- The audio queue reported `6144/8192` bytes with zero drops and zero errors. No new XMA warning appeared during the battle portion of this log.

## Fixture validation

The generated guest fixture passes enabled 30/60/90/120-Hz timing, mixed and jittered frame intervals, early-return and carry/reset cases, target-path filtering, and native edge-case behavior. The disabled 90/120-Hz negative control remains stuck as expected. Reproduce it with:

```text
python tools/tests/battle_script_timer_test.py --generated-dir <generated PPC directory> --simde-dir <SIMDe directory> --compiler clang++
```

On Windows, run this from a prepared developer command prompt or add `--vcvars <vcvars64.bat>`.

## Pending validation and limits

No full runtime build, live gameplay comparison in the two target dialogue scenes, or player acceptance has been recorded for this correction. The bounded 90-FPS log is consistent with the report but does not prove the root cause or an opcode trigger in that scene; the guest fixture supports the scoped source change. Issue #148 is not established as fully resolved or ready to close. No release or real-scene acceptance is claimed.
