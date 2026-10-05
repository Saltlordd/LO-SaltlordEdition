# Cutscene Tour — October 2026

[简体中文](CUTSCENE_TOUR.zh-CN.md)

![One frame from each of the 178 toured cutscenes](images/cutscene-tour/mosaic.webp)

On 4–5 October 2026, three machines played every real-time cutscene in Lost Odyssey they could reach, with temporal anti-aliasing and upscaling on, to look for shadow and jitter defects. A person reviewed the screenshots. Diagnostics in the renderer ranked what to look at first.

## In numbers

| | |
|---|---|
| Real-time cutscenes found in the game data | 195 event packages on 4 discs |
| Cutscenes reached and played | 178 |
| Machines in parallel | 3: NVIDIA on Direct3D 12, AMD on Direct3D 12 under Proton, Apple on Metal |
| Playback recorded | 7.2 hours, in about 4.5 hours of wall time |
| Screenshots taken | 14,253 |
| Shader pairs flagged / reviewed line by line | 47 / 45 |
| Rendering fixes shipped from it | 2 pull requests ([#218](https://github.com/freefrank/LostOdysseyRecomp/pull/218), [#224](https://github.com/freefrank/LostOdysseyRecomp/pull/224)) |

| Machine | Settings | Discs | Median FPS | Slowest 10 % |
|---|---|---|---|---|
| PC, RTX 5080 | Direct3D 12, DLSS Balanced + TAA + AO, 1920×1080 | 1 | 59.9 | 59.3 |
| psvita, Radeon 8060S | Direct3D 12 under Proton, FSR + TAA + AO, 1920×1080 | 3 | 58.2 | 30.0 |
| MacBook Pro, M1 Max | Metal, MetalFX + TAA + AO, 1600×900 | 2, 4 | 59.5 | 43.8 |

The frame rates come from the screenshot timestamps, with the game window in the background and a screenshot every two seconds. They compare the machines, not a benchmark.

## How it worked

1. **Find the cutscenes.** The disc index (`LO.fpi`) lists every real-time event package. The game's own Scenario Jump debug menu reaches them, 14 entries per page over 36 pages. OCR of the menu tied 178 packages to a page and row.
2. **Drive the game unattended.** A small driver jumps to the event-debug map, requests the right disc and opens Scenario Jump. It finds the arrow cursor on screen and reads the row with OCR before pressing A, then plays the cutscene for up to four minutes. It stops early when the field HUD returns or a battle starts. Windows OCR ran on the PC and tesseract on Linux and macOS.
3. **Collect two kinds of evidence.**
   - **Pictures.** A screenshot every ~2 s, laid out in numbered contact sheets. The maintainer looked at every cutscene this way.
   - **Renderer diagnostics.** Every 60 frames the renderer logged draws that should have received the sub-pixel jitter but did not, and why: camera mismatch, depth mismatch or an unknown shader.
4. **Review.** The 47 shader pairs the diagnostics flagged were checked line by line against their translated HLSL, three reviews in parallel. The question for each: can this pass move with the jittered pixel without breaking anything?

## What it found

- **Block shadows in a cutscene (#212).** In RT_084B, Jansen's close-up draws the scene twice with two cameras. The second view's shadow volumes, shadow projections and lights compared against the first camera and stayed unjittered, which covered faces in flickering blocks. This was the bug that started the tour. The renderer now tracks each scene camera in the frame ([#218](https://github.com/freefrank/LostOdysseyRecomp/pull/218)).
- **Flickering battle floor light (#212).** Per-light floor passes in the Old Sorceress' Mansion battles were not mapped, so their light dropped out pixel by pixel ([#218](https://github.com/freefrank/LostOdysseyRecomp/pull/218)).
- **45 more unmapped passes** ([#224](https://github.com/freefrank/LostOdysseyRecomp/pull/224)):
  - 30 per-light passes;
  - 10 depth and material passes;
  - 5 skinned character passes used throughout the cutscenes.
  Each was up to half a pixel off its depth, and an unmapped depth writer stops motion-vector replay for the frame it appears in. All 45 were reviewed and mapped as exact pairs. Reruns on all three machines showed none of them unjittered.
- **A crash after RT_099B.** The game reads a null pointer while loading the world map after this cutscene, on both the PC and the Mac. It is most likely state that the debug jump skips, and no player has reported it.
- **No visible shadow defect left** in the 178 contact sheets. One remaining diagnostic (`702c`, one draw per frame in eight cutscenes) showed nothing on screen.

Technical details: [cutscene tour notes](notes/jitter-cutscene-tour-2026-10-04.md) and [#212 notes](notes/jitter-212-2026-10-04.md).

## Every cutscene

Each sheet shows every third screenshot (about one every six seconds); the number on a tile is the screenshot's index in that cutscene. Disc 1 was recorded on the PC, disc 3 on psvita, discs 2 and 4 on the Mac. RT_099B is the PC run.

<details>
<summary><b>Disc 1 · 60 cutscenes</b></summary>


**RT_003B · Kaim Discovered**

![rt_003b](images/cutscene-tour/rt_003b.webp)


**RT_005A · Seth Appears**

![rt_005a](images/cutscene-tour/rt_005a.webp)


**RT_006B**

![rt_006b](images/cutscene-tour/rt_006b.webp)


**RT_008B**

![rt_008b](images/cutscene-tour/rt_008b.webp)


**RT_009B**

![rt_009b](images/cutscene-tour/rt_009b.webp)


**RT_011_1B · Monorail Departs**

![rt_011_1b](images/cutscene-tour/rt_011_1b.webp)


**RT_013B · Kaim Looks out the Window**

![rt_013b](images/cutscene-tour/rt_013b.webp)


**RT_014B**

![rt_014b](images/cutscene-tour/rt_014b.webp)


**RT_015B**

![rt_015b](images/cutscene-tour/rt_015b.webp)


**RT_016_1B · Kaim's Questioning**

![rt_016_1b](images/cutscene-tour/rt_016_1b.webp)


**RT_016_3B**

![rt_016_3b](images/cutscene-tour/rt_016_3b.webp)


**RT_017A · First Dream**

![rt_017a](images/cutscene-tour/rt_017a.webp)


**RT_018A · Jansen Appears**

![rt_018a](images/cutscene-tour/rt_018a.webp)


**RT_019C · Gongora and Tolten**

![rt_019c](images/cutscene-tour/rt_019c.webp)


**RT_020B**

![rt_020b](images/cutscene-tour/rt_020b.webp)


**RT_021B · Gongora's Thoughtful Face**

![rt_021b](images/cutscene-tour/rt_021b.webp)


**RT_022B · Gongora and Kaim**

![rt_022b](images/cutscene-tour/rt_022b.webp)


**RT_023_1A · Seth in the Park**

![rt_023_1a](images/cutscene-tour/rt_023_1a.webp)


**RT_023_2A · Departure**

![rt_023_2a](images/cutscene-tour/rt_023_2a.webp)


**RT_024B · Start in Ipsilon**

![rt_024b](images/cutscene-tour/rt_024b.webp)


**RT_026B · Night in the Small Mountain Hut**

![rt_026b](images/cutscene-tour/rt_026b.webp)


**RT_027B · Rain Starts to Fall**

![rt_027b](images/cutscene-tour/rt_027b.webp)


**RT_028A · Tolten's Meal**

![rt_028a](images/cutscene-tour/rt_028a.webp)


**RT_029B · Mountain Peak**

![rt_029b](images/cutscene-tour/rt_029b.webp)


**RT_033B · Looking up at Grand Staff**

![rt_033b](images/cutscene-tour/rt_033b.webp)


**RT_034B · Bogimoray Appears**

![rt_034b](images/cutscene-tour/rt_034b.webp)


**RT_035B · Numara Soldiers in the Hollow**

![rt_035b](images/cutscene-tour/rt_035b.webp)


**RT_036B · Lighting the Fuse**

![rt_036b](images/cutscene-tour/rt_036b.webp)


**RT_037B**

![rt_037b](images/cutscene-tour/rt_037b.webp)


**RT_038_1B · Are You Scared?**

![rt_038_1b](images/cutscene-tour/rt_038_1b.webp)


**RT_038_3B**

![rt_038_3b](images/cutscene-tour/rt_038_3b.webp)


**RT_040A · Memory Comes Back**

![rt_040a](images/cutscene-tour/rt_040a.webp)


**RT_041B**

![rt_041b](images/cutscene-tour/rt_041b.webp)


**RT_042C · Preparing Black Pearl**

![rt_042c](images/cutscene-tour/rt_042c.webp)


**RT_047A · The Past is Pointless**

![rt_047a](images/cutscene-tour/rt_047a.webp)


**RT_048B · I Like the Past**

![rt_048b](images/cutscene-tour/rt_048b.webp)


**RT_049B**

![rt_049b](images/cutscene-tour/rt_049b.webp)


**RT_051A · Jansen and Ming Meet**

![rt_051a](images/cutscene-tour/rt_051a.webp)


**RT_052B · Kaim and Seth Found**

![rt_052b](images/cutscene-tour/rt_052b.webp)


**RT_054A · Jansen Turned to Stone**

![rt_054a](images/cutscene-tour/rt_054a.webp)


**RT_055B**

![rt_055b](images/cutscene-tour/rt_055b.webp)


**RT_056A · Released in Philosopher's Room**

![rt_056a](images/cutscene-tour/rt_056a.webp)


**RT_057B · Ming Knocked Out**

![rt_057b](images/cutscene-tour/rt_057b.webp)


**RT_058A · The Twins**

![rt_058a](images/cutscene-tour/rt_058a.webp)


**RT_060B · Cavalry Flee**

![rt_060b](images/cutscene-tour/rt_060b.webp)


**RT_061A · Are You...**

![rt_061a](images/cutscene-tour/rt_061a.webp)


**RT_062B**

![rt_062b](images/cutscene-tour/rt_062b.webp)


**RT_063B**

![rt_063b](images/cutscene-tour/rt_063b.webp)


**RT_064A · Live, Lirum!**

![rt_064a](images/cutscene-tour/rt_064a.webp)


**RT_067B · Fake Roxian**

![rt_067b](images/cutscene-tour/rt_067b.webp)


**RT_068B · After Fake Roxian Battle**

![rt_068b](images/cutscene-tour/rt_068b.webp)


**RT_069B**

![rt_069b](images/cutscene-tour/rt_069b.webp)


**RT_070B**

![rt_070b](images/cutscene-tour/rt_070b.webp)


**RT_071_1B · Kaim Laughs**

![rt_071_1b](images/cutscene-tour/rt_071_1b.webp)


**RT_074B · Mack Brings Flower**

![rt_074b](images/cutscene-tour/rt_074b.webp)


**RT_076A · Funeral - Gondola to the Sea**

![rt_076a](images/cutscene-tour/rt_076a.webp)


**RT_077B · Mack's Letter**

![rt_077b](images/cutscene-tour/rt_077b.webp)


**RT_078B · Armoring of the White Boa**

![rt_078b](images/cutscene-tour/rt_078b.webp)


**RT_081B · Obsidian Aura Appears**

![rt_081b](images/cutscene-tour/rt_081b.webp)


**RT_082A · After Obsidian Aura Battle (from RT_81)**

![rt_082a](images/cutscene-tour/rt_082a.webp)

</details>

<details>
<summary><b>Disc 2 · 28 cutscenes</b></summary>


**RT_084B · Tolten Becomes 45th King**

![rt_084b](images/cutscene-tour/rt_084b.webp)


**RT_086B · Taken to Temple**

![rt_086b](images/cutscene-tour/rt_086b.webp)


**RT_087B · Suspicion of Spying**

![rt_087b](images/cutscene-tour/rt_087b.webp)


**RT_088B · Cooke and Mack to the Rescue**

![rt_088b](images/cutscene-tour/rt_088b.webp)


**RT_089A · Capturing Ming**

![rt_089a](images/cutscene-tour/rt_089a.webp)


**RT_090B · Escape with Ming**

![rt_090b](images/cutscene-tour/rt_090b.webp)


**RT_091B · Kakanas' Magic Tank**

![rt_091b](images/cutscene-tour/rt_091b.webp)


**RT_092B · Caterpillar Destroyed**

![rt_092b](images/cutscene-tour/rt_092b.webp)


**RT_095A · Slantnose Activated**

![rt_095a](images/cutscene-tour/rt_095a.webp)


**RT_096_2B**

![rt_096_2b](images/cutscene-tour/rt_096_2b.webp)


**RT_099B · Arthrosaurus Visible**

![rt_099b](images/cutscene-tour/rt_099b.webp)


**RT_101_1B · Arriving at Southernmost Cape**

![rt_101_1b](images/cutscene-tour/rt_101_1b.webp)


**RT_101_2B**

![rt_101_2b](images/cutscene-tour/rt_101_2b.webp)


**RT_104B · Jansen and Ming in the Bar**

![rt_104b](images/cutscene-tour/rt_104b.webp)


**RT_105C · Rumors of the Old Sorcerer**

![rt_105c](images/cutscene-tour/rt_105c.webp)


**RT_107B · (From RT_106)**

![rt_107b](images/cutscene-tour/rt_107b.webp)


**RT_108B · (From RT_106)**

![rt_108b](images/cutscene-tour/rt_108b.webp)


**RT_109B**

![rt_109b](images/cutscene-tour/rt_109b.webp)


**RT_110B**

![rt_110b](images/cutscene-tour/rt_110b.webp)


**RT_117C · Seal on Black Cave Removed**

![rt_117c](images/cutscene-tour/rt_117c.webp)


**RT_119B · Armor of Gohtza**

![rt_119b](images/cutscene-tour/rt_119b.webp)


**RT_122B · Knock Down the Pillar?**

![rt_122b](images/cutscene-tour/rt_122b.webp)


**RT_123B · Deporting Saman**

![rt_123b](images/cutscene-tour/rt_123b.webp)


**RT_127B · Finding Gongora**

![rt_127b](images/cutscene-tour/rt_127b.webp)


**RT_129B · Moaning Gongora**

![rt_129b](images/cutscene-tour/rt_129b.webp)


**RT_130A · Before Councilor Gongora Battle**

![rt_130a](images/cutscene-tour/rt_130a.webp)


**RT_131B**

![rt_131b](images/cutscene-tour/rt_131b.webp)


**RT_132A · Recovery by Petal**

![rt_132a](images/cutscene-tour/rt_132a.webp)

</details>

<details>
<summary><b>Disc 3 · 56 cutscenes</b></summary>


**RT_135B · Welcome in Saman**

![rt_135b](images/cutscene-tour/rt_135b.webp)


**RT_138B · Before Dinozaoro Battle**

![rt_138b](images/cutscene-tour/rt_138b.webp)


**RT_140B · Man in a Hood**

![rt_140b](images/cutscene-tour/rt_140b.webp)


**RT_141B · Birthmark on Ming's Chest**

![rt_141b](images/cutscene-tour/rt_141b.webp)


**RT_142B · Before the Substitute's Throne**

![rt_142b](images/cutscene-tour/rt_142b.webp)


**RT_145C · Substitute Exposed**

![rt_145c](images/cutscene-tour/rt_145c.webp)


**RT_146_1A · Marth... King of Gohtza**

![rt_146_1a](images/cutscene-tour/rt_146_1a.webp)


**RT_146_2A**

![rt_146_2a](images/cutscene-tour/rt_146_2a.webp)


**RT_147C · Through the Ticket Gate**

![rt_147c](images/cutscene-tour/rt_147c.webp)


**RT_149_2B**

![rt_149_2b](images/cutscene-tour/rt_149_2b.webp)


**RT_150B · Magic Train Departs**

![rt_150b](images/cutscene-tour/rt_150b.webp)


**RT_151B · Substitute Carrots and Sticks**

![rt_151b](images/cutscene-tour/rt_151b.webp)


**RT_153_1B · Cooke and Mack's Steps**

![rt_153_1b](images/cutscene-tour/rt_153_1b.webp)


**RT_153_2B**

![rt_153_2b](images/cutscene-tour/rt_153_2b.webp)


**RT_155B · Gohtza, Tolten and Ming**

![rt_155b](images/cutscene-tour/rt_155b.webp)


**RT_156B**

![rt_156b](images/cutscene-tour/rt_156b.webp)


**RT_157B**

![rt_157b](images/cutscene-tour/rt_157b.webp)


**RT_158B · Kaim and Sarah Highjack the Train**

![rt_158b](images/cutscene-tour/rt_158b.webp)


**RT_162C · Pirate Hunter's and Other Stories**

![rt_162c](images/cutscene-tour/rt_162c.webp)


**RT_163A · Gongora Speech**

![rt_163a](images/cutscene-tour/rt_163a.webp)


**RT_164B · Picking on the Dark Saints**

![rt_164b](images/cutscene-tour/rt_164b.webp)


**RT_165A · Not Loyal Enough**

![rt_165a](images/cutscene-tour/rt_165a.webp)


**RT_166C · Trouble for Gohtzan Royal Carriage**

![rt_166c](images/cutscene-tour/rt_166c.webp)


**RT_167A**

![rt_167a](images/cutscene-tour/rt_167a.webp)


**RT_168A**

![rt_168a](images/cutscene-tour/rt_168a.webp)


**RT_169B · Gongora's Ice Fragment**

![rt_169b](images/cutscene-tour/rt_169b.webp)


**RT_170B**

![rt_170b](images/cutscene-tour/rt_170b.webp)


**RT_171C · Finding Cooke and Mack on the Train**

![rt_171c](images/cutscene-tour/rt_171c.webp)


**RT_172A · Cooke and Mack Open Their Eyes**

![rt_172a](images/cutscene-tour/rt_172a.webp)


**RT_176A**

![rt_176a](images/cutscene-tour/rt_176a.webp)


**RT_177C · Crying Cooke, Staring Mack**

![rt_177c](images/cutscene-tour/rt_177c.webp)


**RT_178B · Warp Out in Uhra**

![rt_178b](images/cutscene-tour/rt_178b.webp)


**RT_181B · Magic Monitor in Castle Town**

![rt_181b](images/cutscene-tour/rt_181b.webp)


**RT_182A · Bridge's Magic Monitor**

![rt_182a](images/cutscene-tour/rt_182a.webp)


**RT_183B · Rousing Gohtza Troops**

![rt_183b](images/cutscene-tour/rt_183b.webp)


**RT_184A · Gongora's Inauguration, Sed's Sacrifice**

![rt_184a](images/cutscene-tour/rt_184a.webp)


**RT_185A · Saving Sed**

![rt_185a](images/cutscene-tour/rt_185a.webp)


**RT_188A · Cook and Mack in the Snow**

![rt_188a](images/cutscene-tour/rt_188a.webp)


**RT_189C · Mack! Don't Fall Asleep!**

![rt_189c](images/cutscene-tour/rt_189c.webp)


**RT_191A · Jansen, Ming and the Bonfire**

![rt_191a](images/cutscene-tour/rt_191a.webp)


**RT_192A · A Ming's Recollections**

![rt_192a](images/cutscene-tour/rt_192a.webp)


**RT_193C · Jansen and Ming to the City**

![rt_193c](images/cutscene-tour/rt_193c.webp)


**RT_195C · Joining Hands and Jumping Rubble**

![rt_195c](images/cutscene-tour/rt_195c.webp)


**RT_197B · Cooke and Mack Collapse**

![rt_197b](images/cutscene-tour/rt_197b.webp)


**RT_199A · Cooke and Mack Healed**

![rt_199a](images/cutscene-tour/rt_199a.webp)


**RT_202B · Hole in the Gohtza Tank**

![rt_202b](images/cutscene-tour/rt_202b.webp)


**RT_205B · Before Magic Beast Battle**

![rt_205b](images/cutscene-tour/rt_205b.webp)


**RT_206_2**

![rt_206_2](images/cutscene-tour/rt_206_2.webp)


**RT_206B · Nautilus Launches**

![rt_206b](images/cutscene-tour/rt_206b.webp)


**RT_207B · Blocked Underwater**

![rt_207b](images/cutscene-tour/rt_207b.webp)


**RT_208B · Soaring Through the Sky**

![rt_208b](images/cutscene-tour/rt_208b.webp)


**RT_209B · High Five**

![rt_209b](images/cutscene-tour/rt_209b.webp)


**RT_210C · Drunkard's Information**

![rt_210c](images/cutscene-tour/rt_210c.webp)


**RT_211A · Before Ice Magic Beast Battle**

![rt_211a](images/cutscene-tour/rt_211a.webp)


**RT_212A · After Ice Magic Beast Battle**

![rt_212a](images/cutscene-tour/rt_212a.webp)


**RT_213B · Big Reunion**

![rt_213b](images/cutscene-tour/rt_213b.webp)

</details>

<details>
<summary><b>Disc 4 · 34 cutscenes</b></summary>


**RT_215B · Meeting with Mother**

![rt_215b](images/cutscene-tour/rt_215b.webp)


**RT_217B · Memory of the Hall of Mirrors**

![rt_217b](images/cutscene-tour/rt_217b.webp)


**RT_223B · Great Eastern Ruins**

![rt_223b](images/cutscene-tour/rt_223b.webp)


**RT_224C · Tolten is in Progress**

![rt_224c](images/cutscene-tour/rt_224c.webp)


**RT_225B · Before Ancient Monster Battle**

![rt_225b](images/cutscene-tour/rt_225b.webp)


**RT_226B · After Ancient Monster Battle**

![rt_226b](images/cutscene-tour/rt_226b.webp)


**RT_228C · Full Speed Ahead**

![rt_228c](images/cutscene-tour/rt_228c.webp)


**RT_232B · Kakanas Greeting**

![rt_232b](images/cutscene-tour/rt_232b.webp)


**RT_233B · Before Kakanas' Tank Battle**

![rt_233b](images/cutscene-tour/rt_233b.webp)


**RT_234_1B · After Kakanas' Tank Battle 1**

![rt_234_1b](images/cutscene-tour/rt_234_1b.webp)


**RT_234_2B · After Kakanas' Tank Battle 2**

![rt_234_2b](images/cutscene-tour/rt_234_2b.webp)


**RT_239A · Queen's Love**

![rt_239a](images/cutscene-tour/rt_239a.webp)


**RT_240B · Jansen Annoyed with Snickering Twins**

![rt_240b](images/cutscene-tour/rt_240b.webp)


**RT_242C · White Boa Charges In (From MV_46)**

![rt_242c](images/cutscene-tour/rt_242c.webp)


**RT_244B · Saints of Darkness Appear**

![rt_244b](images/cutscene-tour/rt_244b.webp)


**RT_245B · Before Saints of Darkness Battle**

![rt_245b](images/cutscene-tour/rt_245b.webp)


**RT_246B · After Saints of Darkness Battle**

![rt_246b](images/cutscene-tour/rt_246b.webp)


**RT_251B · Glass Room**

![rt_251b](images/cutscene-tour/rt_251b.webp)


**RT_252B · Before GS Mid-Boss Battle 2**

![rt_252b](images/cutscene-tour/rt_252b.webp)


**RT_253A · Before Dark Jansen Battle**

![rt_253a](images/cutscene-tour/rt_253a.webp)


**RT_255A · After Dark Jansen Battle**

![rt_255a](images/cutscene-tour/rt_255a.webp)


**RT_256A · Gongora to the Tower of Mirrors**

![rt_256a](images/cutscene-tour/rt_256a.webp)


**RT_257B · Got To Escape!**

![rt_257b](images/cutscene-tour/rt_257b.webp)


**RT_261B · Press On or There's No Future!**

![rt_261b](images/cutscene-tour/rt_261b.webp)


**RT_262A · Before Tower of Mirrors Magic Beast Battle**

![rt_262a](images/cutscene-tour/rt_262a.webp)


**RT_263A · Shut Out the Light**

![rt_263a](images/cutscene-tour/rt_263a.webp)


**RT_264A · After Last Gongora Battle B**

![rt_264a](images/cutscene-tour/rt_264a.webp)


**RT_269A · Seth, Let's Meet Again**

![rt_269a](images/cutscene-tour/rt_269a.webp)


**RT_270_1A · Epilogue**

![rt_270_1a](images/cutscene-tour/rt_270_1a.webp)


**RT_270_2A**

![rt_270_2a](images/cutscene-tour/rt_270_2a.webp)


**RT_270_3A**

![rt_270_3a](images/cutscene-tour/rt_270_3a.webp)


**RT_270_4A**

![rt_270_4a](images/cutscene-tour/rt_270_4a.webp)


**RT_270_5A**

![rt_270_5a](images/cutscene-tour/rt_270_5a.webp)


**RT_270_6A**

![rt_270_6a](images/cutscene-tour/rt_270_6a.webp)

</details>
