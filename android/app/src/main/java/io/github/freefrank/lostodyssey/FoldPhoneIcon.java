package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.graphics.*;
import android.graphics.drawable.Drawable;
final class FoldPhoneIcon extends Drawable {
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);private final int size;
    FoldPhoneIcon(Context c){size=BrandUi.dp(c,26);}
    public int getIntrinsicWidth(){return size;}public int getIntrinsicHeight(){return size;}
    public void draw(Canvas canvas){canvas.save();canvas.translate(getBounds().left,getBounds().top);canvas.scale(getBounds().width()/26f,getBounds().height()/26f);paint.setColor(BrandUi.ACCENT);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.6f);Path p=new Path();p.moveTo(3,5);p.lineTo(13,8);p.lineTo(23,5);p.lineTo(23,21);p.lineTo(13,24);p.lineTo(3,21);p.close();canvas.drawPath(p,paint);canvas.drawLine(13,8,13,24,paint);canvas.restore();}
    public void setAlpha(int a){paint.setAlpha(a);}public void setColorFilter(ColorFilter f){paint.setColorFilter(f);}public int getOpacity(){return PixelFormat.TRANSLUCENT;}
}
