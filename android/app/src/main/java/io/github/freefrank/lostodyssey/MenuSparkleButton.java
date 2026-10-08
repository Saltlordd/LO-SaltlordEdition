package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.widget.Button;
/** Two tiny icy glints on each primary button, redrawn at most 12.5 times per second. */
final class MenuSparkleButton extends Button {
 private final Paint glint=new Paint(Paint.ANTI_ALIAS_FLAG);private final int offset;private final android.graphics.Path star=new android.graphics.Path();
 MenuSparkleButton(Context c,int offset){super(c);this.offset=offset;setWillNotDraw(false);}
 @Override protected void onDraw(Canvas canvas){
  super.onDraw(canvas);
  boolean animate=isEnabled()&&MenuSparklePolicy.animate(isShown(),hasWindowFocus(),android.animation.ValueAnimator.areAnimatorsEnabled());
  long time=animate?android.os.SystemClock.uptimeMillis():0;
  float d=getResources().getDisplayMetrics().density;
  for(int i=0;i<2;i++){
   float strength=animate?MenuSparklePolicy.brightness(time,offset+i):.3f;
   float radius=(2.5f+strength*1.5f)*d;
   float x=i==0?8*d:getWidth()-8*d,y=getHeight()*(i==0?.30f:.70f);
   glint.setColor(BrandUi.ACCENT);glint.setAlpha(Math.round(35+125*strength));glint.setStrokeWidth(d);
   star.reset();star.moveTo(x,y-radius);star.lineTo(x+radius*.22f,y-radius*.22f);star.lineTo(x+radius,y);star.lineTo(x+radius*.22f,y+radius*.22f);star.lineTo(x,y+radius);star.lineTo(x-radius*.22f,y+radius*.22f);star.lineTo(x-radius,y);star.lineTo(x-radius*.22f,y-radius*.22f);star.close();canvas.drawPath(star,glint);
   glint.setAlpha(Math.round(20+70*strength));canvas.drawCircle(x,y,d,glint);
  }
  if(animate)postInvalidateDelayed(80);
 }
 @Override public void onWindowFocusChanged(boolean focused){super.onWindowFocusChanged(focused);invalidate();}
}
