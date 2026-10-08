package io.github.freefrank.lostodyssey;
import android.app.AlertDialog;
import android.content.Context;
import android.view.KeyEvent;
import android.view.MotionEvent;
import org.libsdl.app.SDLControllerManager;
/** A modal capture window must keep delivering controller events to SDL. */
final class ControllerCaptureDialog extends AlertDialog {
 ControllerCaptureDialog(Context c){super(c);}
 @Override public boolean dispatchKeyEvent(KeyEvent e){
  if(SDLControllerManager.isDeviceSDLJoystick(e.getDeviceId())){
   if(e.getAction()==KeyEvent.ACTION_DOWN){SDLControllerManager.onNativePadDown(e.getDeviceId(),e.getKeyCode());}
   else if(e.getAction()==KeyEvent.ACTION_UP){SDLControllerManager.onNativePadUp(e.getDeviceId(),e.getKeyCode());}
   return true;
  }return super.dispatchKeyEvent(e);
 }
 @Override public boolean dispatchGenericMotionEvent(MotionEvent e){if(SDLControllerManager.isDeviceSDLJoystick(e.getDeviceId())){SDLControllerManager.handleJoystickMotionEvent(e);return true;}return super.dispatchGenericMotionEvent(e);}
}
