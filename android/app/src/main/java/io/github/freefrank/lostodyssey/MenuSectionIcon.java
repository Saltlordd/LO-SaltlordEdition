package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.graphics.*;
import android.graphics.drawable.Drawable;
// Small static line symbols: no bitmap work, timers or animation.
final class MenuSectionIcon extends Drawable {
 private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
 private final int size;private final String title;
 MenuSectionIcon(Context c,String title){size=BrandUi.dp(c,26);this.title=title;paint.setColor(BrandUi.ACCENT);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.6f);paint.setStrokeCap(Paint.Cap.ROUND);paint.setStrokeJoin(Paint.Join.ROUND);}
 public int getIntrinsicWidth(){return size;}public int getIntrinsicHeight(){return size;}
 public void draw(Canvas c){c.save();c.translate(getBounds().left,getBounds().top);c.scale(getBounds().width()/26f,getBounds().height()/26f);
  if(title.startsWith("Overlay")){c.drawRoundRect(3,3,23,23,3,3,paint);c.drawLine(6,10,12,10,paint);c.drawLine(9,7,9,13,paint);c.drawCircle(18,8,1.5f,paint);c.drawCircle(18,17,2,paint);c.drawLine(6,19,11,19,paint);}
  else if(title.startsWith("Controllers")){Path p=new Path();p.moveTo(7,7);p.lineTo(19,7);p.cubicTo(23,7,25,21,21,21);p.lineTo(17,17);p.lineTo(9,17);p.lineTo(5,21);p.cubicTo(1,21,3,7,7,7);c.drawPath(p,paint);c.drawLine(6,12,12,12,paint);c.drawLine(9,9,9,15,paint);c.drawCircle(18,11,1,paint);c.drawCircle(21,14,1,paint);}
  else if(title.startsWith("Graphics")){c.drawRoundRect(2,4,24,19,2,2,paint);c.drawLine(13,19,13,23,paint);c.drawLine(8,23,18,23,paint);c.drawLine(6,15,11,10,paint);c.drawLine(11,10,15,14,paint);c.drawLine(15,14,20,8,paint);}
  else if(title.startsWith("Files")){Path p=new Path();p.moveTo(3,21);p.lineTo(3,5);p.lineTo(11,5);p.lineTo(14,8);p.lineTo(23,8);p.lineTo(23,21);p.close();c.drawPath(p,paint);c.drawLine(7,13,19,13,paint);c.drawLine(7,17,15,17,paint);}
  else if(title.startsWith("Saves")){c.drawRoundRect(4,3,22,23,2,2,paint);c.drawRect(8,3,18,10,paint);c.drawRect(8,15,18,23,paint);c.drawLine(15,5,15,8,paint);}
  else if(title.startsWith("Debug")){c.drawRoundRect(7,6,19,21,5,5,paint);c.drawLine(10,3,13,6,paint);c.drawLine(16,3,13,6,paint);for(int y:new int[]{10,15,20}){c.drawLine(3,y,7,y,paint);c.drawLine(19,y,23,y,paint);}c.drawLine(13,9,13,18,paint);}
  else {c.drawCircle(13,13,10,paint);c.drawCircle(13,7,0.8f,paint);c.drawLine(13,11,13,19,paint);}
  c.restore();
 }
 public void setAlpha(int a){paint.setAlpha(a);}public void setColorFilter(ColorFilter f){paint.setColorFilter(f);}public int getOpacity(){return PixelFormat.TRANSLUCENT;}
}
