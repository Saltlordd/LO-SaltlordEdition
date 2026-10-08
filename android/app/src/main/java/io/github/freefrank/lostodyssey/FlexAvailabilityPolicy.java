package io.github.freefrank.lostodyssey;
final class FlexAvailabilityPolicy {
 // Samsung reports 1 for its cover and 0 for its inner display. Other vendors
 // use hinge angle; unknown capability/posture stays unavailable while experimental.
 static boolean available(boolean folding,int displayType,float hingeAngle,int activeMode){
  if(activeMode!=0)return true; // Always allow the user to return to fullscreen.
  if(!folding)return false;
  if(displayType==1)return false;
  if(displayType==0)return true;
  return !Float.isNaN(hingeAngle)&&hingeAngle>10f;
 }
}
