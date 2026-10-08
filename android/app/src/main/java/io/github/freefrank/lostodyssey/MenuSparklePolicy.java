package io.github.freefrank.lostodyssey;
/** A slow, deterministic twinkle; no textures, blur or particle allocations. */
final class MenuSparklePolicy {
 static float brightness(long millis,int index){double phase=(Math.floorMod(millis,5200L)+index*1100)%5200/5200.0;return (float)(.16+.84*Math.pow((1+Math.cos(phase*2*Math.PI))/2,4));}
 static boolean animate(boolean shown,boolean focused,boolean enabled){return shown&&focused&&enabled;}
}
