package cn.edu.hust.buildingtalkback.speaking;

import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.GLES11Ext;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.io.IOException;
import java.lang.ref.WeakReference;

import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.helper.CameraHelper;
import cn.edu.hust.buildingtalkback.helper.ImageHelper;
import cn.edu.hust.buildingtalkback.iinterface.IMedia;
import cn.edu.hust.buildingtalkback.iinterface.INetwork;
import cn.edu.hust.buildingtalkback.iinterface.ISip;
import cn.edu.hust.buildingtalkback.jni.Media;
import cn.edu.hust.buildingtalkback.jni.NativeInterface;
import cn.edu.hust.buildingtalkback.jni.Network;
import cn.edu.hust.buildingtalkback.main.MainActivity;
import cn.edu.hust.buildingtalkback.myview.CircleImageView;
import cn.edu.hust.buildingtalkback.ringing.RingEvent;
import cn.edu.hust.buildingtalkback.sip.Sip;
import de.greenrobot.event.EventBus;

/**
 * Created by admin on 2016/1/15.
 * 呼出的电话被接听
 */
public class SpeakingOutActivity extends AppCompatActivity implements View.OnClickListener,
        SurfaceHolder.Callback, Camera.PreviewCallback{
    // 用以显示对方情况的surfaceView
    private SurfaceView outer_sur;
    private static SurfaceHolder outerHolder;

    private CircleImageView target_user_photo; // 对方头像
    private TextView target_user_name;  // 对方用户名
    private TextView speaking_time;    // 通话时长

    // 用以显示自身情况的SurfaceView
//    private SurfaceView inter_sur;
//    private SurfaceHolder inHolder;
    private Camera mCamera;
    Camera.Size cSize;

    private SurfaceTexture surfaceTexture;
    private byte[] previewBuffer;

    private RelativeLayout speaking_record_bt; // 录音按钮
    private RelativeLayout speaking_hangout_bt; // 挂断按钮

    private ISip sip;
    private IMedia media;
    private INetwork network;

    private String other;
    private boolean outgoing;

    private int time_count = 0; // 通话计时
    private String speaking_time_txt;
    private MyHandler mHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_speaking_out_layout);
        init();
        initView();
        // 开始视频显示
        startMedia();
    }

    // 相关参数初始化
    private void init() {
        sip = Sip.getInstance();
        media = Media.getInstance();
        network = Network.getInstance();

        mHandler = new MyHandler(this);
        EventBus.getDefault().register(this);

        openCamera();
        cSize = mCamera.getParameters().getPreviewSize();
        // 初始化视频发送环境：
        // 注意要旋转一个角度
        media.initVideo(cSize.height, cSize.width);

        surfaceTexture = new SurfaceTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);

        try {
            mCamera.setPreviewTexture(surfaceTexture);
        } catch (IOException e) {

        }

        cSize = mCamera.getParameters().getPreviewSize();
        int buffersize = cSize.width * cSize.height * ImageFormat.getBitsPerPixel(ImageFormat.NV21) / 8;
        previewBuffer = new byte[buffersize];
        Camera.Parameters p = mCamera.getParameters();
        p.setPictureSize(cSize.width, cSize.height);
        p.setPreviewFormat(ImageFormat.NV21); // 设置为最原始的YUYV格式

        /*这是唯一值，也可以不设置。有些同学可能设置成 PixelFormat 下面的一个值，其实是不对的，具体的可以看官方文档*/
        mCamera.setDisplayOrientation(90);
        mCamera.setParameters(p);
        mCamera.addCallbackBuffer(previewBuffer);
        mCamera.setPreviewCallbackWithBuffer(this);
        mCamera.startPreview();
    }

    // 相关界面参数初始化
    private void initView() {
        outer_sur = $(R.id.outer_sur);
        outerHolder = outer_sur.getHolder();
        outerHolder.setFixedSize(352, 288); // 设置分辨率
        outerHolder.addCallback(this);
        outerHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

        target_user_photo = $(R.id.target_user_photo);
        target_user_name = $(R.id.target_user_name);
        speaking_time = $(R.id.speaking_time);

//        inter_sur = $(R.id.inter_sur);
//        inHolder = inter_sur.getHolder();
//        inHolder.addCallback(this); // SurfaceHolder加入回调接口
//        inHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);// 設置顯示器類型，setType必须设置

        speaking_record_bt = $(R.id.speaking_record_bt);
        speaking_hangout_bt = $(R.id.speaking_hangout_bt);
        speaking_record_bt.setOnClickListener(this);
        speaking_hangout_bt.setOnClickListener(this);

        speaking_time_txt = getResources().getString(R.string.speaking_time_txt);
    }

    private void startMedia() {
        Intent intent = getIntent();
        other = intent.getStringExtra("other");
        outgoing = intent.getBooleanExtra("outgoing", true);
        new Thread(new Runnable() {

            @Override
            public void run() {
            // 对于呼出电话
            if (outgoing) {
                network.waitForClient();
                media.startVideo(true, true);
//                    media.startVideo(true, true);
//                    media.startAudio();
            }  // 对于呼入电话
            else {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                network.startClient();
                media.startVideo(true, true);
//                media.startVideo(true, true);
//                    media.startAudio();
            }
            }
        }).start();

        isConnecting = true;
        // 开启发送数据线程
        new Thread(new TimeRunnable()).start();
    }

    private volatile boolean isConnecting = false;
    // 时间刷新线程
    private class TimeRunnable implements Runnable {
        @Override
        public void run() {
            while (isConnecting) {
                try {
                    Thread.sleep(1000);
                    time_count++;
                    if (mHandler != null) {
                        mHandler.sendEmptyMessage(TIME_TO_REFRESH);
                    }
                } catch (Exception e) {}
            }
        }
    }

    // 1s刷新通知
    private static final int TIME_TO_REFRESH = 1;
    // ============== 通话计时 =================//
    private class MyHandler extends Handler {
        private WeakReference<SpeakingOutActivity> act;

        public MyHandler(SpeakingOutActivity activity) {
            act = new WeakReference<>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            act.get().handleMessage(msg);
        }
    }

    // 消息处理函数
    private void handleMessage(Message msg) {
        switch (msg.what) {
            case TIME_TO_REFRESH:
                try {
                    speaking_time.setText(speaking_time_txt + time_count + "s");
                } catch (Exception e) {}

                break;
        }
    }


    @Override
    protected void onResume() {
        super.onResume();
        openCamera(); // 打开摄像头
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

    // 初始化camera参数
    private void initCamera() {
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
        network.stopNetwork();
        media.stopVideo();
        isConnecting = false;
        EventBus.getDefault().unregister(this);
        super.onDestroy();
    }

    private <T> T $(int resId) {
        return (T) findViewById(resId);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.speaking_record_bt:
                break;

            case R.id.speaking_hangout_bt:
                sip.endCall();
                this.finish();
                break;
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        NativeInterface.setStatus(NativeInterface.SPEAKING);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        outerHolder = null;
//        isSendOn = false;
    }

    // 发送帧数据信息
    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {

        camera.addCallbackBuffer(previewBuffer);

        byte[] changedData = new byte[data.length];
        // 发送每一帧数据
        ImageHelper.YUV420spRotate90(changedData, data, cSize.width, cSize.height);
        Log.d(MainActivity.LOG_TAG2, "currentThread==" + Thread.currentThread().getName());
        Log.d(MainActivity.LOG_TAG, "SpeakingOut onPreviewFrame data.size==" + changedData.length);

        media.asyncSendFrame(changedData);

//        dataToSend = changedData.clone();
    }

//    private byte[] changedData;
//    private boolean hasDataToSend = true; // 标志有无数据要发送
//    private boolean isSendOn = false;      // 标志SendThread是否存活
//    private volatile byte[] dataToSend = null;
//    // 开辟一个子线程来发送数据
//    private class SendThread implements Runnable {
//
//        @Override
//        public void run() {
//            isSendOn = true;
//
//            while (isSendOn) {
//                if (hasDataToSend) {
//                    try {
//                        Log.d(MainActivity.LOG_TAG2, "dataToSend");
//                        if (dataToSend != null) {
//                            media.sendVideoFrame(dataToSend);
//                        }
//                    } catch (Exception e) {
//                        Log.e(MainActivity.LOG_TAG, "SendThread " + e.toString());
//                    } finally {
////                        hasDataToSend = false;
//                        dataToSend = null;
//                    }
//                }
//            }
//        }
//    }

    // 显示图像
    public static void showImage(int colors[], int width, int height) {
        try {
            if (outerHolder != null) {
                synchronized (outerHolder)
                {
                    Log.i(MainActivity.LOG_TAG2, "showImage Thread==" + Thread.currentThread().getName());
                    Canvas canvas = outerHolder.lockCanvas();
                    canvas.drawBitmap(colors, 0, width, 0, 0, width, height, false, null);
                    outerHolder.unlockCanvasAndPost(canvas);
                }
            }
        } catch (Exception e) {
            Log.e(MainActivity.LOG_TAG, "showImage Error:" + e.toString());
        }
    }

    // 响应RingEvent
    public void onEventMainThread(RingEvent event) {
        switch (event.getAction()) {
            case SpeakingEvent.RING_HANGOUT_SUCCESS:
                finish();// 关闭程序
                break;
        }
    }
}
