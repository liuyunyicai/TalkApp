package cn.edu.hust.buildingtalkback;

import cn.edu.hust.buildingtalkback.jni.Sip;
import cn.edu.hust.buildingtalkback.util.CallType;
import cn.edu.hust.buildingtalkback.util.CallRecord;
import cn.edu.hust.buildingtalkback.util.RecordCursorAdapter;
import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

public class RecordActivity extends Activity implements OnClickListener, OnItemClickListener {

	private Button btnAll;
	private Button btnIncoming;
	private Button btnOutgoing;
	private Button btnMissed;
	private Button btnClear;
	private ListView lvRecord;

	private CallRecord history;
	private CallType callType;

	RecordCursorAdapter adapter;

	@Override
	protected void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_record);

		history = new CallRecord(this);
		adapter = new RecordCursorAdapter(this, null);
		
		initView();
	}
	
	
	private void initView() {

		btnAll = (Button) findViewById(R.id.btn_call_all);
		btnAll.setOnClickListener(this);

		btnIncoming = (Button) findViewById(R.id.btn_call_incoming);
		btnIncoming.setOnClickListener(this);

		btnOutgoing = (Button) findViewById(R.id.btn_call_outgoing);
		btnOutgoing.setOnClickListener(this);

		btnMissed = (Button) findViewById(R.id.btn_call_missed);
		btnMissed.setOnClickListener(this);

		btnClear = (Button) findViewById(R.id.btn_call_clear);
		btnClear.setOnClickListener(this);

		lvRecord = (ListView) findViewById(R.id.lv_record);
		lvRecord.setOnItemClickListener(this);
		callType = CallType.ALL;
	}
	
	

	@Override
	protected void onResume() {

		super.onResume();
		showRecordList(callType);
	}


	@Override
	public void onClick(View view) {

		switch (view.getId()) {

		case R.id.btn_call_all:
			callType = CallType.ALL;
			break;

		case R.id.btn_call_incoming:
			callType = CallType.INCOMING;
			break;

		case R.id.btn_call_outgoing:
			callType = CallType.OUTGOING;
			break;

		case R.id.btn_call_missed:
			callType = CallType.MISSED;
			break;

		case R.id.btn_call_clear:
			history.clearHitory();
			break;

		default:
			break;
		}

		showRecordList(callType);
	}

	private void showRecordList(CallType type) {
		
		adapter.changeCursor(history.getCallRecord(type));
		lvRecord.setAdapter(adapter);
	}


	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		
		TextView tvWho = (TextView) findViewById(R.id.tv_record_who);
		Sip.makeCall(tvWho.getText().toString());
	}
}
