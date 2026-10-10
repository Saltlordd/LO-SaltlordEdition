package io.github.freefrank.lostodyssey;

import android.content.Context;
import android.content.SharedPreferences;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

/** Explicit Android output choices; applied before native settings are loaded. */
final class MobilePresentation {
 static SharedPreferences prefs(Context c){return c.getSharedPreferences("mobile-presentation",Context.MODE_PRIVATE);}
 static boolean timings(Context c){return false;}
 static int scaling(Context c){return 0;}
 static int rgb(Context c){return prefs(c).getInt("rgb",1)==1?1:0;}
 static void choose(Context c,String key,int value){if(!key.equals("scaling")&&!key.equals("rgb"))throw new IllegalArgumentException("Unknown output choice");if(!prefs(c).edit().putInt(key,value==1?1:0).commit())throw new IllegalStateException("Could not save output choice");}
 static String settings(String source,int filter,int range){
  if(filter<0||filter>1||range<0||range>1)throw new IllegalArgumentException("Invalid output choice");
  int[] saved=SavedGraphics.parse(source);
  StringBuilder out=new StringBuilder();
  for(String line:source.split("\n")){
   String key=line.trim().split("=",2)[0].trim();
   if(key.equals("scaling_quality")||key.equals("expand_rgb_range")||key.equals("internal_resolution")||key.equals("antialiasing")||key.equals("fxaa")||key.equals("anisotropic_filtering")||key.equals("upscaler")||key.equals("fsr_sharpness")||key.equals("frame_generation_provider")||key.equals("variable_refresh_rate"))continue;
   out.append(line).append('\n');
  }
  return out.append("scaling_quality=0\nexpand_rgb_range=").append(range).append("\ninternal_resolution=").append(saved[0]).append("\nantialiasing=").append(saved[2]).append("\nfxaa=").append(saved[2]==1?1:0).append("\nanisotropic_filtering=0\nupscaler=0\nfsr_sharpness=0\nframe_generation_provider=0\nvariable_refresh_rate=0\n").toString();
 }
 static void prepare(Context c)throws IOException{
  File directory=new File(c.getFilesDir(),"config"),file=new File(directory,"settings.ini");
  if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot create graphics settings folder");
  String previous=file.isFile()?new String(Files.readAllBytes(file.toPath()),StandardCharsets.UTF_8):"";
  // Introduce Expanded once for fresh installs and upgrades from diagnostic builds.
  // Later explicit Standard choices remain respected across all graphics profiles.
  if(!prefs(c).getBoolean("expanded-rgb-default-v1",false)){
   if(!prefs(c).edit().putInt("rgb",1).putBoolean("expanded-rgb-default-v1",true).commit())throw new IOException("Cannot save colour-range default");
  }
  String updated=settings(previous,scaling(c),rgb(c));
  if(previous.equals(updated))return;
  File temporary=File.createTempFile("presentation-",".tmp",directory);
  try{
   try(FileOutputStream out=new FileOutputStream(temporary)){out.write(updated.getBytes(StandardCharsets.UTF_8));out.getFD().sync();}
   Files.move(temporary.toPath(),file.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
  }finally{temporary.delete();}
 }
}
