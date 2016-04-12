package cn.edu.hust.buildingtalkback.main;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import com.yalantis.contextmenu.lib.ContextMenuDialogFragment;
import com.yalantis.contextmenu.lib.MenuObject;
import com.yalantis.contextmenu.lib.MenuParams;
import com.yalantis.contextmenu.lib.interfaces.OnMenuItemClickListener;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import cn.edu.hust.buildingtalkback.activities.DialActivity;
import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.activities.RecordActivity;
import cn.edu.hust.buildingtalkback.activities.RingingActivity;
import cn.edu.hust.buildingtalkback.activities.SettingActivity;
import cn.edu.hust.buildingtalkback.iinterface.ISip;
import cn.edu.hust.buildingtalkback.jni.NativeInterface;
import cn.edu.hust.buildingtalkback.mytest.TestActivity;
import cn.edu.hust.buildingtalkback.ringing.CallInRingActivity;
import cn.edu.hust.buildingtalkback.ringing.CalloutRingActivity;
import cn.edu.hust.buildingtalkback.sip.Sip;
import cn.edu.hust.buildingtalkback.sip.SipEvent;
import cn.edu.hust.buildingtalkback.sip.SipService;
import de.greenrobot.event.EventBus;

public class MainActivity extends AppCompatActivity implements OnClickListener, OnMenuItemClickListener {

    public static final String LOG_TAG = "LOG_TAG";
    public static final String LOG_TAG2 = "LOG_TAG2";

    private String username;
    private String password;
    private String serverIp;

    private Toolbar toolbar; // 顶部导航栏
    private Button btnDial;
    private Button btnRecord;
    private Button btn_smart_home;
    private ImageView sip_state_view; // 记录当前用户是否SIP在线

    private ExecutorService executors;

    private FragmentManager fragmentManager;


    private ISip sip;

