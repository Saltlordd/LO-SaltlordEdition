package io.github.freefrank.lostodyssey;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
/** Explicit diagnostic export of portable pipeline state/hash recipes only. */
final class PipelineWarmupReport {
 static final int MAX_BYTES=2*1024*1024;
 static void append(File recipes,OutputStream out)throws IOException{
  if(!recipes.isFile())return;
  if(recipes.length()>MAX_BYTES){out.write("\nPipeline warmup metadata omitted: exceeds report limit.\n".getBytes(StandardCharsets.UTF_8));return;}
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();
  try(InputStream in=new FileInputStream(recipes)){
   byte[] buffer=new byte[8192];int n;
   while((n=in.read(buffer))!=-1){if(bytes.size()+n>MAX_BYTES)return;bytes.write(buffer,0,n);}
  }catch(IOException e){out.write("\nPipeline warmup metadata unavailable.\n".getBytes(StandardCharsets.UTF_8));return;}
  if(bytes.size()==0)return;
  out.write(("\n--- Pipeline warmup metadata v1: state/hash recipes, no shader binaries ---\n"+Base64.getEncoder().encodeToString(bytes.toByteArray())+"\n--- End pipeline warmup metadata ---\n").getBytes(StandardCharsets.UTF_8));
 }
}
