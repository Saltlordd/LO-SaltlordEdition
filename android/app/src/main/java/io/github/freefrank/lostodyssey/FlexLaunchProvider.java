package io.github.freefrank.lostodyssey;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;

/** One-use, in-memory handoff in the main process. Never stores a Flex default. */
public final class FlexLaunchProvider extends ContentProvider {
    private static String pending;
    private static int pendingMode;
    static synchronized String issue(int mode){
        pending=java.util.UUID.randomUUID().toString();pendingMode=FlexLayoutPolicy.valid(mode);return pending;
    }
    static int consume(Context context,Intent intent){
        String token=intent.getStringExtra("flex-token");intent.removeExtra("flex-token");
        if(token==null)return 0;
        try{
            Bundle result=context.getContentResolver().call(Uri.parse("content://"+context.getPackageName()+".flex"),"consume",token,null);
            return result==null?0:FlexLayoutPolicy.valid(result.getInt("mode",0));
        }catch(RuntimeException e){android.util.Log.w("LO.Flex","Session handoff unavailable; using fullscreen",e);return 0;}
    }
    @Override public synchronized Bundle call(String method,String token,Bundle extras){
        Bundle result=new Bundle();int mode=0;
        synchronized(FlexLaunchProvider.class){
            if("consume".equals(method)&&pending!=null&&pending.equals(token)){
                mode=pendingMode;pending=null;pendingMode=0;
            }
        }
        result.putInt("mode",mode);return result;
    }
    @Override public boolean onCreate(){return true;}
    @Override public Cursor query(Uri uri,String[] projection,String selection,String[] args,String order){return null;}
    @Override public String getType(Uri uri){return null;}
    @Override public Uri insert(Uri uri,ContentValues values){return null;}
    @Override public int delete(Uri uri,String selection,String[] args){return 0;}
    @Override public int update(Uri uri,ContentValues values,String selection,String[] args){return 0;}
}
