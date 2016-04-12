package cn.edu.hust.buildingtalkback.util;

import cn.edu.hust.buildingtalkback.R;
import android.content.Context;
import android.database.Cursor;
import android.util.SparseIntArray;
import android.view.View;
import android.widget.ImageView;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;

public class RecordCursorAdapter extends SimpleCursorAdapter {

	private static final String FROM_STRS[] = {
		CallRecord.WHO,
		CallRecord.TIME,
		CallRecord.DURATION,
		CallRecord.TYPE
	};

	private static final int TO_IDS[] = {
		R.id.tv_record_who,
		R.id.tv_record_time,
		R.id.tv_record_duration,
		R.id.iv_record_type
	};
	
	private static final int CALL_TYPE_IDS[] = {
		CallType.INCOMING.getInt(),
		CallType.OUTGOING.getInt(),
		CallType.MISSED.getInt()
	};
	
	private static final int DRAWABLE_IDS[] = {
		android.R.drawable.sym_call_incoming,
		android.R.drawable.sym_call_outgoing,
		android.R.drawable.sym_call_missed,
	};
	
	private static SparseIntArray map;
	
	private String hour;
	private String minute;
	private String second;
	
	static {

		map = new SparseIntArray();
		for (int i = 0; i < CALL_TYPE_IDS.length; i++)
			map.put(CALL_TYPE_IDS[i], DRAWABLE_IDS[i]);
	}

	public RecordCursorAdapter(Context context, Cursor cursor) {

		super(context, R.layout.record_item, cursor, FROM_STRS, TO_IDS, 0);
		
		hour = context.getString(R.string.hour);
		minute = context.getString(R.string.minute);
		second = context.getString(R.string.second);

		setViewBinder(new ViewBinder() {

			@Override
			public boolean setViewValue(View view, Cursor cursor, int colume) {

				switch (view.getId()) {
				
				//	call type
				case R.id.iv_record_type:
					((ImageView) view).setImageResource(map.get(cursor.getInt(colume)));
					break;

				//	call duration
				case R.id.tv_record_duration:
					int duration = cursor.getInt(colume);
					((TextView) view).setText(convertDurationToString(duration));
					break;
					
				//	call from and when
				default:
					((TextView) view).setText(cursor.getString(colume));
					break;
				}

				return true;
			}
		});
	}

	private String convertDurationToString(int duration) {

		StringBuilder builder = new StringBuilder();

		if (duration >= 3600)
			builder.append((duration / 3600) + hour);
		if (duration >= 60)
			builder.append((duration % 3600 / 60) + minute);
		builder.append((duration % 60) + second);
		
		return builder.toString();
	}
}