    // SharedPreference相关参数

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_layout);
        fragmentManager = getSupportFragmentManager();
        sip = Sip.getInstance();
        EventBus.getDefault().register(this);

        initView();
        initMenuFragment();
        initData();
    }

    // 初始化数据
    private void initData() {
        NativeInterface.setActivity(this);
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        username = prefs.getString(getString(R.string.key_username), "");
        password = prefs.getString(getString(R.string.key_password), "");
        serverIp = prefs.getString(getString(R.string.key_serverip), "");

        executors = Executors.newCachedThreadPool();
    }

    private <T> T $(int resId) {
        return (T) findViewById(resId);
    }

    @Override
    protected void onResume() {
        super.onResume();
        // 每一次resume注意注册一次
        startService(new Intent(this, SipService.class));
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        EventBus.getDefault().unregister(this);
    }

    protected void initView() {
        // 设置顶部导航栏
        toolbar = $(R.id.toolbar);
        toolbar.setTitle("");
        setSupportActionBar(toolbar);

        btnDial = $(R.id.btn_make_call);
        btnRecord = $(R.id.btn_record_query);
        btn_smart_home = $(R.id.btn_smart_home);

        btnDial.setOnClickListener(this);
        btnRecord.setOnClickListener(this);
        btn_smart_home.setOnClickListener(this);

        sip_state_view = $(R.id.sip_state_view);
        sip_state_view.setOnClickListener(this);
        sip_state_view.setImageResource(SipService.Sip_Conn_State == ISip.SIP_REGISTERED ? R.mipmap.person_icon :
                R.mipmap.person_icon_unregisted);

    }

    // ================== 响应Service发送来的事件 ============//
    public void onEventMainThread(MainEvent event) {
        try {
            switch (event.getAction()) {
                // 注册成功刷新界面
                case MainEvent.SIP_REGISTER_SUCCESS:
                    sip_state_view.setImageResource(R.mipmap.person_icon);
                    break;
                // 注册失败刷新界面
                case MainEvent.SIP_REGISTER_FAIL:
                    sip_state_view.setImageResource(R.mipmap.person_icon_unregisted);
                    break;
                case SipEvent.SIP_CALLOUT_RINGING:
                    onSipCalloutRinging(event.getData());
                    break;
                case SipEvent.SIP_CALLIN_RINGING:
                    onSipCallInRinging(event.getData());
                    break;

            }
        } catch (Exception e) {
            Log.e(MainActivity.LOG_TAG, "MainActivity onEventMainThread " + e.toString());
        }
    }

    // 当有电话呼入情况，开启CallInRingActivity
    private void onSipCallInRinging(String target_user) {
        Intent intent = new Intent(this, CallInRingActivity.class);
        intent.putExtra("other", target_user);
        startActivity(intent);
    }

    // 当有电话呼出情况，开启CallingOutActivity
    private void onSipCalloutRinging(String target_user) {
        Intent intent = new Intent(this, CalloutRingActivity.class);
        intent.putExtra("other", target_user);
        startActivity(intent);
    }


    // ================== 建立菜单列表 ============//
    private ContextMenuDialogFragment mMenuDialogFragment; // 菜单列表

    // ====== 设置菜单 ===== //
    private void initMenuFragment() {
        MenuParams menuParams = new MenuParams();
        menuParams.setActionBarSize((int) getResources().getDimension(R.dimen.tool_bar_height));
        menuParams.setMenuObjects(getMenuObjects());
        menuParams.setClosableOutside(false);
        mMenuDialogFragment = ContextMenuDialogFragment.newInstance(menuParams);
        mMenuDialogFragment.setItemClickListener(this);
    }

    private List<MenuObject> getMenuObjects() {

        List<MenuObject> menuObjects = new ArrayList<>();

        MenuObject close = new MenuObject();
        close.setResource(R.mipmap.icn_close);

        Resources res = getResources();
        MenuObject netset = new MenuObject(res.getString(R.string.net_set));
        netset.setResource(R.mipmap.netset_icon);

        MenuObject record = new MenuObject(res.getString(R.string.record_txt));
        record.setResource(R.mipmap.record_icon);

        MenuObject book = new MenuObject(res.getString(R.string.book_txt));
        book.setResource(R.mipmap.book_icon);

        MenuObject exit = new MenuObject(res.getString(R.string.exit_txt));
        exit.setResource(R.mipmap.exit_icon);

        menuObjects.add(close);
        menuObjects.add(netset);
        menuObjects.add(record);
        menuObjects.add(book);
        menuObjects.add(exit);

        return menuObjects;
    }

    // ==== 创建菜单及响应 ==== //
    @Override
    public boolean onCreateOptionsMenu(final Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.context_menu:
                if (fragmentManager.findFragmentByTag(ContextMenuDialogFragment.TAG) == null) {
                    mMenuDialogFragment.show(fragmentManager, ContextMenuDialogFragment.TAG);
                }
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    // 菜单响应函数
    private static final int CLOSE_MENU = 0;
    private static final int NETSET_MENU = 1;
    private static final int RECORD_MENU = 2;
    private static final int BOOK_MENU = 3;
    private static final int EXIT_MENU = 4;
    @Override
    public void onMenuItemClick(View clickedView, int position) {
        switch (position) {
            case CLOSE_MENU:
                break;
            case NETSET_MENU:
                startActivity(new Intent(this, SettingActivity.class));
                break;
            case RECORD_MENU:
                break;
            case BOOK_MENU:
                break;
            case EXIT_MENU:
                // 强制退出应用
                android.os.Process.killProcess(android.os.Process.myPid());
                break;
        }
    }

    @Override
    public void onClick(View view) {

        switch (view.getId()) {

            case R.id.btn_make_call:
                // 只有登录之后才能拨打电话，否则程序将会崩溃
                if (SipService.Sip_Conn_State != ISip.SIP_REGISTERED) {
                    Toast.makeText(MainActivity.this, R.string.register_hint, Toast.LENGTH_SHORT).show();
                } else {
                    startActivity(new Intent(this, DialActivity.class));
                }
                break;
            case R.id.btn_record_query:
                startActivity(new Intent(this, RecordActivity.class));
                break;
            case R.id.sip_state_view:
                // 若当前状态的未连接时，则尝试连接服务器
//                if (SipService.Sip_Conn_State == ISip.SIP_UNREGISTER)
                {
                    startService(new Intent(this, SipService.class));
                }
                break;
            case R.id.btn_smart_home:
                startActivity(new Intent(this, TestActivity.class));
                break;
            default:
                break;
        }
    }

    // ===== 双击退出程序 === //
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onBackCodePressed();
        }
        return false;
    }

    private int mPressedTime = 0;

    private void onBackCodePressed() {
        // 表示第一次点击
        if (mPressedTime == 0) {
            Toast.makeText(this, "连续点击退出程序 ", Toast.LENGTH_SHORT).show();
            ++mPressedTime;

            new Thread() {
                @Override
                public void run() {
                    try {
                        sleep(2000);
                    } catch (Exception e) {
                        Log.e("LOG_TAG", e.toString());
                    } finally {
                        mPressedTime = 0;
                    }
                }
            };
        } else {
//            android.os.Process.killProcess(android.os.Process.myPid());
            MainActivity.this.finish();
        }
    }
}
