package io.github.freefrank.lostodyssey;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import org.json.JSONObject;

/** Small install records only; never scans or hashes large game archives on the UI thread. */
final class ContentInventory {
    static String pendingDlc(File files){
        StringBuilder text=new StringBuilder();File pending=new File(files,"pending-content");File[] markers=pending.listFiles((dir,name)->name.endsWith(".ready"));
        if(markers!=null){Arrays.sort(markers);for(File marker:markers){String n=marker.getName();append(text,new File(pending,n.substring(0,n.length()-6)+"/dlc"),"restart required");}}
        return text.toString().trim();
    }
    static String dlc(File files,boolean setup){
        StringBuilder text=new StringBuilder();
        append(text,new File(files,"game/dlc"),"installed");
        File pending=new File(files,"pending-content");File[] markers=pending.listFiles((dir,name)->name.endsWith(".ready"));
        if(markers!=null){Arrays.sort(markers);for(File marker:markers){String n=marker.getName();append(text,new File(pending,n.substring(0,n.length()-6)+"/dlc"),setup?"ready — install when setup finishes":"ready — restart required");}}
        return text.length()==0?"DLC: none added (optional)":text.toString().trim();
    }
    private static void append(StringBuilder out,File root,String status){
        File[] folders=root.listFiles(File::isDirectory);if(folders==null)return;Arrays.sort(folders);
        for(File folder:folders){File record=new File(folder,".lo-dlc.json");if(!record.isFile())continue;String name=folder.getName();
            if(record.length()<=512*1024)try(InputStream in=new FileInputStream(record)){ByteArrayOutputStream bytes=new ByteArrayOutputStream();byte[] b=new byte[8192];int n;while((n=in.read(b))!=-1){if(bytes.size()+n>512*1024)throw new IOException("Record too large");bytes.write(b,0,n);}JSONObject json=new JSONObject(new String(bytes.toByteArray(),StandardCharsets.UTF_8));String display=json.optString("display_name","").trim();if(!display.isEmpty())name=display;}catch(Exception ignored){}
            out.append("DLC · ").append(name).append(": ").append(status).append('\n');
        }
    }
}
