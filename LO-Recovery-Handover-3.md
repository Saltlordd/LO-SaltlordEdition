# Saltlord recovery handover — native rebuild checkpoint

Source commit: 30a03f44dc8ab2a6889c7273350fba958f56e8dd
Branch: reconstruct-test9-v0922
Foundation: d1ae779e2424ebedaa010d6a2fc68e3f270e74c3 (recovery-v61-foundation)
Bundle prerequisite: public base 89317cf6e858baed3e543bd9e96f106f4f26ffbf
Pinned upstream v0.9.22: d3888a0d879a23ff26197a3ad29fe64975b4b6b8

## Completed

All 88 runtime units and Plume Vulkan backend compile with NDK r29; runtime links successfully. Preserved game-code/support archives are reused. PPCContext matches 206 member offsets, size and alignment; settings Config matches all 29 offsets. 84 of 89 objects have identical executable-section bytes to preserved Test9. The remaining objects contain 17 changed shared sections plus added/removed sections. This comparison excludes relocations and data and does not prove behavioral equivalence.

The 11-file integrated Plume patch now replays exactly on its pinned base, including the missing logging header. Cache APIs, battle-status dependency, motion replay dependency and portable-pack paths/contracts compile. Shared renderer owns driver-cache persistence. Unused prefetch field removed to restore member layout.

Wizard/options/report polish is recovered from preserved APK strings: 37 long literals, Performance heading, removed layout paragraph and shorter report-success message. Java compilation and D8 pass. Recipe/corpus/driver-cache tests passed during recovery; corpus preservation test rerun after final copy restoration.

Runtime SHA256: daad8cdded45dcd64ef150260efa22c8f1ddfd115780c248ac42a2587cf23c33
Runtime bytes: 317376816
Stripped runtime bytes: 86823928
Stripped runtime SHA256: 6c3bc90a87ac09d169073713f07c8af07266da231ab171055f3387cd7f6953ba
ARM64 ELF load alignment: 16 KiB.

## Still required

Review remaining native differences against Test9, recover original signing key privately, create a recovery test APK with the original certificate, then test on the phone. Original commit 8843e7549ee5844e00c665df23b429077c88f8bf remains missing. No newly signed APK or phone validation from this checkpoint. Never invent binary equivalence or generate a replacement signing key.

Keep tested version63 APK unchanged: SHA256 ce98db7ea326fd29fce049ad5ed9bcbe2ca3c006424aa7f5204b9e305ae3a655. Its successful performance remains the reference.

## Files

LO-Saltlord-Recovered-Source.zip: public source, exact3974-record corpus, integrated Plume patch. Initialize pinned submodules separately; apply ONLY the integrated Plume patch on this branch.
LO-Saltlord-Recovery-Branches.bundle: foundation and updated reconstruction refs; valid against public base above. The user's existing source-recovery-backup branch contains the earlier b3868e checkpoint, not this newer commit. Public push remains unavailable in this session.
LO-Saltlord-Native-Rebuild-PRIVATE.zip: private generated headers, game-symbol declarations, rebuilt objects/runtime, scripts/logs/comparisons. DO NOT upload this private file to the public repository. It reuses the separately preserved Test9 compiled support/PPC archive and fetched pinned dependencies. It contains no signing key.
LO-Recovery-SHA256SUMS.txt: checksums.

Before beta release, retain verified source in GitHub and keep private build inputs securely backed up. Saltlord only in public material. AI-assisted integration; credit freefrank and contributors.
