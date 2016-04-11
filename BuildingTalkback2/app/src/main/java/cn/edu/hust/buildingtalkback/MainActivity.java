package cn.edu.hust.buildingtalkback;

import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.jni.NativeInterface;
import cn.edu.hust.buildingtalkback.jni.Sip;
import android.R.integer;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class MainActivity extends Activity implements OnClickListener {

	private static final String LOG_TAG = "BT_MAINACTIVITY";
	
	private String username;
	private String password;
	private String serverIp;

	private Button btnDial;
	private Button btnRecord;
	private Button btnSetting;

	@Override
	protected void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_main);

		initView();
		
		NativeInterface.setActivity(this);

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);

		username = prefs.getString(getString(R.string.key_username), "");
		password = prefs.getString(getString(R.string.key_password), "");
		serverIp = prefs.getString(getString(R.string.key_serverip), "");
		
		new Thread(new Runnable() {
			@Override
			public void run() {
				Sip.register(serverIp, username, password);
			}
		}).start();
	}	

	protected void initView() {

		btnDial = (Button) findViewById(R.id.btn_make_call);
		btnDial.setOnClickListener(this);

		btnRecord = (Button) findViewById(R.id.btn_record_query);
		btnRecord.setOnClickListener(this);

		btnSetting = (Button) findViewById(R.id.btn_setting);
		btnSetting.setOnClickListener(this);
	}


	@Override
	public boolean onCreateOptionsMenu(Menu menu) {

		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}


	@Override
	public boolean onOptionsItemSelected(MenuItem item) {

		int id = item.getItemId();
		if (id == R.id.action_settings) {
			startActivity(new Intent(this, SettingActivity.class));
			return true;
		}
		return super.onOptionsItemSelected(item);
	}


	@Override
	public void onClick(View view) {

		switch (view.getId()) {

		case R.id.btn_make_call:
			startActivity(new Intent(this, DialActivity.class));
			break;

		case R.id.btn_record_query:
			startActivity(new Intent(this, RecordActivity.class));
			break;

		case R.id.btn_setting:
			startActivity(new Intent(this, SettingActivity.class));

		default:
			break;
		}
	}
}
