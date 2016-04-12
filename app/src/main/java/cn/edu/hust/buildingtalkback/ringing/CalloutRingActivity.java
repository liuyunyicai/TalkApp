package cn.edu.hust.buildingtalkback.ringing;

import android.content.Intent;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Bundle;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.List;
import java.util.logging.Handler;

import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.activities.SpeakingActivity;
import cn.edu.hust.buildingtalkback.helper.ImageHelper;
import cn.edu.hust.buildingtalkback.iinterface.IMedia;
import cn.edu.hust.buildingtalkback.iinterface.INetwork;
import cn.edu.hust.buildingtalkback.iinterface.ISip;
import cn.edu.hust.buildingtalkback.jni.Media;
import cn.edu.hust.buildingtalkback.jni.Network;
import cn.edu.hust.buildingtalkback.main.MainActivity;
import cn.edu.hust.buildingtalkback.myview.CircleImageView;
import cn.edu.hust.buildingtalkback.sip.Sip;
import cn.edu.hust.buildingtalkback.sip.SipService;
import cn.edu.hust.buildingtalkback.speaking.SpeakingOutActivity;
import de.greenrobot.event.EventBus;

/**
 * Created by admin on 2016/1/11.
 * 呼出电话 响铃界面
 */
public class CalloutRingActivity extends AppCompatActivity implements SurfaceHolder.Callback,
        Camera.PreviewCallback , View.OnClickListener{
    private static final String LOG_TAG = "LOG_TAG";
    private SurfaceView mSurfaceview = null; // SurfaceView对象：(视图组件)视频显示
    private SurfaceHolder mSurfaceHolder = null; // SurfaceHolder对象：(抽象接口)SurfaceView支持类
    private Camera mCamera = null; // Camera对象，相机预览

    private Button endCallBt;

    private ISip sip;
    private IMedia media;
    private INetwork network;

    private String targetUser; // 拨打用户

    public static int cur_state; // 记录当前状态
    private static final int WAITING_CLIENT = -1; // 等待呼出电话接听
    private static final int CLIENT_RECV = 1;     // 呼出电话已接听
    private static final int END_CALL = 10;       // 结束通话

    // 相关的View参数
    private CircleImageView target_user_photo;
    private TextView target_user_name;
    private TextView target_user_call_state;
    private TextView calling_state;
    private RelativeLayout hangout_bt;

    Camera.Size cSize;

    public static boolean isClientConnected = false;
    private boolean isActivityClosing = false;

    private long startTime = System.currentTimeMillis();
    // 最长等待时间
    public static final int MAX_WATING_TIME = 30 * 1000; // 100s

    private String no_reply_fail;
    private int color_red;
    private CallOutHandler mHandler;

    boolean isRecycled = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_callout_layout);

        sip = Sip.getInstance();
        media = Media.getInstance();
        network = Network.getInstance();

        // 每一次resume注意注册一次
//        startService(new Intent(this, SipService.class));

        Intent intent = getIntent();
        targetUser = intent.getStringExtra("other");

        EventBus.getDefault().register(this);
        mHandler = new CallOutHandler(this);
        isClientConnected = false; // 每一次重启都记得进行初始化

