package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
/** Presentation choices only; game pixels and control geometry are retained. */
final class DisplayAppearance {
 static final ColorMatrixColorFilter MONOCHROME;
 static {ColorMatrix m=new ColorMatrix();m.setSaturation(0);MONOCHROME=new ColorMatrixColorFilter(m);}
 static int border(Context c){return Math.max(0,Math.min(2,c.getSharedPreferences("display-appearance",0).getInt("border",0)));}
 static void setBorder(Context c,int value){c.getSharedPreferences("display-appearance",0).edit().putInt("border",value).apply();apply(c);}
 static boolean locked16(Context c){return c.getSharedPreferences("display-appearance",0).getBoolean("lock-16-9",false);}
 static void setLocked16(Context c,boolean value){c.getSharedPreferences("display-appearance",0).edit().putBoolean("lock-16-9",value).apply();apply(c);}
 static int surround(Context c){return new int[]{0xff0b1119,0xff101114,0xff000000}[border(c)];}
 static void apply(Context c){RuntimeReadinessActivity.nativeBorderColour(border(c));if(c instanceof RuntimeReadinessActivity)((RuntimeReadinessActivity)c).refreshDisplay();}
}
