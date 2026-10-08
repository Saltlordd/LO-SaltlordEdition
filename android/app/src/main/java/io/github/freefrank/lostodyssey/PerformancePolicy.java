package io.github.freefrank.lostodyssey;
/** Sampling arithmetic and honest unavailable values, independent of Android UI. */
final class PerformancePolicy {
 static double rate(long before,long after,long elapsed){return before<0||after<before||elapsed<=0?Double.NaN:(after-before)*1000.0/elapsed;}
 static double cpu(long before,long after,long elapsed){return before<0||after<before||elapsed<=0?Double.NaN:(after-before)*100.0/elapsed;}
 static double percent(String text){try{double value=Double.parseDouble(text.trim().replace("%",""));return Double.isFinite(value)&&value>=0&&value<=100?value:Double.NaN;}catch(RuntimeException e){return Double.NaN;}}
 static String thermal(int state){String[] names={"Normal","Light","Moderate","Severe","Critical","Emergency","Shutdown"};return state>=0&&state<names.length?names[state]:"Unavailable";}
 static int size(int value){return Math.max(0,Math.min(2,value));}
}
