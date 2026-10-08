package io.github.freefrank.lostodyssey;
final class TouchContextPolicy {
 static boolean eligible(int context,int kind,String label){return kind>=6||context!=1||label.equals("A");}
 static int floatingKind(boolean rt,float x,float width){return rt?2:(x<width*.5f?1:2);}
 static float fade(float from,float to,long elapsed){return from+(to-from)*Math.min(1f,Math.max(0f,elapsed/180f));}
}
