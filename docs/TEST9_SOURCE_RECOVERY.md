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

Production Android Java compile against SDK35 and D8 passed. Recipe tests passed normalization, v1 migration, scene tags, corruption/truncation, exact roundtrip and atomic writes. Generic/Android driver cache tests passed identity and preservation. Java corpus checks passed 3974 records, corruption/schema/truncation rejection, separate installation, learned v2/opaque-file preservation, saves/driver-cache preservation and repeat installation. Integrated Plume patch replay byte-matched all 10 modified files on base d890ac899e505fb30040e037a4037cdeca68f033. Diff whitespace checks passed.

## Not yet complete

The reconstructed native runtime has NOT been compiled or linked. Motion replay #376 dependency fix still needs conflict resolution (inactive under ordinary Android Off/SMAA). Final shortened wizard/options/report strings have not been recovered verbatim. This checkpoint does not establish native equivalence, APK equivalence, full-game/device compatibility or reproducible end-to-end builds. The older warmup Java test targets the superseded v1 merge behavior; use pipeline_corpus_recovery_test.py for this branch.

Do not build/publish a beta from this checkpoint without native validation. Keep the excellent tested version63 APK unchanged: SHA256 ce98db7ea326fd29fce049ad5ed9bcbe2ca3c006424aa7f5204b9e305ae3a655, 145945848 bytes. Corpus SHA256 5ced4aa19e9326ba1ad9a8ab695d94d720feef173ec2d68b16700d9abccb5c18.

## Plume preparation

On this reconstruction branch, check out Plume d890ac899e505fb30040e037a4037cdeca68f033 and apply ONLY tools/patches/plume-android-v0922-integrated.patch. Do not apply the old plume patch chain as well. Other dependency pins/patches remain from the restored foundation.

## Next steps

Recover NDK r29 and the pinned dependency headers privately. Use preserved Test9 runtime-commands.json/build-native.py as the compilation specification, remapping source/build/toolchain paths; preserve generated PPC objects unchanged. Compile/link the runtime, resolve errors, and compare against preserved objects/symbols and APK. Resolve inactive replay dependency and exact final copy. Re-run Android Java/D8 and native tests after changes. Secure verified matching source before releasing beta0.1.1.

No signing secrets, compiled recovery objects or user game files belong in the source archive. Integration by Saltlord with AI assistance; upstream freefrank and contributors retain their attribution/licences.
