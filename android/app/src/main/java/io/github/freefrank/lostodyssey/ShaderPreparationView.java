package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.view.Gravity;
import android.widget.*;
/** Native progress display; no independent compiler or simulated progress. */
final class ShaderPreparationView extends FrameLayout {
 static final String WARNING="Shaders must be prepared before your first game. This can take a while, especially with all four discs.\n\nYou can minimise the app while preparation continues, but it may take longer. Keeping the app open can finish faster; progress appears in your notifications when allowed. Please don't close or kill the app. If interrupted, the next launch resumes from the available cache.\n\nA crystal chime confirms it's ready. Later launches reuse the cache; updates and driver changes may need more preparation. Brief, occasional hitches may still occur when new effects or graphics pipelines are encountered for the first time. Cached preparation can make later encounters smoother.";
 private final TextView detail,title,note;private final ShaderProgressBar bar;private final PreparationDisplayPolicy display=new PreparationDisplayPolicy();private final LinearLayout content;private final ImageView logo;
 ShaderPreparationView(Context c){super(c);setBackground(BrandUi.backdrop(c));setClickable(true);setFocusable(true);
  ScrollView scroll=new ScrollView(c);scroll.setFillViewport(true);addView(scroll,new LayoutParams(-1,-1));
  content=new LinearLayout(c);content.setOrientation(LinearLayout.VERTICAL);content.setGravity(Gravity.CENTER);int pad=BrandUi.dp(c,28);content.setPadding(pad,pad,pad,pad);scroll.addView(content,new ScrollView.LayoutParams(-1,-2));
  logo=new ImageView(c);logo.setImageBitmap(BrandUi.load(c,"lo_saltlord_logo_full.png"));logo.setScaleType(ImageView.ScaleType.FIT_CENTER);logo.setContentDescription("LO: Saltlord Edition");content.addView(logo,new LinearLayout.LayoutParams(-1,BrandUi.dp(c,112)));
  title=new TextView(c);title.setText("Preparing shaders");title.setTextSize(26);title.setTextColor(BrandUi.PRIMARY);title.setGravity(Gravity.CENTER);content.addView(title);
  detail=new TextView(c);detail.setTextSize(17);detail.setTextColor(BrandUi.ACCENT);detail.setGravity(Gravity.CENTER);detail.setPadding(0,pad/2,0,pad/2);content.addView(detail);
  bar=new ShaderProgressBar(c);bar.setMax(1000);bar.setProgressTintList(ColorStateList.valueOf(BrandUi.ACCENT));bar.setProgressBackgroundTintList(ColorStateList.valueOf(0xff344552));LinearLayout.LayoutParams bp=new LinearLayout.LayoutParams(-1,BrandUi.dp(c,14));bp.setMargins(pad,0,pad,pad);content.addView(bar,bp);
  note=new TextView(c);note.setText("This required preparation helps deliver smoother gameplay.\n\nYou can minimise the app, though preparation may be slower. Keeping it open can finish faster. If notifications are allowed, follow progress there; otherwise reopen the app to check. Please don't close the app until it's ready. You'll hear a crystal chime when preparation is complete.");title.setVisibility(GONE);detail.setVisibility(GONE);note.setVisibility(GONE);note.setTextSize(16);note.setTextColor(BrandUi.SECONDARY);note.setGravity(Gravity.CENTER);content.addView(note);
 }
 @Override protected void onSizeChanged(int w,int h,int oldw,int oldh){super.onSizeChanged(w,h,oldw,oldh);
  if(content==null)return;int pad=BrandUi.dp(getContext(),h<BrandUi.dp(getContext(),500)?16:28);content.setPadding(pad,pad,pad,pad);
  android.view.ViewGroup.LayoutParams lp=logo.getLayoutParams();lp.height=Math.max(BrandUi.dp(getContext(),56),Math.min(BrandUi.dp(getContext(),112),h/5));logo.setLayoutParams(lp);
 }
 void update(long state){boolean full=display.detailed(state);title.setVisibility(full?VISIBLE:GONE);detail.setVisibility(full?VISIBLE:GONE);note.setVisibility(full?VISIBLE:GONE);bar.setShimmer(display.shimmer(state));int done=(int)(state&0xfffffffL),total=(int)((state>>>28)&0xfffffffL);int stage=(int)((state>>>56)&15),unit=(int)(state>>>60);
  String[] stages={"Compiling shaders","Preparing pipelines","Checking the cache","Extracting shaders","Scanning game files","Loading cached shaders"},units={"shaders","pipelines","files","MiB","entries"};
  bar.setIndeterminate(total==0);if(total>0)bar.setProgress((int)(1000L*Math.min(done,total)/total));
  if(state==0){detail.setText("Finishing preparation…");return;}
  detail.setText((stage<stages.length?stages[stage]:"Preparing game resources")+"\n"+(total>0?String.format(java.util.Locale.US,"%,d / %,d %s · %d%%",done,total,unit<units.length?units[unit]:"items",100L*Math.min(done,total)/total):"Please wait…"));
 }
}
