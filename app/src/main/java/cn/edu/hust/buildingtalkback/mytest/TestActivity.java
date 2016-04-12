package cn.edu.hust.buildingtalkback.mytest;

import android.hardware.Camera;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import cn.edu.hust.buildingtalkback.R;

/**
 * Created by admin on 2016/1/5.
 */
public class TestActivity extends AppCompatActivity implements View.OnClickListener {

    private Toolbar toolbar;
    private Button returnBt;

    private Camera mCamera = null;     // Camera对象，相机预览

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.test_layout);

        mCamera = Camera.open();// 开启摄像头
        mCamera.setPreviewCallback(new Camera.PreviewCallback() {
            @Override
            public void onPreviewFrame(byte[] data, Camera camera) {
                //传递进来的data,默认是YUV420SP的
                try {
                    Log.i("LOG_TAG", "going into onPreviewFrame" + data.length);
                    int YUVIMGLEN = data.length;
                    // 拷贝原生yuv420sp数据
//					System.arraycopy(data, 0, mYUV420SPSendBuffer, 0, data.length);
                    //System.arraycopy(data, 0, mWrtieBuffer, 0, data.length);

                    // 开启编码线程，如开启PEG编码方式线程
//					mSendThread1.start();

                } catch (Exception e) {
                    Log.v("System.out", e.toString());
                }// endtry
            }// endonPriview
        });
        mCamera.startPreview(); // 打开预览画面

        initView();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mCamera != null) {
            mCamera.release();
        }
    }

    private void initView() {
        toolbar = $(R.id.toolbar);
        toolbar.setTitle("");
        setSupportActionBar(toolbar);

        returnBt = $(R.id.return_bt);
        returnBt.setOnClickListener(this);
    }

    private <T> T $(int resId) {
        return (T) findViewById(resId);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.return_bt:
                this.finish();
                break;
        }
    }
}
