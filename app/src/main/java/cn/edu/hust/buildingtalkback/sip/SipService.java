package cn.edu.hust.buildingtalkback.sip;

import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import cn.edu.hust.buildingtalkback.main.MainActivity;
import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.iinterface.ISip;
import cn.edu.hust.buildingtalkback.main.MainEvent;
import cn.edu.hust.buildingtalkback.ringing.CalloutRingActivity;
import de.greenrobot.event.EventBus;

/**
 * Created by admin on 2015/12/30.
 */
public class SipService extends Service {
    private SharedPreferences share;
    private ExecutorService executors;

    // SIP 相关参数
    public static int Sip_Conn_State = ISip.SIP_UNREGISTER; // SIP连接状态
    public static int Sip_Call_State = ISip.SIP_CALL_FREE; // SIP通话状态

    private ISip sip;

    // SIP环境是否已创建，即servie是否是首次使用SIP环境
    private static int SIP_INITED = -1;

    @Override
    public void onCreate() {
        super.onCreate();
        // 注册EventBus
        EventBus.getDefault().register(this);
        share = PreferenceManager.getDefaultSharedPreferences(this);
        executors = Executors.newCachedThreadPool();

        initSip();
        Log.i(MainActivity.LOG_TAG, "Service onCreate!");

//        // 注册Sip
//        sipRegister();
    }

    private void initSip() {
        sip = Sip.getInstance();
        // 初始化SIP环境
        SIP_INITED = sip.sip_init();
//        Log.w(MainActivity.LOG_TAG, "SIP_INITED == " + SIP_INITED);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // 面对上次未连接成功（可能是服务器发生问题，如未开启的情况）
        // 要注意判断当前是否处理正在连接状态，避免同一IP的重复连接（这样会造成系统崩溃）
//        if (Sip_Conn_State == ISip.SIP_UNREGISTER)
        {
            sipRegister();
        }
        Log.d(MainActivity.LOG_TAG, "onStartCommand Sip_Conn_State==" + Sip_Conn_State);
        return super.onStartCommand(intent, flags, startId);
    }

    // 响应SipEvent
    public void onEventMainThread(SipEvent event) {
        switch (event.getSipAction()) {
            // ==== SIP注册相关响应 =====//
            case SipEvent.SIP_REGISTER:
                sipRegister();
                break;
            case SipEvent.SIP_UNREGISTER:
                sipUnregister();
                break;
            case SipEvent.SIP_REGISTER_SUCCESS:
                onSipRegSuc(event.getData());
                break;
            case SipEvent.SIP_REGISTER_FAIL:
                onSipRegFail(event.getData());
                break;

            // ===== SIP拨打电话相关响应===== //
            case SipEvent.SIP_CALLIN_RINGING:
                onSipCallinRinging(event.getData());
                break;
            case SipEvent.SIP_CALLOUT_RINGING:
                onSipCalloutRinging(event.getData());
                break;
            case SipEvent.SIP_CALLIN_BUSY:
                onSipCallinBusy();
                break;
            case SipEvent.SIP_CALLIN_HANGOUT:
                onSipCallinHangout();
                break;
            case SipEvent.SIP_CALLOUT_HANGOUT:
                onSipCalloutHangout();
                break;
        }
    }

    // 呼入电话
    private void onSipCallinRinging(String data) {
        EventBus.getDefault().post(new MainEvent(SipEvent.SIP_CALLIN_RINGING, data));
    }

    // 呼出电话
    private void onSipCalloutRinging(String data) {
        // 注意不要在Service或者BroadcastReceiver中启动Activity
        // 将开启Activity转发到MainActivity中，因为MainActivity是默认会一直存在的
        EventBus.getDefault().post(new MainEvent(SipEvent.SIP_CALLOUT_RINGING, data));
    }

    // 正在通话中
    private void onSipCallinBusy() {
        Toast.makeText(SipService.this, R.string.call_busy_hint, Toast.LENGTH_SHORT).show();
    }

    // 挂断呼入电话
    private void onSipCallinHangout() {

    }

    // 挂断呼出电话
    private void onSipCalloutHangout(){

    }

    // 开辟Sip线程尝试连接服务器
    private void sipRegister() {
        final String username = share.getString(getString(R.string.key_username), "");
        final String password = share.getString(getString(R.string.key_password), "");
        final String serverIp = share.getString(getString(R.string.key_serverip), "");

        // 检查参数是否合法
        if ((username.trim().equals("")) ||
            (password.trim().equals("")) ||
            (serverIp.trim().equals(""))) {
            onSipRegFail(serverIp);
        } else {
            // 开始连接服务器
            // 开启注册线程
            executors.execute(new Runnable() {
                @Override
                public void run() {
                    try {
                        if (SIP_INITED != -1)
                        {
                            Sip_Conn_State = ISip.SIP_UNREGISTER;
                            // 更新当前界面
                            EventBus.getDefault().post(new MainEvent(MainEvent.SIP_REGISTER_FAIL));
                            Log.i(MainActivity.LOG_TAG, serverIp + " begin To register");
                            int result = sip.register(serverIp, username, password);
                            Sip_Conn_State = ISip.SIP_CONNECTING; // 修改为正在连接状态
                        }
                    } catch (Exception e) {
                        onSipRegFail(serverIp);
                        Log.e(MainActivity.LOG_TAG, "sig error");
                    }
                }
            });
        }
    }

    // SIP注销
    private void sipUnregister() {
        // 如果已经注册再解注册
        if (Sip_Conn_State == ISip.SIP_REGISTERED) {
            executors.execute(new Runnable() {
                @Override
                public void run() {
                    try {
                        Log.i(MainActivity.LOG_TAG, " begin To Unregister");
                        sip.unregister();
                    } catch (Exception e) {
                        Log.e(MainActivity.LOG_TAG, "sig error");
                    }
                }
            });
        }
    }

    // SIP注册成功
    private void onSipRegSuc(final String targetIP) {
        // 获得系统当前serverIP
        final String serverIp = share.getString(getString(R.string.key_serverip), "");

        if (serverIp.equals(targetIP)) {
//            Toast.makeText(this, R.string.sip_reg_succ, Toast.LENGTH_SHORT).show();
            Sip_Conn_State = ISip.SIP_REGISTERED;
            EventBus.getDefault().post(new MainEvent(MainEvent.SIP_REGISTER_SUCCESS));
        }
    }

    // SIP注册失败
    private void onSipRegFail(final String targetIP) {
        // 获得系统当前serverIP
        final String serverIp = share.getString(getString(R.string.key_serverip), "");

        if (serverIp.equals(targetIP)) {
//            Toast.makeText(this, R.string.sip_reg_fail, Toast.LENGTH_SHORT).show();
            Sip_Conn_State = ISip.SIP_UNREGISTER;
            EventBus.getDefault().post(new MainEvent(MainEvent.SIP_REGISTER_FAIL));
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        // 解注册EventBus
        EventBus.getDefault().unregister(this);
    }
}
