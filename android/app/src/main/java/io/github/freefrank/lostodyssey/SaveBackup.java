package io.github.freefrank.lostodyssey;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;

/** Ordinary game saves only. No game assets, settings, cache or emulator state. */
final class SaveBackup {
    static final long MAX_FILE=16L*1024*1024, MAX_TOTAL=128L*1024*1024;
    static final int MAX_ENTRIES=1024;
    private static final Set<String> FILES=new HashSet<>(Arrays.asList("save.bin",".lo-content",".lo-thumbnail.png","__thumbnail.png"));
    static File root(File files){return new File(files,"data/save");}
    static File previous(File files){return new File(files,"save-backups/before-last-restore.zip");}
    static final class Plan implements Closeable {
        final File staged;
        final List<String> slots;
        final Set<String> overwritten;
        Plan(File staged,List<String> slots,Set<String> overwritten){this.staged=staged;this.slots=Collections.unmodifiableList(slots);this.overwritten=Collections.unmodifiableSet(overwritten);}
        public void close() throws IOException {remove(staged);}
    }
    static boolean slot(String name){return name.matches("user[0-9]{2}");}
    private static void safe(File file) throws IOException {
        if(Files.isSymbolicLink(file.toPath()))throw new IOException("Links are not supported in save folders");
    }
    private static List<File> children(File folder) throws IOException {
        safe(folder);File[] entries=folder.listFiles();if(entries==null)throw new IOException("Cannot read save folder");
        List<File> list=Arrays.asList(entries);list.sort(Comparator.comparing(File::getName));return list;
    }
    static List<String> slots(File saves) throws IOException {
        List<String> result=new ArrayList<>();safe(saves);if(!saves.exists())return result;
        for(File dir:children(saves))if(slot(dir.getName())){safe(dir);if(!dir.isDirectory())throw new IOException("Invalid save slot: "+dir.getName());safe(new File(dir,"save.bin"));if(new File(dir,"save.bin").isFile())result.add(dir.getName());}
        return result;
    }
    static void export(File saves,OutputStream output) throws IOException {
        List<String> names=slots(saves);if(names.isEmpty())throw new IOException("No saved games found yet");
        exportNames(saves,names,output);
    }
    private static void exportNames(File saves,List<String> names,OutputStream output) throws IOException {
        long total=0;
        try(ZipOutputStream zip=new ZipOutputStream(output)){
            for(String name:names){File dir=new File(saves,name);ZipEntry folder=new ZipEntry("save/"+name+"/");folder.setTime(dir.lastModified());zip.putNextEntry(folder);zip.closeEntry();
                for(File file:children(dir)){
                    if(!FILES.contains(file.getName()))throw new IOException("Unexpected file in "+name+": "+file.getName());
                    safe(file);if(!file.isFile()||file.length()>MAX_FILE)throw new IOException("Invalid save file");total+=file.length();if(total>MAX_TOTAL)throw new IOException("Save backup is too large");
                    ZipEntry e=new ZipEntry("save/"+name+"/"+file.getName());e.setTime(file.lastModified());zip.putNextEntry(e);
                    try(InputStream in=new FileInputStream(file)){copy(in,zip,MAX_FILE);}zip.closeEntry();
                }
            }
        }
    }
    private static void mkdir(File dir) throws IOException {safe(dir);if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Cannot create save folder");}
    private static String[] path(String raw) throws IOException {
        if(raw.isEmpty()||raw.length()>512||raw.startsWith("/")||raw.indexOf('\\')>=0||raw.indexOf(':')>=0||raw.indexOf('\0')>=0)throw new IOException("Unsafe path in backup");
        String name=raw.endsWith("/")?raw.substring(0,raw.length()-1):raw;
        String[] parts=name.split("/",-1);for(String p:parts)if(p.isEmpty()||p.equals(".")||p.equals(".."))throw new IOException("Unsafe path in backup");return parts;
    }
    static Plan prepare(InputStream input,File saves,File stagingParent) throws IOException {
        mkdir(stagingParent);File stage=new File(stagingParent,"saves-"+UUID.randomUUID());mkdir(stage);
        Map<String,Long> times=new HashMap<>();Map<String,String> origins=new HashMap<>();Set<String> paths=new HashSet<>();long total=0;int count=0;
        try{
            try(ZipInputStream zip=new ZipInputStream(input)){
                ZipEntry entry;
                while((entry=zip.getNextEntry())!=null){if(++count>MAX_ENTRIES)throw new IOException("Too many files in backup");String[] parts=path(entry.getName());if(entry.isDirectory()&&zip.read()!=-1)throw new IOException("Directory entries must be empty");
                    int index=-1;for(int i=0;i<parts.length;i++)if(slot(parts[i])){if(index!=-1)throw new IOException("Ambiguous save slot path");index=i;}
                    if(index<0){if(entry.isDirectory())continue;throw new IOException("Choose a ZIP containing user00-style save folders");}
                    String name=parts[index],origin=String.join("/",Arrays.copyOf(parts,index+1));
                    if(origins.containsKey(name)&&!origins.get(name).equals(origin))throw new IOException("Duplicate save slot: "+name);origins.put(name,origin);
                    File dir=new File(stage,name);mkdir(dir);if(entry.isDirectory()){if(parts.length!=index+1)throw new IOException("Nested save folders are unsupported");if(entry.getTime()>0)times.put(name,entry.getTime());continue;}
                    if(parts.length!=index+2||!FILES.contains(parts[index+1]))throw new IOException("Unexpected file in save backup");
                    String filename=parts[index+1].equals("__thumbnail.png")?".lo-thumbnail.png":parts[index+1];
                    if(!paths.add(name+"/"+filename))throw new IOException("Duplicate save file");
                    if(entry.getSize()>MAX_FILE)throw new IOException("Save file is too large");File file=new File(dir,filename);
                    try(FileOutputStream out=new FileOutputStream(file)){total+=copy(zip,out,MAX_FILE);out.getFD().sync();}
                    if(total>MAX_TOTAL)throw new IOException("Save backup is too large");if(entry.getTime()>0){file.setLastModified(entry.getTime());if(filename.equals("save.bin"))times.put(name,entry.getTime());}zip.closeEntry();
                }
            }
            List<String> names=slots(stage);if(names.isEmpty()||names.size()!=origins.size())throw new IOException("Each slot must contain save.bin");
            Set<String> overwritten=new TreeSet<>();
            for(String name:names){File dir=new File(stage,name),save=new File(dir,"save.bin");if(save.length()<4)throw new IOException("Empty or incomplete save: "+name);
                try(InputStream in=new FileInputStream(save)){byte[] magic=new byte[4];if(in.read(magic)!=4)throw new IOException("Incomplete save");String header=new String(magic,java.nio.charset.StandardCharsets.US_ASCII);if(header.equals("CON ")||header.equals("LIVE")||header.equals("PIRS"))throw new IOException("Xbox 360 containers need extraction first");}
                File metadata=new File(dir,".lo-content");if(metadata.exists())validateMetadata(metadata,name);else try(FileOutputStream out=new FileOutputStream(metadata)){out.write(contentData(name));out.getFD().sync();}
                if(times.containsKey(name))dir.setLastModified(times.get(name));if(new File(saves,name).exists())overwritten.add(name);
            }
            return new Plan(stage,names,overwritten);
        }catch(IOException|RuntimeException e){try{remove(stage);}catch(IOException cleanup){e.addSuppressed(cleanup);}throw e;}
    }
    static byte[] contentData(String name) throws IOException {
        if(!slot(name))throw new IOException("Unsupported slot name");byte[] data=new byte[308];ByteBuffer b=ByteBuffer.wrap(data).order(ByteOrder.BIG_ENDIAN);b.putInt(1).putInt(1);
        byte[] display=name.getBytes(java.nio.charset.StandardCharsets.UTF_16BE),file=name.getBytes(java.nio.charset.StandardCharsets.US_ASCII);System.arraycopy(display,0,data,8,display.length);System.arraycopy(file,0,data,264,file.length);return data;
    }
    private static void validateMetadata(File file,String name) throws IOException {
        if(file.length()!=308)throw new IOException("Invalid save metadata for "+name);byte[] data=Files.readAllBytes(file.toPath());
        if(ByteBuffer.wrap(data).order(ByteOrder.BIG_ENDIAN).getInt(4)!=1)throw new IOException("Not game-save metadata");int end=264;while(end<306&&data[end]!=0)end++;
        if(end==306||!new String(data,264,end-264,java.nio.charset.StandardCharsets.US_ASCII).equals(name))throw new IOException("Save metadata does not match its slot");
    }
    /** Replay-safe directory transaction. Unrelated existing slots are preserved. */
    static void install(Plan plan,File saves,File backup) throws IOException {
        recover(saves);File parent=saves.getParentFile();mkdir(parent);File next=new File(parent,".save-restore-new"),old=new File(parent,".save-restore-old");
        remove(next);mkdir(next);
        try{
            if(saves.exists())copyTree(saves,next);
            for(String name:plan.slots){File target=new File(next,name);remove(target);copyTree(new File(plan.staged,name),target);}
            List<String> existing=slots(saves);
            if(!existing.isEmpty()){mkdir(backup.getParentFile());File temp=new File(backup.getParentFile(),"before-last-restore.tmp");try(FileOutputStream out=new FileOutputStream(temp)){exportNames(saves,existing,out);}Files.move(temp.toPath(),backup.toPath(),StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE);}
            // Old exists only while a restore is incomplete; recover rolls it back.
            if(!saves.exists())mkdir(saves);
            Files.move(saves.toPath(),old.toPath(),StandardCopyOption.ATOMIC_MOVE);
            try{Files.move(next.toPath(),saves.toPath(),StandardCopyOption.ATOMIC_MOVE);}
            catch(IOException e){Files.move(old.toPath(),saves.toPath(),StandardCopyOption.ATOMIC_MOVE);throw e;}
            // Rename is the commit marker; a crash before this rolls back on launch.
            File committed=new File(parent,".save-restore-committed");Files.move(old.toPath(),committed.toPath(),StandardCopyOption.ATOMIC_MOVE);
            try{remove(committed);}catch(IOException ignored){} // recovered on next launch
        }catch(IOException|RuntimeException e){recover(saves);throw e;}
        finally{remove(next);}
    }
    static void recover(File saves) throws IOException {
        File parent=saves.getParentFile(),old=new File(parent,".save-restore-old");safe(parent);safe(saves);safe(old);
        if(old.exists()){remove(saves);Files.move(old.toPath(),saves.toPath(),StandardCopyOption.ATOMIC_MOVE);}
        remove(new File(parent,".save-restore-new"));remove(new File(parent,".save-restore-committed"));
    }
    private static void copyTree(File from,File to) throws IOException {
        safe(from);if(from.isDirectory()){mkdir(to);for(File f:children(from))copyTree(f,new File(to,f.getName()));}
        else{if(!from.isFile()||from.length()>MAX_FILE)throw new IOException("Invalid existing save file");try(InputStream in=new FileInputStream(from);FileOutputStream out=new FileOutputStream(to)){copy(in,out,MAX_FILE);out.getFD().sync();}}to.setLastModified(from.lastModified());
    }
    private static long copy(InputStream in,OutputStream out,long max) throws IOException {byte[] buf=new byte[32768];long size=0;int n;while((n=in.read(buf))!=-1){size+=n;if(size>max)throw new IOException("Save file is too large");out.write(buf,0,n);}return size;}
    static void remove(File file) throws IOException {safe(file);if(!file.exists())return;if(file.isDirectory())for(File child:children(file))remove(child);if(!file.delete())throw new IOException("Cannot remove staging folder");}
}
