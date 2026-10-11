# LO: Saltlord Edition source recovery

This checkpoint contains the recovered v61 foundation and an incomplete Test9 reconstruction. It is not proven to match the tested beta APK. No public push or release occurred.

Workspace: /workspace/scratch/ce0bac179dd2/recovered-v61-source
Foundation: recovery-v61-foundation at d1ae779e2424ebedaa010d6a2fc68e3f270e74c3
Reconstruction: reconstruct-test9-v0922 at b3868e842f72a7c0688bea74c25354f4c69cc37b
Public base: 89317cf6e858baed3e543bd9e96f106f4f26ffbf
Upstream pin: d3888a0d879a23ff26197a3ad29fe64975b4b6b8 (v0.9.22)
Plume pin: d890ac899e505fb30040e037a4037cdeca68f033
Original missing Test9 commit: 8843e7549ee5844e00c665df23b429077c88f8bf

The ZIP contains tracked source, exact 3974-record corpus and integrated Plume patch. Pinned submodules must be initialized separately. Keys, SDK/NDK, compiled objects and user game files are excluded. The Git bundle requires the public base commit.

## Upload both branches

Keep the bundle beside a new checkout on a machine with Git and authenticated GitHub access:

```bash
git clone https://github.com/Saltlordd/LO-SaltlordEdition.git
cd LO-SaltlordEdition
git fetch ../LO-Saltlord-Recovery-Branches.bundle refs/heads/recovery-v61-foundation:refs/heads/recovery-v61-foundation refs/heads/reconstruct-test9-v0922:refs/heads/reconstruct-test9-v0922
git push origin recovery-v61-foundation reconstruct-test9-v0922
```

Do not tag this reconstruction as release source. Git credentials here were unavailable; the connected GitHub route returned403.

## Verification

Production Java compile and D8 passed. Native recipe tests passed normalization, v1 migration, scenes, corruption/truncation, roundtrips, limits and atomic writes. Generic/Android cache tests passed driver identity and preservation. Java corpus checks passed separate installation and preservation of learned files, saves and driver caches. Integrated Plume patch replay matched all10 modified files on its clean pinned base.

Full native runtime compilation/linking has NOT been performed. Motion-replay dependency and verbatim final copy remain incomplete. See docs/TEST9_SOURCE_RECOVERY.md inside the ZIP. Keep the tested APK unchanged; no replacement APK was produced.

## Focused tests

```bash
python tools/tests/pipeline_corpus_recovery_test.py
g++ -std=c++20 -pthread -I LostOdysseyRecomp tools/tests/pipeline_cache_test.cpp -o /tmp/lo-recipes
mkdir -p /tmp/lo-recipe-test
/tmp/lo-recipes /tmp/lo-recipe-test
g++ -std=c++20 -pthread -I LostOdysseyRecomp tools/tests/driver_pipeline_cache_test.cpp -o /tmp/lo-driver-cache
/tmp/lo-driver-cache
```

For Plume, apply ONLY tools/patches/plume-android-v0922-integrated.patch on the pinned clean base; do not apply the old Plume patch chain too.

Native rebuild requires NDKr29 and pinned dependency headers. Test9-PRIVATE-Compiled-Recovery.zip preserves runtime-commands.json, build-native.py, compiled support and generated PPC archive. Remap helper paths to the recovered checkout, compile/link all runtime units and compare with preserved Test9 objects/symbols. Phase24 private kit preserves SDK/SDL Java and signer. Do not publish either private archive. No fully reproducible end-to-end build claim.

Tested version63 APK SHA256: ce98db7ea326fd29fce049ad5ed9bcbe2ca3c006424aa7f5204b9e305ae3a655 (145945848 bytes).

Integration by Saltlord with AI assistance. Credit freefrank and upstream contributors.
