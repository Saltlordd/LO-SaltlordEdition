package io.github.freefrank.lostodyssey;
import android.app.AlertDialog;import android.content.Context;import android.widget.*;
final class GraphicsChoices {
 static final String HELP="Dynamic 60 targets up to 60 FPS; Steady 30 targets consistent pacing. SMAA smooths edges; FXAA gives a softer image. Lower resolution or turn AA off if gameplay slows.";
 static int[] current(){return new int[]{RuntimeReadinessActivity.nativeRenderResolution(0),RuntimeReadinessActivity.nativeFrameRate(0),RuntimeReadinessActivity.nativeAntiAliasing(-1)};}
 static void choose(Context context,int h,int fps,int aa,boolean preset,Runnable changed){int[] old=current();String warning="";
  if(h==1080&&fps==60&&(old[0]!=h||old[1]!=fps))warning="1080p with Dynamic 60 adds substantial workload. Try 720p or Steady 30 if gameplay slows.";
  if(h==1080&&aa!=0&&(old[0]!=h||old[2]!=aa))warning+=(warning.isEmpty()?"":"\n\n")+"Anti-aliasing at 1080p adds workload. Performance varies by device and scene.";
  Runnable apply=()->{if(preset){MobilePresentation.choose(context,"rgb",1);RuntimeReadinessActivity.nativeGraphicsPreset(h,aa);}else{RuntimeReadinessActivity.nativeRenderResolution(h);RuntimeReadinessActivity.nativeAntiAliasing(aa);}RuntimeReadinessActivity.nativeFrameRate(fps);changed.run();};
  if(warning.isEmpty()){apply.run();return;}AlertDialog dialog=new AlertDialog.Builder(context).setTitle("Demanding graphics combination").setMessage(warning).setNegativeButton("Cancel",null).setPositiveButton("Use anyway",(d,n)->apply.run()).create();dialog.show();BrandUi.finishDialog(dialog);
 }
 static void selectAa(Context c,Runnable changed){String[] names={"Off","SMAA — clearer edges","FXAA — softer image"};int[] ids={0,2,1};int a=current()[2],selected=a==2?1:a==1?2:0;AlertDialog d=new AlertDialog.Builder(c).setTitle("Anti-aliasing").setSingleChoiceItems(names,selected,(dialog,n)->{dialog.dismiss();int[] now=current();choose(c,now[0],now[1],ids[n],false,changed);}).setNegativeButton("Back",null).create();d.show();BrandUi.finishDialog(d);}
 static void presets(Context c,LinearLayout panel,Runnable changed){String[] names={"Low-spec","Balanced","Performance","Quality"};int[][] choices={{720,30,0},{720,60,0},{720,60,2},{1080,30,2}};
  TextView heading=new TextView(c);heading.setText("Quick performance profiles");heading.setTextColor(BrandUi.PRIMARY);panel.addView(heading);
  for(int i=0;i<names.length;i++){final int index=i;Button b=new Button(c);int[] p=choices[i];b.setText(names[i]+" · "+p[0]+"p / "+(p[1]==60?"Dynamic 60":"Steady 30")+" / "+(p[2]==2?"SMAA":"AA Off")+(i>=2?" · Higher-end":""));b.setOnClickListener(v->choose(c,p[0],p[1],p[2],true,changed));panel.addView(b);}
  BrandUi.help(panel,"Performance profiles","Low-spec is the conservative default. Performance and Quality suit more capable devices. Profiles use Expanded RGB; you can customise settings afterwards. Results vary by device and scene.");
 }
}
