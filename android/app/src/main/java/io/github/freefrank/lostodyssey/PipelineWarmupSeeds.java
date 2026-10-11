package io.github.freefrank.lostodyssey;

import android.content.Context;
import java.io.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.*;
import java.util.*;

/** Versioned portable recipes; actual pipelines are always compiled locally by the renderer. */
final class PipelineWarmupSeeds {
 static final String ASSET="warmup/pipelines_corpus.bin",RESET="rebuild-pipelines-next-launch";
 static final int HEADER=48,KEY=64,RECORD=72,MAX=16384,SHADER_VERSION=27;
 static long hash(byte[] bytes,int start,int length){long h=0xcbf29ce484222325L;for(int i=start;i<start+length;i++){h^=bytes[i]&255;h*=0x100000001b3L;}return h;}
 static ByteBuffer buffer(byte[] bytes){return ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);}
 static byte[] read(InputStream in)throws IOException{
  ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] bytes=new byte[8192];int n;
  while((n=in.read(bytes))!=-1){if(out.size()+n>HEADER+MAX*104)throw new IOException("Recipe file too large");out.write(bytes,0,n);}return out.toByteArray();
 }
 static List<byte[]> records(byte[] bytes)throws IOException{
  if(bytes.length<HEADER)throw new IOException("Recipe header missing");ByteBuffer b=buffer(bytes);
  if(!Arrays.equals(Arrays.copyOf(bytes,8),"LOPSO001".getBytes(StandardCharsets.US_ASCII))||b.getInt(8)!=1||b.getInt(12)!=SHADER_VERSION||b.getInt(16)!=1||b.getInt(20)!=RECORD||b.getInt(28)!=0)throw new IOException("Incompatible recipe schema");
  int count=b.getInt(24);if(count<0||count>MAX||bytes.length!=HEADER+count*RECORD||b.getLong(40)!=hash(bytes,0,40)||b.getLong(32)!=hash(bytes,HEADER,bytes.length-HEADER))throw new IOException("Recipe framing or checksum failed");
  List<byte[]> result=new ArrayList<>();
  for(int i=0;i<count;i++){
   int p=HEADER+i*RECORD;
   if(b.getLong(p+KEY)!=hash(bytes,p,KEY)||(b.getInt(p+24)&~0x3807)!=0||(b.getInt(p+28)&~15)!=0||b.getInt(p+44)!=0||(b.getInt(p+48)&~0xffffff)!=0||(b.getInt(p+52)&~0xffffff)!=0||!Float.isFinite(b.getFloat(p+60)))throw new IOException("Invalid recipe record");
   result.add(Arrays.copyOfRange(bytes,p,p+RECORD));
  }
  return result;
 }
 static byte[] merge(byte[] seed,byte[] learned)throws IOException{
  List<byte[]> bundled=records(seed);Map<String,byte[]> entries=new LinkedHashMap<>();
  if(learned!=null)for(byte[] r:records(learned))entries.put(Base64.getEncoder().encodeToString(Arrays.copyOf(r,KEY)),r);
  for(byte[] r:bundled)entries.putIfAbsent(Base64.getEncoder().encodeToString(Arrays.copyOf(r,KEY)),r);
  if(entries.size()>MAX)throw new IOException("Merged recipe limit exceeded");
  byte[] result=Arrays.copyOf(seed,HEADER+entries.size()*RECORD);int p=HEADER;
  for(byte[] r:entries.values()){System.arraycopy(r,0,result,p,RECORD);p+=RECORD;}
  ByteBuffer b=buffer(result);b.putInt(24,entries.size());b.putLong(32,hash(result,HEADER,result.length-HEADER));b.putLong(40,hash(result,0,40));return result;
 }
 static void atomicWrite(File target,byte[] bytes)throws IOException{
  File parent=target.getParentFile();if(!parent.isDirectory()&&!parent.mkdirs())throw new IOException("Cannot create recipe directory");
  File temporary=File.createTempFile("pipeline-seed-",".tmp",parent);
  try{try(FileOutputStream out=new FileOutputStream(temporary)){out.write(bytes);out.getFD().sync();}Files.move(temporary.toPath(),target.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);}finally{temporary.delete();}
 }
 static String driverCacheName(String directory,String library)throws IOException{
  String identity=directory.isEmpty()?"system":directory+"\n"+library;
  try{StringBuilder hex=new StringBuilder();for(byte b:MessageDigest.getInstance("SHA-256").digest(identity.getBytes(StandardCharsets.UTF_8)))hex.append(String.format(Locale.ROOT,"%02x",b&255));return "vulkan-pipelines-"+hex+".bin";}catch(NoSuchAlgorithmException e){throw new IOException(e);}
 }
 static int validateCorpus(byte[] bytes)throws IOException{
  if(bytes.length<HEADER)throw new IOException("Recipe header missing");ByteBuffer b=buffer(bytes);
  if(!Arrays.equals(Arrays.copyOf(bytes,8),"LOPSO001".getBytes(StandardCharsets.US_ASCII))||b.getInt(8)!=2||b.getInt(12)!=SHADER_VERSION||b.getInt(16)!=2||b.getInt(20)!=104||b.getInt(28)!=0)throw new IOException("Incompatible corpus schema");
  int count=b.getInt(24);if(count<0||count>MAX||bytes.length!=HEADER+count*104||b.getLong(40)!=hash(bytes,0,40)||b.getLong(32)!=hash(bytes,HEADER,bytes.length-HEADER))throw new IOException("Corpus checksum failed");
  for(int i=0;i<count;i++){int pos=HEADER+i*104;if(b.getLong(pos+96)!=hash(bytes,pos,96))throw new IOException("Corpus record checksum failed");}
  return count;
 }
 static void prepare(Context context)throws IOException{
  byte[] seed;try(InputStream in=context.getAssets().open(ASSET)){seed=read(in);}int count=validateCorpus(seed);
  File target=new File(context.getFilesDir(),"data/cache/shaders/pipelines_corpus.bin");
  if(!target.isFile()||!Arrays.equals(seed,Files.readAllBytes(target.toPath())))atomicWrite(target,seed);
  if(!MobilePresentation.prefs(context).edit().putBoolean(RESET,false).commit())throw new IOException("Cannot disarm legacy reset");
  String status="bundled="+count+" separate_corpus=true learned_file=preserved";
  MobilePresentation.prefs(context).edit().putString("pipeline-seed-status",status).commit();
  android.util.Log.i("LO.Launch","Pipeline warmup: "+status);
 }
 static String status(Context context){return MobilePresentation.prefs(context).getString("pipeline-seed-status","Not prepared yet");}
}
