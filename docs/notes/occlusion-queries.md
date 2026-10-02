# Host GPU occlusion queries (#118)

Status 2026-10-02: merged as PR #122 (`c0163a8`) and part of v0.7.35 (tag
`v0.7.35`). Direct3D 12 and Vulkan; Metal keeps the old fake counts. Checked in
the frozen Uhra save and at the #118 spot on the world map on an RTX 5080.

## Why the flare showed through terrain

The game measures how much of the sun is visible with GPU occlusion queries.
The command processor used to answer every `EVENT_WRITE_ZPD` with a made-up,
growing count (`LO_ZPD_MODE=grow`), so every query read as fully visible and
the flare never faded behind geometry. This has been the case since the first
release; it is not a regression.

## What the guest does with a query

D3D issues a ZPD event for the BEGIN record (slot + 0x20), the query's draws,
then a ZPD event for the END record (the 64-byte slot base). Each record is
`xe_gpu_depth_sample_counts`: eight little-endian dwords, ZPass at +16/+20.

`sub_823CF3F0` (D3D `GetData`, type 9) returns "not ready" while the GPU fence
recorded in the query object (`query+24`) has not passed, kicking the command
buffer when the query is still in the open segment. After the fence it sums
`END.ZPassA + END.ZPassB - BEGIN.ZPassA - BEGIN.ZPassB` over the query's records
(`query+28`, count at `query+148`) and still reports "not ready" while the last
END record holds the sentinel `0xFFFFFEED` in both ZPass halves. Its caller
`sub_823CF390` loops without a timeout. UE3 (`sub_823CE960`) clears a
primitive's visibility bit when the result is zero.

In Uhra the game issues about 256 queries per frame and reads them in the same
frame, right after issuing them (main render flow: depth prepass, query
results, base pass).

## Implementation

- Plume (`tools/patches/plume-lostodyssey.patch`):
  `RenderDevice::createOcclusionQueryPool`,
  `RenderCommandList::beginOcclusionQuery` / `endOcclusionQuery` and the
  `occlusionQueries` / `occlusionQueryPrecise` capabilities. D3D12 uses an
  `OCCLUSION` query heap and resolves each query into a readback buffer when it
  ends. Vulkan uses a `VK_QUERY_TYPE_OCCLUSION` pool, begins the render pass
  before the query, uses `PRECISE` when the device supports it, and reads results
  with availability. The defaults return no pool, so Metal compiles unchanged and
  the renderer falls back to the fake counts.
- Renderer: each GPU slot owns a 1024-query pool, reset when its command list
  opens. A guest draw inside a guest query is wrapped in its own host query; the
  results are read when the slot's fence has completed (`RecycleSlot`).
- `gpu/occlusion_queries.h` keeps the bookkeeping, tested by
  `tools/tests/occlusion_queries_test.cpp` (review-regressions):
  - Host samples are converted with the target's guest/host size ratio times the
    guest MSAA sample count (host targets are single-sampled). Any passing host
    sample keeps at least 1.
  - A query with a draw that returned before its host query, no draws, an END
    without BEGIN, an unavailable result or a failed batch reads as visible
    (`0x10000`), as before.
  - A BEGIN for a slot whose previous query is still pending drops the old
    result: the record belongs to the new query.

## Modes (`LO_ZPD_MODE`)

| Value | END record | Culling | Uhra cost (D3D12, 2560x1440, uncapped, ABBA) |
|---|---|---|---|
| unset / `host` (default) | Written at the event with the last measured count of the same record; 1 when that was zero or unknown | Never: no query reads zero | 128.8 FPS vs 130.3 fake (about 1%) |
| `strict` | Written once the host GPU has counted it; the guest waits | With exact counts | 111.2 FPS vs 131.7 fake (about 15%) |
| `grow`, `xenia`, `begin0`, `none` | Old fake counts, no host queries | Never | baseline |

The default follows Xenia's default `fast` mode: counts used as magnitudes, such
as flare visibility, follow the real ones a frame or two late, while culling is
unchanged from the fake counts, so a stale zero can never hide an object.

Strict needs the command processor to complete records while the guest polls:
the `GetData` hook (`debug/query_trace.cpp`) flags a waiting guest, and the
command processor flushes the open batch and waits for its fence when it is idle
or about to block in `WAIT_REG_MEM`. Because the game reads its queries in the
same frame, every frame then waits for the host GPU to finish its depth prepass
and queries (1.6 ms per frame in Uhra). Submitting the work before the first
query earlier did not shorten that wait and lowered the rate further (about
104 FPS), so it was removed. Any failure (device loss, failed submit or wait,
shutdown) writes the waiting records as visible, so a guest never waits forever.

## Validation so far

- Vulkan and D3D12, Uhra, 60 FPS cap: no errors; about 4.6% of queries measured
  zero in strict mode and 14.6% with the default (more primitives are drawn when
  nothing is culled).
- Screenshots at swap 1800, default and strict against `grow` on D3D12: only
  animated characters, swaying foliage, the animated sign and their shadows
  differ; no static object disappears.
- The query proxy draws have no pixel shader. NVIDIA counts their samples; Xenia
  binds an empty pixel shader for such draws because some drivers do not, which
  this change does not do yet.
- #118 world-map flare, Vulkan, 1600x900: the user saved at the reported spot
  (slot 08, Twilight Ocean, disc 4), where the sun sits behind a cliff next to
  the Nautilus. Loading it with `LO_ZPD_MODE=grow` drew the sun disc and a warm
  glare over the cliff in all three screenshots taken 3–6 s after loading; the
  default mode showed the dark cliff with no sun and no glare in all three.
  Strict mode was not run there.
- Not checked: AMD and Intel GPUs, exclusive fullscreen. The disc 1 jail scene and the disc 3 train fight, which Xenia's
  "Disable Occlusion Queries" patch for this game mentions, have not been run
  in strict mode.
