package io.github.freefrank.lostodyssey;
import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.os.Build;
/** Own-app exit metadata, included only in user-requested local reports. */
final class ProcessExitReport {
 static String summary(Context context){
  if(Build.VERSION.SDK_INT<30)return "Process exit history: unavailable before Android 11\n";
  try{return Api30.summary(context);}catch(RuntimeException|LinkageError e){return "Process exit history: unavailable ("+e.getClass().getSimpleName()+")\n";}
 }
 private static final class Api30 {
  static String summary(Context context){
   ActivityManager am=(ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
   if(am==null)return "Process exit history: unavailable\n";
   StringBuilder out=new StringBuilder("Recent own-app exits (includes intentional restarts):\n");
   java.util.List<ApplicationExitInfo> exits=am.getHistoricalProcessExitReasons(context.getPackageName(),0,5);
   if(exits.isEmpty())out.append("None reported\n");
   for(ApplicationExitInfo e:exits)out.append("timestampMs=").append(e.getTimestamp()).append(" reason=").append(reason(e.getReason())).append(" status=").append(e.getStatus()).append(" importance=").append(e.getImportance()).append(" pssKiB=").append(e.getPss()).append(" rssKiB=").append(e.getRss()).append('\n');
   return out.toString();
  }
 }
 static String reason(int reason){switch(reason){case 1:return "EXIT_SELF";case 2:return "SIGNALED";case 3:return "LOW_MEMORY";case 4:return "JAVA_CRASH";case 5:return "NATIVE_CRASH";case 6:return "ANR";case 10:return "USER_REQUESTED";default:return "OTHER("+reason+")";}}
}