//        Log.w(MainActivity.LOG_TAG, "CalloutRingActivity onCreate");

        initView();
        // 初始化视频帧发送环境
        initMedia();
    }

    // 初始化视频帧发送环境
    private void initMedia() {
        cur_state = WAITING_CLIENT;
        // 打开Camera
        openCamera();

        cSize = mCamera.getParameters().getPreviewSize();
        // 初始化视频发送环境：
        // 注意要旋转一个角度
        media.initVideo(cSize.height, cSize.width);

        new Thread(new Runnable() {
            @Override
            public void run() {
//                int result = network.waitForClient();
            if (network.waitForClient() != -1) {
                cur_state = CLIENT_RECV;
            }
            }
        }).start();
    }

    private void initView() {
        mSurfaceview = $(R.id.ringout_sur);
        mSurfaceHolder = mSurfaceview.getHolder(); // 绑定SurfaceView，取得SurfaceHolder对象
        mSurfaceHolder.addCallback(this); // SurfaceHolder加入回调接口
        mSurfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);// 設置顯示器類型，setType必须设置

        target_user_photo = $(R.id.target_user_photo);
        target_user_name = $(R.id.target_user_name);
        target_user_call_state = $(R.id.target_user_call_state);
        calling_state = $(R.id.calling_state);
        hangout_bt = $(R.id.hangout_bt);

        hangout_bt.setOnClickListener(this);
        target_user_name.setText(targetUser);

        no_reply_fail = getResources().getString(R.string.no_reply_fail);
        color_red = getResources().getColor(R.color.red);

