package io.github.freefrank.lostodyssey;
import android.view.Gravity;
import android.view.View;
import android.widget.*;
/** A fitted menu, with a centred primary group and a compact, pinned footer. */
final class LauncherMenu extends FrameLayout {
 private final View[] items=new View[8];private final ImageButton flex;
 private LauncherLayoutPolicy.Plan plan;
 private final android.os.Handler handler=new android.os.Handler(android.os.Looper.getMainLooper());
 private final Runnable posture=new Runnable(){public void run(){updateFlex();handler.postDelayed(this,1000);}};
 private void updateFlex(){boolean available=DeviceUi.flexAvailable(getContext(),0);flex.setEnabled(available);flex.setAlpha(1f);flex.setColorFilter(available?BrandUi.ACCENT:0xff8d959c,android.graphics.PorterDuff.Mode.SRC_IN);android.graphics.drawable.GradientDrawable bg=(android.graphics.drawable.GradientDrawable)flex.getBackground();bg.setColor(available?0xff263d4e:0xff292e33);bg.setStroke(BrandUi.dp(getContext(),1),available?BrandUi.ACCENT:0xff737b83);flex.setContentDescription(available?"Flex mode (experimental)":"Flex mode unavailable — open the foldable inner display");}
 @Override protected void onAttachedToWindow(){super.onAttachedToWindow();handler.post(posture);}
 @Override protected void onDetachedFromWindow(){handler.removeCallbacks(posture);super.onDetachedFromWindow();}
 LauncherMenu(RuntimeReadinessActivity c,Runnable start,Runnable options,Runnable exit,Runnable chooseFlex){
  super(c);setBackground(BrandUi.backdrop(c));
  ImageView logo=new ImageView(c);logo.setImageBitmap(BrandUi.full(c));logo.setScaleType(ImageView.ScaleType.FIT_CENTER);logo.setContentDescription("LO: Saltlord Edition logo");add(0,logo);
  String[] titles={"Start game","Options","Exit app"};Runnable[] actions={start,options,exit};
  for(int i=0;i<3;i++){MenuSparkleButton b=new MenuSparkleButton(c,i*2);b.setTag("section-header");b.setText(titles[i]);b.setGravity(Gravity.CENTER);final Runnable action=actions[i];b.setOnClickListener(v->action.run());BrandUi.style(b);b.setPadding(BrandUi.dp(c,12),BrandUi.dp(c,4),BrandUi.dp(c,12),BrandUi.dp(c,4));b.setAutoSizeTextTypeUniformWithConfiguration(12,19,1,android.util.TypedValue.COMPLEX_UNIT_SP);add(i+1,b);}
  Button thanks=footer(c,"Acknowledgements",()->{c.menuChime();BrandUi.showCredits(c);});add(4,thanks);
  Button licences=footer(c,"Open-source licences",()->{c.menuChime();BrandUi.showNotices(c);});add(5,licences);
  TextView version=new TextView(c);version.setText("v0.1.0 beta");version.setTextColor(BrandUi.SECONDARY);version.setGravity(Gravity.LEFT|Gravity.CENTER_VERTICAL);version.setAutoSizeTextTypeUniformWithConfiguration(10,12,1,android.util.TypedValue.COMPLEX_UNIT_SP);add(6,version);
  flex=new ImageButton(c);flex.setContentDescription("Flex mode (experimental)");flex.setImageDrawable(new FoldPhoneIcon(c));flex.setScaleType(ImageView.ScaleType.FIT_CENTER);int inset=BrandUi.dp(c,15);flex.setPadding(inset,inset,inset,inset);flex.setOnClickListener(v->chooseFlex.run());
  android.graphics.drawable.GradientDrawable circle=new android.graphics.drawable.GradientDrawable();circle.setShape(android.graphics.drawable.GradientDrawable.OVAL);circle.setColor(0xff263d4e);circle.setStroke(BrandUi.dp(c,1),BrandUi.ACCENT);flex.setBackground(circle);add(7,flex);
 }

 private Button footer(RuntimeReadinessActivity c,String text,Runnable action){Button b=new Button(c);b.setText(text);b.setOnClickListener(v->action.run());BrandUi.style(b);b.setPadding(BrandUi.dp(c,8),0,BrandUi.dp(c,8),0);b.setAutoSizeTextTypeUniformWithConfiguration(10,14,1,android.util.TypedValue.COMPLEX_UNIT_SP);return b;}
 private void add(int index,View child){items[index]=child;addView(child,new FrameLayout.LayoutParams(1,1));}
 @Override protected void onMeasure(int ws,int hs){int w=MeasureSpec.getSize(ws),h=MeasureSpec.getSize(hs);plan=LauncherLayoutPolicy.plan(w,h,getResources().getDisplayMetrics().density);for(int i=0;i<8;i++){int[] b=plan.rects[i];items[i].measure(MeasureSpec.makeMeasureSpec(Math.max(0,b[2]-b[0]),MeasureSpec.EXACTLY),MeasureSpec.makeMeasureSpec(Math.max(0,b[3]-b[1]),MeasureSpec.EXACTLY));}setMeasuredDimension(w,h);}
 @Override protected void onLayout(boolean changed,int l,int t,int r,int b){plan=LauncherLayoutPolicy.plan(r-l,b-t,getResources().getDisplayMetrics().density);for(int i=0;i<8;i++){int[] box=plan.rects[i];items[i].layout(box[0],box[1],box[2],box[3]);}updateFlex();}
}
