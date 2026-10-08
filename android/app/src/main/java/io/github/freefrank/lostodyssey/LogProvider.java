package io.github.freefrank.lostodyssey;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import java.io.File;
import java.io.FileNotFoundException;

/** Read-only, URI-granted access to exactly one diagnostic snapshot. */
public final class LogProvider extends ContentProvider {
    public static Uri logUri(Context context) {
        return Uri.parse("content://" + context.getPackageName() + ".logs/android-phase1.log");
    }
    private File file(Uri uri) {
        if (!logUri(getContext()).equals(uri)) throw new IllegalArgumentException("Unknown log URI");
        return new File(getContext().getCacheDir(), "log-export/android-phase1.log");
    }
    @Override public boolean onCreate() { return true; }
    @Override public String getType(Uri uri) { file(uri); return "text/plain"; }
    @Override public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!"r".equals(mode)) throw new FileNotFoundException("Read-only log export");
        return ParcelFileDescriptor.open(file(uri), ParcelFileDescriptor.MODE_READ_ONLY);
    }
    @Override public Cursor query(Uri uri, String[] projection, String selection,
                                  String[] selectionArgs, String sortOrder) {
        File snapshot = file(uri);
        String[] columns = projection == null
            ? new String[] { OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE } : projection;
        MatrixCursor cursor = new MatrixCursor(columns, 1);
        Object[] values = new Object[columns.length];
        for (int i = 0; i < columns.length; ++i) {
            if (OpenableColumns.DISPLAY_NAME.equals(columns[i])) values[i] = "android-phase1.log";
            else if (OpenableColumns.SIZE.equals(columns[i])) values[i] = snapshot.length();
        }
        cursor.addRow(values);
        return cursor;
    }
    @Override public Uri insert(Uri uri, ContentValues values) { throw new UnsupportedOperationException("Read-only"); }
    @Override public int update(Uri uri, ContentValues values, String selection, String[] args) { throw new UnsupportedOperationException("Read-only"); }
    @Override public int delete(Uri uri, String selection, String[] args) { throw new UnsupportedOperationException("Read-only"); }
}
