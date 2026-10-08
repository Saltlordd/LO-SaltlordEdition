package io.github.freefrank.lostodyssey;
/** Real compiler stages choose detailed first-preparation UI; zero-progress gaps do not reset it. */
final class PreparationDisplayPolicy {
 private boolean preparing;
 boolean detailed(long state){
  int total=(int)((state>>>28)&0xfffffffL),stage=(int)((state>>>56)&15);
  if(state!=0&&total>0&&(stage==0||stage==3||stage==4))preparing=true;
  return preparing;
 }
 boolean shimmer(long state){return preparing&&state!=0;}
}
