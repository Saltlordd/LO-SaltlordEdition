#!/usr/bin/env python3
"""Host integration test; pass an org.json implementation JAR and a JDK java executable."""
import argparse, pathlib, subprocess, tempfile

STUBS = {
 'android/content/SharedPreferences.java': '''package android.content;import java.util.*;public class SharedPreferences{final Map<String,Object> m=new HashMap<>();public boolean getBoolean(String k,boolean d){return m.containsKey(k)?(Boolean)m.get(k):d;}public String getString(String k,String d){return m.containsKey(k)?(String)m.get(k):d;}public int getInt(String k,int d){return m.containsKey(k)?(Integer)m.get(k):d;}public Editor edit(){return new Editor();}public class Editor{Map<String,Object> pending=new HashMap<>();public Editor putInt(String k,int v){pending.put(k,v);return this;}public Editor putString(String k,String v){pending.put(k,v);return this;}public Editor putBoolean(String k,boolean v){pending.put(k,v);return this;}public Editor remove(String k){pending.put(k,null);return this;}public boolean commit(){for(String k:pending.keySet())if(pending.get(k)==null)m.remove(k);else m.put(k,pending.get(k));return true;}public void apply(){commit();}}}''',
 'android/content/Context.java': '''package android.content;import java.io.*;import java.util.*;public class Context{public static final int MODE_PRIVATE=0;final File root;final byte[] seed;final Map<String,SharedPreferences> prefs=new HashMap<>();public Context(File root,byte[] seed){this.root=root;this.seed=seed;getFilesDir().mkdirs();getCacheDir().mkdirs();}public File getFilesDir(){return new File(root,"files");}public File getCacheDir(){return new File(root,"cache");}public SharedPreferences getSharedPreferences(String name,int mode){return prefs.computeIfAbsent(name,k->new SharedPreferences());}public android.content.res.AssetManager getAssets(){return new android.content.res.AssetManager(seed);}}''',
 'android/content/res/AssetManager.java': '''package android.content.res;import java.io.*;public class AssetManager{final byte[] seed;public AssetManager(byte[] seed){this.seed=seed;}public InputStream open(String name){return new ByteArrayInputStream(seed);}}''',
 'android/os/Build.java': '''package android.os;public class Build{public static String[] SUPPORTED_ABIS={"arm64-v8a"};public static class VERSION{public static int SDK_INT=37;}}''',
 'android/system/Os.java': '''package android.system;public class Os{public static long sysconf(int value){return 16384;}}''',
 'android/system/OsConstants.java': '''package android.system;public class OsConstants{public static int _SC_PAGESIZE=30;}''',
 'android/util/Log.java': '''package android.util;public class Log{public static int i(String tag,String text){return 0;}}''',
 'io/github/freefrank/lostodyssey/MobilePresentation.java': '''package io.github.freefrank.lostodyssey;final class MobilePresentation{static android.content.SharedPreferences prefs(android.content.Context c){return c.getSharedPreferences("mobile-presentation",0);}}'''
}
def main():
 p=argparse.ArgumentParser();p.add_argument('--java',default='java');p.add_argument('--json-jar',type=pathlib.Path,required=True);a=p.parse_args()
 root=pathlib.Path(__file__).resolve().parents[2];package=root/'android/app/src/main/java/io/github/freefrank/lostodyssey'
 with tempfile.TemporaryDirectory(prefix='pipeline-warmup-test-') as temp:
  folder=pathlib.Path(temp);sources=[]
  for name,text in STUBS.items():
   target=folder/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text);sources.append(str(target))
  sources += [str(package/(name+'.java')) for name in ['PipelineWarmupSeeds','GpuDriverStore','DriverElf','DriverPackageIdentity']]
  sources.append(str(root/'tools/tests/PipelineWarmupSeedsTest.java'))
  classes=folder/'classes';classes.mkdir()
  subprocess.run([a.java,'com.sun.tools.javac.Main','-cp',str(a.json_jar.resolve()),'-d',str(classes),*sources],check=True)
  subprocess.run([a.java,'-cp',str(classes)+':'+str(a.json_jar.resolve()),'io.github.freefrank.lostodyssey.PipelineWarmupSeedsTest',str(root/'android/app/src/main/assets/warmup/pipelines_vk12_1.bin')],check=True)
if __name__=='__main__':main()
