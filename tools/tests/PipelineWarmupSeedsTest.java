package io.github.freefrank.lostodyssey;
import java.io.*;import java.nio.*;import java.nio.file.*;import java.util.*;import java.util.zip.*;
public final class PipelineWarmupSeedsTest {
 static void check(boolean value,String message){if(!value)throw new AssertionError(message);}
 static void invalid(byte[] bytes)throws Exception{try{PipelineWarmupSeeds.records(bytes);throw new AssertionError("Accepted corrupt recipes");}catch(IOException expected){}}
 static byte[] packageZip(byte[] library,boolean reversed)throws Exception{
  ByteArrayOutputStream out=new ByteArrayOutputStream();
  try(ZipOutputStream z=new ZipOutputStream(out)){
   String[] names=reversed?new String[]{"driver.so","meta.json"}:new String[]{"meta.json","driver.so"};
   for(String name:names){z.putNextEntry(new ZipEntry(name));z.write(name.equals("meta.json")?"{\"name\":\"Test driver\",\"libraryName\":\"driver.so\",\"minApi\":28}".getBytes("UTF-8"):library);z.closeEntry();}
  }return out.toByteArray();
 }
 static byte[] elf(){byte[] data=new byte[128];ByteBuffer b=ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);b.putInt(0,0x464c457f);data[4]=2;data[5]=1;data[6]=1;b.putShort(16,(short)3);b.putShort(18,(short)183);b.putLong(32,64);b.putShort(54,(short)56);b.putShort(56,(short)1);b.putInt(64,1);b.putLong(96,128);b.putLong(104,128);b.putLong(112,16384);return data;}
 public static void main(String[] args)throws Exception{
  byte[] seed=Files.readAllBytes(Paths.get(args[0]));check(PipelineWarmupSeeds.records(seed).size()==202,"Captured coverage changed");
  check(Arrays.equals(seed,PipelineWarmupSeeds.merge(seed,seed)),"Duplicate merge changed seed");
  byte[] corrupt=seed.clone();corrupt[100]^=1;invalid(corrupt);invalid(Arrays.copyOf(seed,seed.length-1));
  byte[] extra=seed.clone();extra[48]^=1;ByteBuffer b=PipelineWarmupSeeds.buffer(extra);b.putLong(112,PipelineWarmupSeeds.hash(extra,48,64));b.putLong(32,PipelineWarmupSeeds.hash(extra,48,extra.length-48));b.putLong(40,PipelineWarmupSeeds.hash(extra,0,40));
  byte[] merged=PipelineWarmupSeeds.merge(seed,extra);check(PipelineWarmupSeeds.records(merged).size()==203,"Learned recipe lost");
  File root=Files.createTempDirectory("pipeline-launch-test").toFile();android.content.Context context=new android.content.Context(root,seed);
  File driverRoot=GpuDriverStore.root(context);driverRoot.mkdirs();File old=new File(driverRoot,"import-old");
  byte[] library=elf();GpuDriverStore.Installed installed=GpuDriverStore.installFromStream(new ByteArrayInputStream(packageZip(library,false)),old);GpuDriverStore.select(context,installed.id());
  GpuDriverStore.Installed same=GpuDriverStore.importDriver(context,new ByteArrayInputStream(packageZip(library,true)));check(same.directory.equals(old),"Reimport stranded legacy cache");
  library[127]=1;GpuDriverStore.Installed changed=GpuDriverStore.importDriver(context,new ByteArrayInputStream(packageZip(library,false)));check(!changed.directory.equals(old)&&changed.id().startsWith("sha256-"),"Changed driver aliased old cache");
  GpuDriverStore.select(context,changed.id());GpuDriverStore.Installed again=GpuDriverStore.importDriver(context,new ByteArrayInputStream(packageZip(library,true)));check(again.directory.equals(changed.directory),"Stable package identity changed");
  check(old.isDirectory(),"Old driver removed");
  File recipes=new File(context.getFilesDir(),"data/cache/shaders/pipelines_vk12_1.bin");PipelineWarmupSeeds.atomicWrite(recipes,extra);
  File save=new File(context.getFilesDir(),"save-sentinel");Files.write(save.toPath(),new byte[]{42});
  String name=PipelineWarmupSeeds.driverCacheName(changed.directory.getAbsolutePath()+"/",changed.metadata.libraryName);File current=new File(context.getCacheDir(),name),other=new File(context.getCacheDir(),"other-driver.bin");Files.write(current.toPath(),new byte[]{1});Files.write(other.toPath(),new byte[]{2});
  PipelineWarmupSeeds.prepare(context);check(PipelineWarmupSeeds.records(Files.readAllBytes(recipes.toPath())).size()==203,"Launch lost learned state");check(current.exists(),"Normal launch reset cache");
  MobilePresentation.prefs(context).edit().putBoolean(PipelineWarmupSeeds.RESET,true).commit();PipelineWarmupSeeds.prepare(context);
  check(!current.exists()&&other.exists()&&save.exists(),"Cold reset touched unrelated data");check(PipelineWarmupSeeds.records(Files.readAllBytes(recipes.toPath())).size()==202,"Cold reset did not isolate bundled coverage");check(!MobilePresentation.prefs(context).getBoolean(PipelineWarmupSeeds.RESET,true),"Cold reset not one shot");
  Files.write(current.toPath(),new byte[]{3});PipelineWarmupSeeds.prepare(context);check(current.exists(),"Second launch reset cache again");
  System.out.println("PASS: 202 recipes, corruption rejection, learned merge, legacy/stable driver reuse, changed-driver isolation, one-shot cold reset and save/cache preservation");
 }
}
