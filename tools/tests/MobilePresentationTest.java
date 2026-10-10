package io.github.freefrank.lostodyssey;
public final class MobilePresentationTest {
 static void check(boolean ok){if(!ok)throw new AssertionError();}
 public static void main(String[] args){
  String before="frame_rate=60\nmotion_blur=0\ndynamic_shadows=1\ngame_language=2\nscaling_quality=1\nexpand_rgb_range=1\ninternal_resolution=1080\nantialiasing=2\nanisotropic_filtering=16\nupscaler=3\nframe_generation_provider=1\nvariable_refresh_rate=1\n";
  String after=MobilePresentation.settings(before,0,1);
  check(after.contains("frame_rate=60\n")&&after.contains("motion_blur=0\n")&&after.contains("dynamic_shadows=1\n")&&after.contains("game_language=2\n"));
  check(after.contains("scaling_quality=0\n")&&after.contains("expand_rgb_range=1\n"));
  check(after.contains("internal_resolution=1080\n")&&after.contains("antialiasing=2\n")&&after.contains("upscaler=0\n")&&after.contains("frame_generation_provider=0\n"));
  check(after.equals(MobilePresentation.settings(after,0,1)));
  check(MobilePresentation.settings(after,1,0).contains("scaling_quality=0\n"));
  check(MobilePresentation.settings("scaling_quality=1\nscaling_quality=1\n",0,0).split("scaling_quality=",-1).length==2);
  boolean invalid=false;try{MobilePresentation.settings(after,2,0);}catch(IllegalArgumentException e){invalid=true;}check(invalid);
  int[][] profiles={{720,30,0},{720,60,0},{720,60,2},{1080,30,2}};
  for(int[] p:profiles){String profile="internal_resolution="+p[0]+"\nframe_rate="+p[1]+"\nantialiasing="+p[2]+"\n";int[] restored=SavedGraphics.parse(MobilePresentation.settings(profile,0,1));check(restored[0]==p[0]&&restored[1]==p[1]&&restored[2]==p[2]);}
  String fxaa=MobilePresentation.settings("internal_resolution=1080\nframe_rate=60\nantialiasing=1\n",0,1);check(fxaa.contains("fxaa=1\n")&&SavedGraphics.parse(fxaa)[1]==60);
  int[] defaults=SavedGraphics.parse(MobilePresentation.settings("",0,1));check(defaults[0]==720&&defaults[1]==30&&defaults[2]==0);
  System.out.println("PASS: all four profile tuples survive launch preparation; FXAA and confirmed 1080p/60 survive; conservative fresh defaults, fixed Basic output, effects/language preservation and idempotence");
 }
}
