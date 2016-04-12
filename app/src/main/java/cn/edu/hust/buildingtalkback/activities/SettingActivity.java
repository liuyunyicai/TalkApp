package cn.edu.hust.buildingtalkback.activities;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.sip.SipEvent;
import de.greenrobot.event.EventBus;

public class SettingActivity extends PreferenceActivity {
	private SharedPreferences shared;

	private String last_serverIp;
	private String last_username;
	private String last_password;

    // 滑动距离
    private float mTouchSlop;

	@SuppressWarnings("deprecation")
	@Override
	protected void onCreate(Bundle savedInstanceState) {

		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.setting);

        mTouchSlop = ViewConfiguration.get(this).getScaledTouchSlop();
		// 先保存未修改之前值
		shared = PreferenceManager.getDefaultSharedPreferences(this);

		last_username = shared.getString(getString(R.string.key_username), "");
		last_password = shared.getString(getString(R.string.key_password), "");
		last_serverIp = shared.getString(getString(R.string.key_serverip), "");
	}

	@Override
	protected void onDestroy() {
		// 获取修改后的结果
		String now_username = shared.getString(getString(R.string.key_username), "");
		String now_password = shared.getString(getString(R.string.key_password), "");
		String now_serverIp = shared.getString(getString(R.string.key_serverip), "");

		// 判断结果值是否修改
		boolean isIpChanged = (!last_username.equals(now_username)) ||
				(!last_password.equals(now_password)) ||
				(!last_serverIp.equals(now_serverIp));

        // 如果发生改变则提示重新连接
		if (isIpChanged) {
//            EventBus.getDefault().post(new SipEvent(SipEvent.SIP_UNREGISTER));
            // 通知SipService去使用新IP重新连接服务器
            EventBus.getDefault().post(new SipEvent(SipEvent.SIP_REGISTER));
        }

		super.onDestroy();
	}

}
