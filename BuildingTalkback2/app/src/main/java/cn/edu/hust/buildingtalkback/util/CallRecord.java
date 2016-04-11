package cn.edu.hust.buildingtalkback.util;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.provider.BaseColumns;

public class CallRecord extends SQLiteOpenHelper {

	private static final String LOG_TAG = "BT_CallRecord";

	private static final String DATABASE_NAME = "history.db";
	private static final int DATABASE_VERSION = 1;

	private static final String TABLE_NAME = "call_history";

	public static final String _ID = BaseColumns._ID;
	public static final String WHO = "who";
	public static final String TIME = "time";
	public static final String TYPE = "type";
	public static final String DURATION = "duration";

	public CallRecord(Context context) {
		super(context, DATABASE_NAME, null, DATABASE_VERSION);
	}

	@Override
	public void onCreate(SQLiteDatabase db) {

		db.execSQL("create table " + TABLE_NAME +
				" (" + _ID + " integer primary key autoincrement, " +
				WHO + " text not null, " +
				TIME + " text not null, " +
				TYPE + " integer not null, " +
				DURATION + " integer not null);");
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		
	}

	private void addCall(String who, String time, CallType type, int duration) {

		SQLiteDatabase db = getWritableDatabase();

		ContentValues values = new ContentValues();

		values.put(WHO, who);
		values.put(TIME, time);
		values.put(TYPE, type.getInt());
		values.put(DURATION, duration);

		db.insert(TABLE_NAME, null, values);
	}
	
	public void addIncomingCall(String who, String time, int duration) {
		addCall(who, time, CallType.INCOMING, duration);
	}
	
	public void addOutgoingCall(String who, String time, int duration) {
		addCall(who, time, CallType.OUTGOING, duration);
	}
	
	public void addMissedCall(String who, String time) {
		addCall(who, time, CallType.MISSED, 0);
	}

	public Cursor getCallRecord(CallType type) {
		
		String sql = "select * from " + TABLE_NAME;

		if (type != CallType.ALL) {
			sql += " where type='" + type.getInt() + "'";
		}

		return getReadableDatabase().rawQuery(sql, null);
	}

	public void clearHitory() {
		getWritableDatabase().delete(TABLE_NAME, null, null);
	}
}
