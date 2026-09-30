# Project review fixes — 2026-09-30

Review baseline: `8c7b749dae3b5162c82d7a31b560bced713f86af` (`main`).
The review examined runtime startup/exit, guest memory and file APIs,
callbacks, GPU command execution, texture uploads/cache, settings and updates.
The changes address these 13 confirmed findings (4 P1, 9 P2):

| Finding | Priority | Result |
|---|---|---|
| R01: file-query output overruns | P1 | Validate fixed structures, variable headers and null buffers before writing. Short buffers return `STATUS_INFO_LENGTH_MISMATCH`; partial names report `STATUS_BUFFER_OVERFLOW` with actual bytes written. |
| R02: allocation rounding overflow | P1 | Round sizes/alignment in 64 bits; reject unrepresentable or oversized requests before allocator state changes. Check virtual-memory size plus base offset and required output pointers. |
| R03: missing mip source in texture identity | P1 | Include effective mip source address in key equality/hash. Legacy five-field captured-key input remains readable. |
| R04: recursive/malformed GPU lists | P1 | Validate physical extents, active-path cycles, 64-level depth, and a 2^24-packet batch budget. Validate declared/opcode operands and variable spans, propagate nested errors, preserve packet boundaries and copy wrapped inline shader payloads safely. Sequential reuse remains valid. |
| R05: missed texture mutations | P2 | Keep the per-frame sampled check (head, tail and 64 windows; short tails now included) and add a complete XXH3 scan of each texture's base/mip stored extents every 16 frames, staggered by address, so a write outside the windows is detected within 16 frames. Remove the old 64 MiB hash clamp; validate physical/upload extents before allocation or decoding. |
| R06: ignored seek/read errors | P2 | Validate offsets, check seek results, distinguish `ferror`/EOF and track actual transferred bytes for ordinary/scatter reads. Failed seeks do not read stale-position data. Scatter segments keep only the 32-bit guest pointer of each `PVOID64`, so a sign-extended high word (physical memory at `0xA0000000+`) is accepted as before; null and end-of-memory spans are rejected. |
| R07: resize failure reported as success | P2 | Validate input/access, propagate flush/resize failures and change logical size only on success. Allocation hints preserve EOF, matching the existing Xenia API convention. |
| R08: audio callback after unregister | P2 | Publish callback/parameter together, track acquired calls and drain them on external unregister. Self-unregister disables future calls without waiting for itself; exception unwinding releases the call. |
| R09: GPU callback data race | P2 | Publish/read callback and user data under one mutex, then release it before guest invocation. Both vsync and queued interrupt paths use a paired snapshot. |
| R10: thread destructor termination on guest return | P2 | Route guest-main return through existing GPU-owner cleanup and process exit; use `_Exit` fallback if the worker has already stopped. |
| R11: inconsistent first-run settings path | P2 | Share `SettingsPath()` across configuration read/write, startup preferences and first-run detection; preserve portable working-directory semantics and XDG layouts. |
| R12: unusable Flatpak update instructions | P2 | Guide standalone-bundle users to the official download and bundle installation, with user/system scope guidance in all five UI languages. Production and fixture implementations share the text. |
| R13: lost directory entry on short buffer | P2 | Consume a matching entry only after successful delivery; a larger-buffer retry returns the same entry. Nonmatching entries still advance. |

## Verification

The independent [CMake/CTest suite](../../tools/tests/review_regressions/CMakeLists.txt)
has 14 Linux tests. The integrated run passed all 14 with ASan/UBSan enabled
for the applicable fixtures. The GPU parser was checked again after its final
packet-boundary change. Audio and GPU callback regressions also passed separate
TSan runs. Key/content checks cover 52,337/516 assertions; file APIs cover
1,234 checks. Allocator negative controls substituting the original production
functions each fail at the intended regression assertion.

