package io.github.freefrank.lostodyssey;
import java.io.*;import java.nio.file.*;import java.util.*;
public final class PipelineWarmupReportTest {
 public static void main(String[] args)throws Exception{
  File f=Files.createTempFile("warmup-test",".bin").toFile();ByteArrayOutputStream out=new ByteArrayOutputStream();
  byte[] sample={1,2,3,4,5};Files.write(f.toPath(),sample);PipelineWarmupReport.append(f,out);
  String report=out.toString("UTF-8");if(!report.contains(Base64.getEncoder().encodeToString(sample)))throw new AssertionError();
  out.reset();try(RandomAccessFile file=new RandomAccessFile(f,"rw")){file.setLength(PipelineWarmupReport.MAX_BYTES+1L);}
  PipelineWarmupReport.append(f,out);if(!out.toString("UTF-8").contains("omitted"))throw new AssertionError();
  f.delete();out.reset();PipelineWarmupReport.append(f,out);if(out.size()!=0)throw new AssertionError();
  System.out.println("PASS: metadata roundtrip, size bound and missing file");
 }
}
