package cn.edu.hust.buildingtalkback.ringing;

import android.content.Intent;
import android.graphics.Canvas;
import android.hardware.Camera;
import android.os.Bundle;
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

import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.activities.SpeakingActivity;
import cn.edu.hust.buildingtalkback.helper.CameraHelper;
import cn.edu.hust.buildingtalkback.iinterface.IMedia;
import cn.edu.hust.buildingtalkback.iinterface.INetwork;
import cn.edu.hust.buildingtalkback.iinterface.ISip;
import cn.edu.hust.buildingtalkback.jni.Media;
import cn.edu.hust.buildingtalkback.jni.NativeInterface;
import cn.edu.hust.buildingtalkback.jni.Network;
import cn.edu.hust.buildingtalkback.main.MainActivity;
import cn.edu.hust.buildingtalkback.myview.CircleImageView;
import cn.edu.hust.buildingtalkback.sip.Sip;
import cn.edu.hust.buildingtalkback.speaking.SpeakingInActivity;
import cn.edu.hust.buildingtalkback.speaking.SpeakingOutActivity;
import de.greenrobot.event.EventBus;

/**
 * Created by admin on 2016/1/14.
 */
public class CallInRingActivity extends AppCompatActivity implements View.OnClickListener, SurfaceHolder.Callback {

    private final String LOG_TAG = "LOG_TAG";

    private SurfaceView sv;

    private TextView tvCaller;

    private Button btnAnswerCall;
    private Button btnEndCall;

    private static SurfaceHolder holder;

    private String other;

    private ISip sip;
    private IMedia media;
    private INetwork network;

    // 相关的View参数
    private CircleImageView target_user_photo;
    private TextView target_user_name;
    private TextView target_user_call_state;
    private TextView calling_state;
    private RelativeLayout hangout_bt;
    private RelativeLayout recv_bt;

    Camera.Size cSize;
    private Camera mCamera; // Camera对象，相机预览

    boolean isRecycled = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_callin_layout);

        sip = Sip.getInstance();
        media = Media.getInstance();
        network = Network.getInstance();

        Intent intent = getIntent();
        other = intent.getStringExtra("other");

        initView();

        EventBus.getDefault().register(this);
//        // 打开Camera
//        openCamera();
//
//        cSize = mCamera.getParameters().getPreviewSize();
//        // 初始化视频发送环境：
//        // 注意要旋转一个角度
//        media.initVideo(cSize.height, cSize.width);

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                network.startClient();
                media.startVideo(false, true);
            }
        }).start();
    }


    private void initView() {
        sv = (SurfaceView) findViewById(R.id.ringin_sur);

        holder = sv.getHolder();
        holder.setFixedSize(352, 288); // 设置分辨率
        holder.addCallback(this);

        target_user_photo = $(R.id.target_user_photo);
        target_user_name = $(R.id.target_user_name);
        target_user_call_state = $(R.id.target_user_call_state);
        calling_state = $(R.id.calling_state);
        hangout_bt = $(R.id.hangout_bt);
        recv_bt = $(R.id.recv_bt);

        hangout_bt.setOnClickListener(this);
        recv_bt.setOnClickListener(this);
        target_user_name.setText(other);


    }

    // 打开Camera
    private void openCamera() {
        // 注意避免在onCreate中已经打开Camera
        if (mCamera == null) {
            // 优先打开前置摄像头
            int cameraIdx = CameraHelper.FindBackCamera();
            if (cameraIdx != -1) {
                mCamera = Camera.open(cameraIdx);
            } else {
                mCamera = Camera.open();
            }
        }
    }

    private <T> T $(int resId) {
        return (T) findViewById(resId);
    }

    @Override
    public void onClick(View v) {

        switch (v.getId()) {
            case R.id.recv_bt:
                sip.answerCall();
                break;

            case R.id.hangout_bt:
                sip.endCall();
                finish(); // 直接关闭Activity;
                break;
            default:
                break;
        }
    }

    // 响应RingEvent
    public void onEventMainThread(RingEvent event) {
        switch (event.getAction()) {
            case RingEvent.RING_CALLIN_RECV_SUCCESS:
                onRingRecvSuccess(event.getData());
                break;
            case RingEvent.RING_HANGOUT_SUCCESS:
                onRingHangoutSuccess();
                break;
        }
    }

    // 接通电话成功响应
    private void onRingRecvSuccess(String data) {
        onRecycle();
        Intent intent = new Intent(CallInRingActivity.this, SpeakingOutActivity.class);
        intent.putExtra("other", data);
        intent.putExtra("outgoing", false);
        startActivity(intent);
        this.finish();
    }

    // 被挂断电话
    private void onRingHangoutSuccess() {
        this.finish();
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
            Log.w(MainActivity.LOG_TAG2, "media.stopVideo()");
            media.stopVideo();
            network.stopNetwork();
            isRecycled = true;
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        this.holder = holder;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        this.holder = holder;
        NativeInterface.setStatus(NativeInterface.RINGING);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        this.holder = null;
    }

    // 显示Image
    public static void showImage(int colors[], int width, int height) {
        try {
            if (holder != null) {
                Canvas canvas = holder.lockCanvas();
                canvas.drawBitmap(colors, 0, width, 0, 0, width, height, false, null);
                holder.unlockCanvasAndPost(canvas);
            }
        } catch (Exception e) {
            Log.e(MainActivity.LOG_TAG, "showImage Error:" + e.toString());
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