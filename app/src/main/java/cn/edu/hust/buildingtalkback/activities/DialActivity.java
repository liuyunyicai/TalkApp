package cn.edu.hust.buildingtalkback.activities;

import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.iinterface.ISip;
import cn.edu.hust.buildingtalkback.ringing.CalloutRingActivity;
import cn.edu.hust.buildingtalkback.sip.Sip;
import cn.edu.hust.buildingtalkback.sip.SipService;
import cn.edu.hust.buildingtalkback.util.CallType;
import cn.edu.hust.buildingtalkback.util.CallRecord;
import cn.edu.hust.buildingtalkback.util.RecordCursorAdapter;

import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.media.Image;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class DialActivity extends AppCompatActivity implements OnClickListener, OnItemClickListener {

	private static final String LOG_TAG = "BT_DIALACTIVITY";

	private ListView lvCallHistory;
	private TextView tvDial;
	private LinearLayout btnDigits[];			//	hold buttons of digit 0~9
	private ImageView btnDial;
    private ImageView btnClear;           // 清空按钮
	private ImageButton ibtnBackspace; // 回退按钮

    private DrawerLayout myDrawerLayout;

	// ToolBar界面
	private Toolbar toolbar;
	private Button returnBt;
    private TextView toolbar_title;

	private ISip sip;

    private String user_name;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_dial);

		sip = Sip.getInstance();
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        user_name = prefs.getString(getString(R.string.key_username), "");

		initView();
	}

	protected void initView() {
		initToorBar();
        myDrawerLayout = $(R.id.my_drawer_layout);

        lvCallHistory = $(R.id.lv_call_history);
        tvDial = $(R.id.tv_dial);
        ibtnBackspace = $(R.id.ibtn_backspace);
        btnDial = $(R.id.btn_make_call);
        lvCallHistory = $(R.id.lv_call_history);
        btnClear = $(R.id.btn_clear);

		ibtnBackspace.setOnClickListener(this);
		btnDial.setOnClickListener(this);
        btnClear.setOnClickListener(this);

		btnDigits = new LinearLayout[10];
		int btnIds[] = {
				R.id.btn_0, R.id.btn_1, R.id.btn_2, R.id.btn_3, R.id.btn_4,
				R.id.btn_5, R.id.btn_6, R.id.btn_7, R.id.btn_8, R.id.btn_9
		};

		String[] bt_txts = {"#", "@", "ABC", "DEF" ,"GHI" ,"JKL" ,"MNO" ,"PQRS", "TUV", "WXYZ"};

		for (int i = 0; i < 10; i++) {
			btnDigits[i] = $(btnIds[i]);
			btnDigits[i].setOnClickListener(this);

            ((TextView) btnDigits[i].findViewById(R.id.dial_num_text)).setText(String.valueOf(i));
            ((TextView) btnDigits[i].findViewById(R.id.dial_txt_text)).setText(bt_txts[i]);
		}
	}



	// 初始化ToolBar
	private void initToorBar() {
		toolbar = $(R.id.toolbar);
		toolbar.setTitle("");
		setSupportActionBar(toolbar);

        toolbar_title = $(R.id.toolbar_title);
        toolbar_title.setText(R.string.call_title);

		returnBt = $(R.id.return_bt);
		returnBt.setOnClickListener(this);
	}

	public <T> T $(int resId) {
        return (T) findViewById(resId);
    }

	@Override
	protected void onResume() {
		Cursor cursor = new CallRecord(this).getCallRecord(CallType.ALL);
		lvCallHistory.setAdapter(new RecordCursorAdapter(this, cursor));
		lvCallHistory.setOnItemClickListener(this);
		super.onResume();

        // 每一次resume中注意重新注册一次
        startService(new Intent(this, SipService.class));
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

        // 呼叫指定用户
		case R.id.btn_make_call:
            String targetName = tvDial.getText().toString().trim();
            if (user_name.equals(targetName))
                Toast.makeText(DialActivity.this, R.string.wrong_call_self, Toast.LENGTH_SHORT).show();
            else if (targetName.length() == 0)
                Toast.makeText(DialActivity.this, R.string.wrong_call_name, Toast.LENGTH_SHORT).show();
            else {
				// 拨打电话
				sip.makeCall(targetName);
                // 直接跳转到拨号界面
                Intent intent = new Intent(this, CalloutRingActivity.class);
                intent.putExtra("other", targetName);
                startActivity(intent);
			}

			break;

		case R.id.btn_clear:
			tvDial.setText("");
			break;

        case R.id.return_bt:
            this.finish();
            break;

		default:		//	Handles views in btnDigits[]
            String pre_text = tvDial.getText().toString();
            String num_text = ((TextView)view.findViewById(R.id.dial_num_text)).getText().toString();

			tvDial.setText(pre_text + num_text);
			break;
		}
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		TextView tvWho = (TextView) findViewById(R.id.tv_record_who);
		sip.makeCall(tvWho.getText().toString());
	}

}
