package io.github.freefrank.lostodyssey;
/** Integer, centred fitting keeps the renderer at 16:9 without stretching its pixels. */
final class ViewportFit {
 static int[] bounds(int w,int h,boolean locked){
  w=Math.max(0,w);h=Math.max(0,h);int fw=w,fh=h;
  if(locked&&w>0&&h>0){if((long)w*9>(long)h*16)fw=(int)((long)h*16/9);else fh=(int)((long)w*9/16);}
  int x=(w-fw)/2,y=(h-fh)/2;return new int[]{x,y,x+fw,y+fh};
 }
}