//        endCallBt = $(R.id.btn_end_call_ringring);
//        endCallBt.setOnClickListener(this);
    }

    public static final int CLOSING_ACTIVITY_0 = 0;
    public static final int CLOSING_ACTIVITY_1 = 1;
    public static final int CLOSING_ACTIVITY_2 = 2;
    public static final int CLOSING_ACTIVITY_3 = 3;

    // 接通电话后打开Speaking界面
    public static final int OPEN_SPEAKING_VIEW = 4;

    private class CallOutHandler extends android.os.Handler {
        private WeakReference<CalloutRingActivity> act;

        public CallOutHandler(CalloutRingActivity activity) {
            act = new WeakReference<CalloutRingActivity>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            act.get().handleMessage(msg);
        }
    }

    // 处理Handler信息逻辑
    private void handleMessage(Message msg) {
        switch (msg.what) {
            case CLOSING_ACTIVITY_0:
                sip.endCall();
                this.finish();
                break;
            case CLOSING_ACTIVITY_1:
            case CLOSING_ACTIVITY_2:
            case CLOSING_ACTIVITY_3:
                if (calling_state!= null) {
                    calling_state.setText(no_reply_fail + msg.what +"s");
                    // 更新界面，提示无人接听，界面将关闭
                    calling_state.setTextColor(color_red);
                }
                break;
            case OPEN_SPEAKING_VIEW:
                onOpenSpeakingView(msg.getData());
                break;
        }
    }

    // 打开通话界面
    private void onOpenSpeakingView(Bundle extra) {
        String data = extra.getString(EXTRA_NAME);
        Intent intent = new Intent(CalloutRingActivity.this, SpeakingOutActivity.class);
        intent.putExtra("other", data);
        startActivity(intent);
        this.finish();
    }

    private <T> T $(int resId) {
        return (T) findViewById(resId);
    }

    // 响应RingEvent
    public void onEventMainThread(RingEvent event) {
        switch (event.getAction()) {
            case RingEvent.RING_CALLOUT_RECV_SUCCESS:
                onRingRecvSuccess(event.getData());
                break;
            case RingEvent.RING_HANGOUT_SUCCESS:
                onRingHangoutSuccess();
                break;
        }
    }

    private static final String EXTRA_NAME = "other";
    // 接通电话成功响应
    private void onRingRecvSuccess(String data) {
        onRecycle();
        Intent intent = new Intent(this, SpeakingOutActivity.class);
        intent.putExtra("other", data);
        intent.putExtra("outgoing", true);
        startActivity(intent);
        this.finish();
    }

    // 被挂断电话
    private void onRingHangoutSuccess() {
        this.finish();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (mCamera == null) {
            return;
        }
        Camera.Size cSize = mCamera.getParameters().getPreviewSize();

        mCamera.stopPreview();
        mCamera.setPreviewCallback(this);
        // 获取Camera支持的视频格式
        Camera.Parameters p = mCamera.getParameters();
        p.setPictureSize(cSize.width, cSize.height);
        p.setPreviewFormat(ImageFormat.NV21); // 设置为最原始的YUYV格式
        /*这是唯一值，也可以不设置。有些同学可能设置成 PixelFormat 下面的一个值，其实是不对的，具体的可以看官方文档*/
        mCamera.setDisplayOrientation(90);
        mCamera.setParameters(p);
        mCamera.startPreview();
    }

    // 查找是否有前置摄像头，如果有则打开
    private int FindBackCamera(){
        int cameraCount = 0;
        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
        cameraCount = Camera.getNumberOfCameras(); // 获得摄像头的总数

        int backInx = -1; // 记录后置摄像头的Index
        for (int camIdx = 0; camIdx < cameraCount;camIdx++ ) {
            Camera.getCameraInfo(camIdx, cameraInfo);
            // 如果有前置摄像头，则优先打开前置摄像头
            if (cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_FRONT) {
                // 代表摄像头的方位，目前有定义值两个分别为CAMERA_FACING_FRONT前置和CAMERA_FACING_BACK后置
                return camIdx;
            }
            if ( cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_BACK ) {
                backInx = camIdx;
            }
        }
        return backInx;
    }

    // 打开Camera
    private void openCamera() {
        // 注意避免在onCreate中已经打开Camera
        if (mCamera == null) {
            // 优先打开前置摄像头
            int cameraIdx = FindBackCamera();
            if (cameraIdx != -1) {
                mCamera = Camera.open(cameraIdx);
            } else {
                mCamera = Camera.open();
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        openCamera();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mCamera != null) {
            mCamera.stopPreview();
            mCamera.setPreviewCallback(null);
            mCamera.release();
            mCamera = null;
        }
    }

    @Override
    protected void onDestroy() {
        onRecycle();
        EventBus.getDefault().unregister(this);
        super.onDestroy();
    }

    // 进行资源回收
    private void onRecycle() {
        if (!isRecycled) {
            network.stopNetwork();
            isRecycled = true;
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        try {
            if (mCamera != null) {
                mCamera.setPreviewDisplay(mSurfaceHolder);
                mCamera.startPreview();

//                mCamera.getParameters().getSupportedPreviewFormats();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mSurfaceHolder = null;
//        if (mCamera != null) {
//            mCamera.stopPreview();
//            mCamera.setPreviewCallback(null);
//        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        Log.v(MainActivity.LOG_TAG, "onPreviewFrame begin isClientConnected == " + isClientConnected);
        if (!isClientConnected && (!isActivityClosing)) {
            // 超过规定时间仍然未接听，则提示退出拨打界面
            if (System.currentTimeMillis() - startTime > MAX_WATING_TIME) {
                // 三秒之后关闭界面
                mHandler.sendEmptyMessage(CLOSING_ACTIVITY_3);
                mHandler.sendEmptyMessageDelayed(CLOSING_ACTIVITY_2, 1000);
                mHandler.sendEmptyMessageDelayed(CLOSING_ACTIVITY_1, 2000);
                mHandler.sendEmptyMessageDelayed(CLOSING_ACTIVITY_0, 3000);
                isActivityClosing = true;
            }
        }

        // 只有当Client连接之后才发送数据
        if (isClientConnected)
        {
            byte[] changedData = new byte[data.length];
            // 发送每一帧数据
            ImageHelper.YUV420spRotate90(changedData, data, cSize.width, cSize.height);
            Log.d(LOG_TAG, "onPreviewFrame data.size==" + changedData.length);
            media.sendVideoFrame(changedData);
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            // 结束通话按钮
            case R.id.hangout_bt:
                Log.w(MainActivity.LOG_TAG, "hangout_bt OnClick");
                // 如果没有连接成功，则不用调用endCall
//                if (isClientConnected)
                sip.endCall();
                this.finish();
                break;
        }
    }

    // 处理回退键
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            Toast.makeText(this, R.string.hang_out_hint, Toast.LENGTH_SHORT).show();
        }
        return false;
    }
}
