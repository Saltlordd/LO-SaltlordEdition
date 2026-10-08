package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.graphics.*;
import android.graphics.drawable.Drawable;
/** Both examples show a landscape phone: the highlighted line is its crease. */
final class HingeExampleIcon extends Drawable {
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);private final boolean vertical;private final int width,height;
    HingeExampleIcon(Context c,boolean vertical){this.vertical=vertical;width=BrandUi.dp(c,76);height=BrandUi.dp(c,48);}
    public int getIntrinsicWidth(){return width;}public int getIntrinsicHeight(){return height;}
    public void draw(Canvas canvas){canvas.save();canvas.translate(getBounds().left,getBounds().top);canvas.scale(getBounds().width()/76f,getBounds().height()/48f);
        paint.setStyle(Paint.Style.FILL);paint.setColor(0xff101c27);canvas.drawRoundRect(2,3,74,45,5,5,paint);
        paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(2);paint.setColor(0xffaabac8);canvas.drawRoundRect(2,3,74,45,5,5,paint);
        paint.setColor(BrandUi.ACCENT);paint.setStrokeWidth(3);if(vertical)canvas.drawLine(38,5,38,43,paint);else canvas.drawLine(4,24,72,24,paint);canvas.restore();}
    public void setAlpha(int a){paint.setAlpha(a);}public void setColorFilter(ColorFilter f){paint.setColorFilter(f);}public int getOpacity(){return PixelFormat.TRANSLUCENT;}
}
