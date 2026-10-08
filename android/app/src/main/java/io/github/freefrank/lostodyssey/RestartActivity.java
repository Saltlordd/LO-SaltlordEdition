package io.github.freefrank.lostodyssey;
import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.content.Intent;

/** Relaunch handoff lives outside :bootcheck, so old guest state cannot survive. */
public final class RestartActivity extends Activity {
 @Override protected void onCreate(Bundle state){
  super.onCreate(state);setRequestedOrientation(android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
  android.widget.TextView message=new android.widget.TextView(this);message.setText("Restarting LO: Saltlord Edition…");message.setTextColor(BrandUi.PRIMARY);message.setTextSize(20);message.setGravity(android.view.Gravity.CENTER);message.setBackgroundColor(BrandUi.SURFACE);setContentView(message);
  final int old=getIntent().getIntExtra("old-boot-process",-1);
  final long deadline=android.os.SystemClock.elapsedRealtime()+5000;
  Handler handler=new Handler(Looper.getMainLooper());
  handler.post(new Runnable(){public void run(){
   boolean alive=false;android.app.ActivityManager manager=(android.app.ActivityManager)getSystemService(ACTIVITY_SERVICE);
   java.util.List<android.app.ActivityManager.RunningAppProcessInfo> running=manager.getRunningAppProcesses();
   if(running!=null)for(android.app.ActivityManager.RunningAppProcessInfo p:running)if(p.pid==old&&p.uid==android.os.Process.myUid()&&p.processName.equals(getPackageName()+":bootcheck")){alive=true;break;}
   if(alive&&android.os.SystemClock.elapsedRealtime()<deadline){handler.postDelayed(this,100);return;}
   if(alive){android.os.Process.killProcess(old);handler.postDelayed(this,150);return;}
   Intent launch=new Intent(RestartActivity.this,RuntimeReadinessActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TASK);
   launch.putExtra("resume-game",true);
   int mode=FlexLayoutPolicy.valid(getIntent().getIntExtra("flex-mode",0));
   if(mode!=0)launch.putExtra("flex-token",FlexLaunchProvider.issue(mode));
   getIntent().removeExtra("flex-mode");
   startActivity(launch);finish();
  }});
 }
}
