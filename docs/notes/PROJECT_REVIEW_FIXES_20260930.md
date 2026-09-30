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
| R05: missed texture mutations | P2 | Replace sampled checks with complete XXH3 scans of base/mip stored extents, including tails. Remove the old 64 MiB hash clamp; validate physical/upload extents before allocation or decoding. |
| R06: ignored seek/read errors | P2 | Validate offsets, check seek results, distinguish `ferror`/EOF and track actual transferred bytes for ordinary/scatter reads. Failed seeks do not read stale-position data. |
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

The full runtime was not built or launched: this cloud checkout lacks private
game input and generated PPC sources, and the runtime requires Clang. Real GPU,
Windows CRT, audio-device, Flatpak installation and gameplay acceptance remain
unverified. No CI result is claimed before the new workflow runs remotely.

## Implementation limits and follow-up measurement

Full texture scans restore detection of mutations outside sampled windows,
but increase scan work proportional to active pitched texture data (including
padding). Gameplay CPU bandwidth and frame-time costs are unmeasured. A later
dirty-page/write-generation design needs to cover every guest writer before it
can safely replace these checks. Existing mid-frame writes and decode-versus-
hash concurrency are not addressed by this change.

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
