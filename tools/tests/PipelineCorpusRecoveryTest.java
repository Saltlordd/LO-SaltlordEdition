package io.github.freefrank.lostodyssey;
import java.nio.file.*;import java.io.*;import java.util.*;
public class PipelineCorpusRecoveryTest {
 static void check(boolean b,String m){if(!b)throw new AssertionError(m);}
 static void reject(byte[] b)throws Exception{try{PipelineWarmupSeeds.validateCorpus(b);throw new AssertionError("Accepted invalid corpus");}catch(IOException expected){}}
 public static void main(String[] args)throws Exception{
  byte[] corpus=Files.readAllBytes(Paths.get(args[0]));check(PipelineWarmupSeeds.validateCorpus(corpus)==3974,"count");
  reject(Arrays.copyOf(corpus,corpus.length-1));byte[] bad=corpus.clone();bad[8]=1;reject(bad);
  bad=corpus.clone();bad[100]^=1;PipelineWarmupSeeds.buffer(bad).putLong(32,PipelineWarmupSeeds.hash(bad,48,bad.length-48));PipelineWarmupSeeds.buffer(bad).putLong(40,PipelineWarmupSeeds.hash(bad,0,40));reject(bad);
  File root=Files.createTempDirectory("corpus-recovery-").toFile();android.content.Context c=new android.content.Context(root,corpus);
  File learned=new File(c.getFilesDir(),"data/cache/shaders/pipelines_vk12_1.bin");learned.getParentFile().mkdirs();Files.write(learned.toPath(),corpus);
  File driver=new File(c.getCacheDir(),"driver-cache.bin");Files.write(driver.toPath(),new byte[]{7,8});File save=new File(c.getFilesDir(),"save.dat");Files.write(save.toPath(),new byte[]{9});
  MobilePresentation.prefs(c).edit().putBoolean(PipelineWarmupSeeds.RESET,true).commit();PipelineWarmupSeeds.prepare(c);
  check(Arrays.equals(corpus,Files.readAllBytes(learned.toPath())),"learned v2 overwritten");check(Arrays.equals(new byte[]{7,8},Files.readAllBytes(driver.toPath()))&&save.isFile(),"cache/save removed");
  check(!MobilePresentation.prefs(c).getBoolean(PipelineWarmupSeeds.RESET,true),"reset armed");
  Files.write(learned.toPath(),new byte[]{1,2,3,4});PipelineWarmupSeeds.prepare(c);check(Arrays.equals(new byte[]{1,2,3,4},Files.readAllBytes(learned.toPath())),"opaque learned overwritten");
  File installed=new File(c.getFilesDir(),"data/cache/shaders/pipelines_corpus.bin");check(Arrays.equals(corpus,Files.readAllBytes(installed.toPath())),"corpus mismatch");long time=installed.lastModified();PipelineWarmupSeeds.prepare(c);check(time==installed.lastModified(),"rewritten");
  System.out.println("PASS: 3974 records, corrupt/schema/truncated rejection, separate install, learned/save/driver preservation, reset disarming, repeat install");
 }
}