The SDL tests compile both the fixture UI and actual production POSIX updater
implementation, verify all localized instruction/command widths, and exercise
dummy-driver returns. Allocation/file/parser/exit fixtures use production code
with synthetic guest storage or driver stubs. See [test commands](../../tools/tests/README.md#project-review-regressions-2026-09-30).

The first pass ran in a cloud checkout without private game input or
generated PPC sources, so it did not build or launch the full runtime.

### Windows runtime follow-up

The branch was rebased onto `ab7a2de` and the full runtime was built locally
with clang-cl. The 14-test suite passed again in WSL Manjaro with ASan/UBSan,
including a new sign-extended scatter segment case. The Windows header tests
also passed: `LoTextureContentTest` (1,864 checks), `LoTextureKeyTest`,
`LoAudioCallbackTest`, `LoSettingsPathTest`, `LoExternalUpdateNoticeTest` and
`LoUserPathsTest`.

The same scripted new-game input ran on this branch and on a `main` build
from the same checkout, each for 150 seconds and 4,381 swaps. Both logs had
the same single existing warning and no errors. From swap 750 on, all 25
periodic screenshots were pixel-identical; the four earlier ones differ only
in animated intro and title frames.

The frozen Uhra 4K environment loads a save with Continue and renders the
plaza at 3840x2160 on D3D12. On every run, the scene rendered as on `main`,
no texture was re-uploaded, and the logs held no warnings besides the
environment's shader-cache write failures, which `main` shows as well.
Uncapped alternating runs gave:

| Build | Runs (FPS) | Command processor CPU (ms/frame) |
|---|---|---|
| `main` | 140.8, 141.4 | 6.62, 6.66 |
| First R05 version (full scan every frame) | 119.3, 130.0 | 7.67, 7.09 |
| `main` | 131.0, 142.0 | 6.95, 6.61 |
| Final R05 (sampled + staggered full scans) | 142.5, 139.2 | 6.53, 6.75 |
| `main` (second session) | 130.8, 145.8 | 6.87, 6.42 |
| Final R05 (second session) | 139.4, 142.4 | 6.72, 6.59 |

Each session ran `main`, branch, branch, `main`; the first run of a session
was always the slowest. Over both sessions of the final version, `main`
averaged 137.4 FPS at 6.71 ms and the branch 140.9 FPS at 6.65 ms.

Diagnostic runs with `LO_RENDER_TIMING` located the first version's cost in
the texture-bind phase (0.91 ms on `main`, 1.51 ms with full scans). The final
version measured 1.00 ms. Individual diagnostic runs of both builds sometimes
fell into a state where every CPU phase ran about 15% slower. Compare runs
from the same session and state only.

Real audio-device unregistering, Flatpak installation, save writing and long
gameplay were not exercised.

## Implementation limits and follow-up measurement

The first version of R05 scanned every active texture completely on every
frame. In the 4K Uhra benchmark that is 173 textures and 24 MB per frame, and
it added about 0.4-0.6 ms to the renderer's texture-bind phase. Replacing the
sampled windows with short XXH3 calls was no cheaper (about 20,000 calls per
frame). The kept design uses the previous inline sampled loop every frame and
spreads full scans over 16 frames, so a write outside the windows is detected
with at most 16 frames of delay. A later dirty-page/write-generation design
needs to cover every guest writer before it can replace these checks.
Existing mid-frame writes and decode-versus-hash concurrency are not addressed
by this change.

Guest threads have no uniform cancellation mechanism. The guest-return fix
therefore reuses the established process-exit policy, rather than promising to
join arbitrary guest callbacks. It performs GPU-owner cleanup and log flushing,
then skips global destruction while detached guest code may survive.

Further UI work identified by the review includes timed display-mode recovery,
visible pending settings, Linux first-run interaction and asynchronous directory
listing. Performance work should start with measured texture-cache growth,
buffer residency, fragmented allocator latency, write/flush/log frequency and
shader-worker effective CPU/memory quotas. These recommendations are separate
from the confirmed fixes; this branch claims no measured speedup.
