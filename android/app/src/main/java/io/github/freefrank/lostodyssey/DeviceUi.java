package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
final class DeviceUi {
 private static SensorManager sensors;
 private static float angle=Float.NaN;
 private static final SensorEventListener hinge=new SensorEventListener(){
  public void onSensorChanged(SensorEvent event){if(event.values.length>0)angle=event.values[0];}
  public void onAccuracyChanged(Sensor sensor,int accuracy){}
 };
 static void observe(Context c){stopObserving();if(!foldable(c))return;SensorManager manager=(SensorManager)c.getSystemService(Context.SENSOR_SERVICE);if(manager==null)return;Sensor sensor=manager.getDefaultSensor(36);if(sensor!=null&&manager.registerListener(hinge,sensor,SensorManager.SENSOR_DELAY_NORMAL))sensors=manager;}
 static void stopObserving(){if(sensors!=null)sensors.unregisterListener(hinge);sensors=null;angle=Float.NaN;}
 private static int displayType(Context c){try{return c.getResources().getConfiguration().getClass().getField("semDisplayDeviceType").getInt(c.getResources().getConfiguration());}catch(ReflectiveOperationException|SecurityException e){return -1;}}
 static boolean flexAvailable(Context c,int activeMode){return FlexAvailabilityPolicy.available(foldable(c),displayType(c),angle,activeMode);}
 static String flexUnavailable(Context c){return foldable(c)?"Unfold your phone to use Flex mode. Reopen Options after unfolding.":"Experimental Flex mode is available on supported folding phones.";}
 static boolean foldable(Context c){return c.getPackageManager().hasSystemFeature("android.hardware.sensor.hinge_angle");}
 static String layoutNote(Context c){return foldable(c)?"Foldable devices can use separate touch layouts for the cover and inner displays.":"Your touch layout is saved automatically.";}
 static String backedUp(Context c){return foldable(c)?"Touch layouts backed up successfully, including cover and inner layouts.":"Touch layouts backed up successfully.";}
}
