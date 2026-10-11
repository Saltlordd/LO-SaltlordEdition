# Test9 source reconstruction checkpoint — not release source

The original Test9 commit 8843e7549ee5844e00c665df23b429077c88f8bf was not recovered. This checkpoint reconstructs documented behavior from the surviving v61 foundation d1ae779e2424ebedaa010d6a2fc68e3f270e74c3 and pinned upstream v0.9.22 d3888a0d879a23ff26197a3ad29fe64975b4b6b8. It is not proven identical to the tested APK.

## Recovered/reconstructed

- Complete earlier Android source foundation.
- Pipeline P1–P4 recipe/cache/library/scene-worker integration, normalized v2 recipes, learned migration, per-worker readers and sibling jobs; completion queue reconstructed separately while startup stays synchronous.
- Separate exact 3974-record shipped corpus; Java preserves native-owned learned files and disarms legacy reset without deleting saves or driver caches.
- Bounded two-worker Android settings, synchronous library preparation and driver-cache write before gameplay.
- One shared Plume Vulkan cache, seeded before presentation, selected-driver cache path and identity validation; custom-loader and Android surface handling retained.
- Live saved culling and upstream native font alignment fix.

## Verification performed

Production Android Java compile against SDK35 and D8 passed. Recipe tests passed normalization, v1 migration, scene tags, corruption/truncation, exact roundtrip and atomic writes. Generic/Android driver cache tests passed identity and preservation. Java corpus checks passed 3974 records, corruption/schema/truncation rejection, separate installation, learned v2/opaque-file preservation, saves/driver-cache preservation and repeat installation. Integrated Plume patch replay byte-matched all 11 modified files on base d890ac899e505fb30040e037a4037cdeca68f033. Diff whitespace checks passed.

## Native rebuild completed (2026-10-11)

All 88 runtime translation units and the Plume Vulkan backend compile with Android NDK r29 and link successfully with no unresolved symbols. Preserved PPC and support archives are reused, not regenerated. PPCContext size/alignment and all 206 member offsets match preserved Test9 DWARF. All 29 settings Config member offsets also match. Executable-section comparisons identify remaining differences; equality of section bytes alone does not prove relocation, data or behavioral equivalence. ARM64 runtime load segments use 16 KiB alignment.

Build checks exposed and corrected missing Plume logging-header packaging, the CurrentBattle prefetch dependency, an orphan desktop map-command reference, portable-pack path/contract API mismatches and duplicate legacy video cache calls. The shared renderer now owns driver-cache persistence. The inactive Android motion-replay dependency patch is restored and compiled. The unused prefetchHarvested field was removed to restore the original renderer member layout.

Final wizard/options/report text was recovered from the preserved version63 DEX string table: 37 long literals, the Performance heading, removal of the layout-note paragraph and the separate concise report-success message. This does not establish identical Java bytecode or exact original source.

## Not yet complete

The original commit is still missing. Native/DEX/APK equivalence and phone performance remain unverified. The original signing key was recovered privately from the Phase24 kit and its certificate matches the tested APK. Recovery1 packaging is a separate test action; never generate a replacement signing key. The build reuses compiled game code and support libraries, so it is not a reproducible end-to-end source build.

Keep the excellent tested version63 APK unchanged: SHA256 ce98db7ea326fd29fce049ad5ed9bcbe2ca3c006424aa7f5204b9e305ae3a655, 145945848 bytes. Corpus SHA256 5ced4aa19e9326ba1ad9a8ab695d94d720feef173ec2d68b16700d9abccb5c18.

## Plume preparation

On this reconstruction branch, check out Plume d890ac899e505fb30040e037a4037cdeca68f033 and apply ONLY tools/patches/plume-android-v0922-integrated.patch. Do not apply the old plume patch chain as well. Other dependency pins/patches remain from the restored foundation.

## Next steps

Use the private native rebuild checkpoint for compiler commands, headers, logs and comparisons. Resolve remaining native differences against preserved Test9 objects; use the recovered original signing key privately; package a clearly labelled recovery test and compare certificates/support libraries/corpus before a phone regression test. Secure verified source before releasing beta0.1.1. The older warmup Java test targets superseded v1 merge behavior; use pipeline_corpus_recovery_test.py for this branch.

No signing secrets, compiled recovery objects or user game files belong in the public source archive. Integration by Saltlord with AI assistance; upstream freefrank and contributors retain their attribution/licences.

## Recovery1 native review

Restored Android synchronous library preparation, event pumping/progress and driver-cache checkpointing based on preserved Test9 disassembly. Restored the LO_PIPELINE_WORKERS limit for pack shader module creation. Removed the scene-load polling call newly introduced during reconstruction: preserved Test9 did not invoke it from the engine hook. This keeps the recovered test close to the successful native behavior rather than enabling an additional unvalidated hook. Dormant polling implementation remains available for later investigation.

Remaining machine-code differences include startup shader lookup/preparation, harmless configuration validation order, build-identification logging and Vulkan device setup. They are not proven equivalent. Recovery1 should retain the tested version63 DEX, assets and eight support libraries, change the rebuilt runtime and manifest version only, and require a phone regression check before release.
