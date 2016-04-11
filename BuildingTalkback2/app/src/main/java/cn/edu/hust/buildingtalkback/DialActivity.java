package cn.edu.hust.buildingtalkback;

import cn.edu.hust.buildingtalkback.jni.Sip;
import cn.edu.hust.buildingtalkback.util.CallType;
import cn.edu.hust.buildingtalkback.util.CallRecord;
import cn.edu.hust.buildingtalkback.util.RecordCursorAdapter;
import android.app.Activity;
import android.database.Cursor;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;

public class DialActivity extends Activity implements OnClickListener, OnItemClickListener {

	private static final String LOG_TAG = "BT_DIALACTIVITY";
	
	private ListView lvCallHistory;
	private TextView tvDial;
	private Button btnDigits[];			//	hold buttons of digit 0~9
	private Button btnDial;
	private ImageButton ibtnBackspace;

	@Override
	protected void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_dial);

		initView();
	}

	protected void initView() {

		lvCallHistory = (ListView) findViewById(R.id.lv_call_history);
		tvDial = (TextView) findViewById(R.id.tv_dial);

		ibtnBackspace = (ImageButton) findViewById(R.id.ibtn_backspace);
		ibtnBackspace.setOnClickListener(this);
		
		btnDial = (Button) findViewById(R.id.btn_make_call);
		btnDial.setOnClickListener(this);

		btnDigits = new Button[10];
		int btnIds[] = {
				R.id.btn_0, R.id.btn_1, R.id.btn_2, R.id.btn_3, R.id.btn_4,
				R.id.btn_5, R.id.btn_6, R.id.btn_7, R.id.btn_8, R.id.btn_9
		};

		for (int i = 0; i < 10; i++) {

			btnDigits[i] = (Button) findViewById(btnIds[i]);
			btnDigits[i].setOnClickListener(this);
		}

	}
	
	@Override
	protected void onResume() {
		
		Cursor cursor = new CallRecord(this).getCallRecord(CallType.ALL);
		lvCallHistory.setAdapter(new RecordCursorAdapter(this, cursor));
		lvCallHistory.setOnItemClickListener(this);
		super.onResume();
	}

	@Override
	public void onClick(View view) {

		switch (view.getId()) {

		case R.id.ibtn_backspace:

			CharSequence cs = tvDial.getText();
			if (cs.length() != 0) {
				tvDial.setText(cs.subSequence(0, cs.length() - 1));
			}
			break;
			
		case R.id.btn_make_call:
			
			Sip.makeCall(tvDial.getText().toString());
			break;
			
		case R.id.btn_clear:
			
			tvDial.setText("");
			break;

		default:		//	Handles views in btnDigits[]
			tvDial.setText((String)tvDial.getText() + ((Button) view).getText());
			break;
		}		
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		
		TextView tvWho = (TextView) findViewById(R.id.tv_record_who);
		Sip.makeCall(tvWho.getText().toString());
	}

}
