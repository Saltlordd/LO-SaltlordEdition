package io.github.freefrank.lostodyssey;
import android.app.ActivityManager;import android.content.Context;import android.os.*;import java.io.*;
final class DiagnosticReport {
 static void copy(InputStream in,OutputStream out)throws IOException{byte[] bytes=new byte[32768];int n;while((n=in.read(bytes))!=-1)out.write(bytes,0,n);}
 static String header(RuntimeReadinessActivity app){String version="Unavailable";try{version=app.getPackageManager().getPackageInfo(app.getPackageName(),0).versionName;}catch(Exception ignored){}
 ActivityManager.MemoryInfo mem=new ActivityManager.MemoryInfo();ActivityManager am=(ActivityManager)app.getSystemService(Context.ACTIVITY_SERVICE);if(am!=null)am.getMemoryInfo(mem);
 StatFs disk=new StatFs(app.getFilesDir().getAbsolutePath());String chip=Build.VERSION.SDK_INT>=31?Build.SOC_MANUFACTURER+" "+Build.SOC_MODEL:Build.HARDWARE;
 GpuDriverStore.Installed driver=GpuDriverStore.selectedDriver(app);String driverName=driver==null?"System":driver.metadata.name;
 return "LO: Saltlord Edition diagnostic report\nApp: "+version+"\nDevice: "+Build.MANUFACTURER+" "+Build.MODEL+"\nAndroid: "+Build.VERSION.RELEASE+" (API "+Build.VERSION.SDK_INT+")\nChipset: "+chip+"\nRAM bytes: "+mem.totalMem+"\nApp-storage volume total/available bytes: "+disk.getTotalBytes()+" / "+disk.getAvailableBytes()+"\nGraphics render/target/AA: "+RuntimeReadinessActivity.nativeRenderResolution(0)+" / "+RuntimeReadinessActivity.nativeFrameRate(0)+" / "+RuntimeReadinessActivity.nativeAntiAliasing(-1)+" (0 Off, 1 FXAA, 2 SMAA)\nFlex mode: "+app.flexMode()+"\n\nSend to: saltlordstrikes@gmail.com\nBug description: [Please add]\nExpected behaviour: [Please add]\nSteps to reproduce: [Please add]\nFrequency / first or repeat encounter: [Please add]\nScreen / controller: [Please add]\nDevice details above are best-effort; storage is a filesystem volume, not advertised phone capacity. Review before sending. No saves are attached.\n\n--- Runtime log ---\n";}
}
