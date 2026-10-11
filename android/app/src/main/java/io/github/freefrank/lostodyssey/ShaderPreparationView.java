package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.view.Gravity;
import android.widget.*;
/** Native progress display; no independent compiler or simulated progress. */
final class ShaderPreparationView extends FrameLayout {
 static final String WARNING="Prepare shaders before your first game, then restart the app. This may take a while.\n\nMinimising is allowed; keep the app running. A chime signals completion. Updates or driver changes may need more preparation.\n\nSome first-encounter hitches can remain.";
 private final TextView detail,title,note;private final ShaderProgressBar bar;private final PreparationDisplayPolicy display=new PreparationDisplayPolicy();private final LinearLayout content;private final ImageView logo;private final LinearLayout.LayoutParams barLayout;
 ShaderPreparationView(Context c){super(c);setBackground(BrandUi.backdrop(c));setClickable(true);setFocusable(true);
  ScrollView scroll=new ScrollView(c);scroll.setFillViewport(true);addView(scroll,new LayoutParams(-1,-1));
  content=new LinearLayout(c);content.setOrientation(LinearLayout.VERTICAL);content.setGravity(Gravity.CENTER);int pad=BrandUi.dp(c,28);content.setPadding(pad,pad,pad,pad);scroll.addView(content,new ScrollView.LayoutParams(-1,-2));
  logo=new ImageView(c);logo.setImageBitmap(BrandUi.load(c,"lo_saltlord_logo_full.png"));logo.setScaleType(ImageView.ScaleType.FIT_CENTER);logo.setContentDescription("LO: Saltlord Edition");content.addView(logo,new LinearLayout.LayoutParams(-1,BrandUi.dp(c,112)));
  title=new TextView(c);title.setText("Preparing shaders");title.setTextSize(26);title.setTextColor(BrandUi.PRIMARY);title.setGravity(Gravity.CENTER);content.addView(title);
  detail=new TextView(c);detail.setTextSize(17);detail.setTextColor(BrandUi.ACCENT);detail.setGravity(Gravity.CENTER);detail.setPadding(0,pad/2,0,pad/2);content.addView(detail);
  bar=new ShaderProgressBar(c);bar.setMax(1000);bar.setProgressTintList(ColorStateList.valueOf(BrandUi.ACCENT));bar.setProgressBackgroundTintList(ColorStateList.valueOf(0xff344552));barLayout=new LinearLayout.LayoutParams(-1,BrandUi.dp(c,14));barLayout.setMargins(pad,0,pad,pad);content.addView(bar,barLayout);
  note=new TextView(c);note.setText("Keep the app running until the chime, then restart. You can minimise and check progress in notifications, if allowed. Some first-encounter hitches can remain.");title.setVisibility(GONE);detail.setVisibility(GONE);note.setVisibility(GONE);note.setTextSize(16);note.setTextColor(BrandUi.SECONDARY);note.setGravity(Gravity.CENTER);content.addView(note);
  if(c instanceof RuntimeReadinessActivity)BrandUi.reportShortcut(content,()->((RuntimeReadinessActivity)c).saveDiagnosticReport());

 }
 // Measure the visible content, not just the display height: text wrapping,
 // font scale and the smaller Flex viewport all affect whether it fits.
 @Override protected void onMeasure(int widthSpec,int heightSpec){
  int available=MeasureSpec.getSize(heightSpec)-getPaddingTop()-getPaddingBottom();
  if(available<=0||MeasureSpec.getMode(heightSpec)==MeasureSpec.UNSPECIFIED){super.onMeasure(widthSpec,heightSpec);return;}
  float density=getResources().getDisplayMetrics().density;
  float heightDp=available/density;
  float base=heightDp<360?0.85f:heightDp<500?0.95f:1f;
  // Keep normal sizing when it fits. Reduce decoration and spacing first,
  // then typography modestly. Large accessibility text can still scroll.
  for(int pass=0;pass<4;pass++){
   float scale=Math.max(0.75f,base-pass*0.05f);
   int pad=BrandUi.dp(getContext(),Math.max(8,Math.round((heightDp<500?16:28)-pass*4)));
   content.setPadding(pad,pad,pad,pad);
   android.view.ViewGroup.LayoutParams lp=logo.getLayoutParams();
   int logoHeight=Math.max(BrandUi.dp(getContext(),36),Math.min(BrandUi.dp(getContext(),112),available/5)-BrandUi.dp(getContext(),pass*8));
   if(lp.height!=logoHeight){lp.height=logoHeight;logo.setLayoutParams(lp);}
   textSize(title,26*scale);textSize(detail,17*scale);textSize(note,16*scale);
   detail.setPadding(0,Math.max(BrandUi.dp(getContext(),4),pad/2),0,Math.max(BrandUi.dp(getContext(),4),pad/2));
   barLayout.height=BrandUi.dp(getContext(),heightDp<360?10:14);
   barLayout.setMargins(pad,0,pad,Math.max(BrandUi.dp(getContext(),8),pad));
   super.onMeasure(widthSpec,heightSpec);
   if(content.getMeasuredHeight()<=available)break;
  }
 }
 private void textSize(TextView view,float sp){
  float expected=sp*getResources().getDisplayMetrics().scaledDensity;
  if(Math.abs(view.getTextSize()-expected)>0.1f)view.setTextSize(sp);
 }
 void update(long state){boolean full=display.detailed(state);title.setVisibility(full?VISIBLE:GONE);detail.setVisibility(full?VISIBLE:GONE);note.setVisibility(full?VISIBLE:GONE);bar.setShimmer(display.shimmer(state));int done=(int)(state&0xfffffffL),total=(int)((state>>>28)&0xfffffffL);int stage=(int)((state>>>56)&15),unit=(int)(state>>>60);
  String[] stages={"Compiling shaders","Preparing pipelines","Checking the cache","Extracting shaders","Scanning game files","Loading cached shaders"},units={"shaders","pipelines","files","MiB","entries"};
  bar.setIndeterminate(total==0);if(total>0)bar.setProgress((int)(1000L*Math.min(done,total)/total));
  if(state==0){detail.setText("Finishing preparation…");return;}
  detail.setText((stage<stages.length?stages[stage]:"Preparing game resources")+"\n"+(total>0?String.format(java.util.Locale.US,"%,d / %,d %s · %d%%",done,total,unit<units.length?units[unit]:"items",100L*Math.min(done,total)/total):"Please wait…"));
 }
}
