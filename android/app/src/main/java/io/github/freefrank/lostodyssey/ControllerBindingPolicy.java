package io.github.freefrank.lostodyssey;
/** Plain binding codec shared by UI, persistence and capture tests. */
final class ControllerBindingPolicy {
 static final int[] DEFAULTS={0,1,2,3,9,10,4,6,7,8,11,12,13,14,108,110,101,100,103,102,105,104,107,106};
 static final String[] TARGETS={"A","B","X","Y","Left bumper (LB)","Right bumper (RB)","Back","Start","Left stick press (L3)","Right stick press (R3)","D-pad up","D-pad down","D-pad left","D-pad right","Left trigger (LT)","Right trigger / ring (RT)","Left stick left","Left stick right","Left stick up","Left stick down","Right stick left","Right stick right","Right stick up","Right stick down"};
 static final String[] BUTTONS={"A","B","X","Y","Back","Guide","Start","Left stick press","Right stick press","Left bumper","Right bumper","D-pad up","D-pad down","D-pad left","D-pad right","Misc","Paddle 1","Paddle 2","Paddle 3","Paddle 4","Touchpad"};
 static final String[] AXES={"Left stick horizontal","Left stick vertical","Right stick horizontal","Right stick vertical","Left trigger","Right trigger"};
 static boolean valid(int b){return b==-1||(b>=0&&b<21)||(b>=100&&b<112);}
 static boolean valid(int[] a){if(a==null||a.length!=24)return false;for(int b:a)if(!valid(b))return false;return true;}
 static String source(int b){if(b==-1)return "Unbound";if(b<21)return BUTTONS[b];int a=(b-100)/2;return AXES[a]+((b&1)==0?" +":" −");}
 static boolean neutral(int[] state){if(state.length!=27)return true;for(int i=0;i<21;i++)if(state[i]!=0)return false;for(int i=21;i<27;i++)if(Math.abs(state[i])>12000)return false;return true;}
 static int pressed(int[] state){if(state.length!=27)return -1;for(int i=0;i<21;i++)if(state[i]>20000)return i;for(int i=0;i<6;i++)if(Math.abs(state[21+i])>20000)return 100+i*2+(state[21+i]<0?1:0);return -1;}
}
