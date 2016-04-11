package cn.edu.hust.buildingtalkback;

import cn.edu.hust.buildingtalkback.jni.Media;
import cn.edu.hust.buildingtalkback.jni.NativeInterface;
import cn.edu.hust.buildingtalkback.jni.Network;
import cn.edu.hust.buildingtalkback.jni.Sip;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Canvas;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class SpeakingActivity extends Activity implements Callback, OnClickListener {

	private final String LOG_TAG = "BT_SPEAKINGACTIVITY";

	private TextView tvCaller;
	private Button btnHangUp;
	private static SurfaceHolder holder;
	
	private String other;
	private boolean outgoing;

	@Override
	protected void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_speaking);
		
		Intent intent = getIntent();
		other = intent.getStringExtra("other");
		outgoing = intent.getBooleanExtra("outgoing", true);

		initView();
		
		new Thread(new Runnable() {
			
			@Override
			public void run() {

				if (outgoing) {
					Network.waitForClient();
					Media.startVideo(true, true);
					Media.startAudio();
				}
				else {
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
					Network.startClient();
					Media.startVideo(true, true);
					Media.startAudio();
				}
			}
		}).start();
	}

	private void initView() {

		tvCaller = (TextView) findViewById(R.id.tv_caller_speaking);
		btnHangUp = (Button) findViewById(R.id.btn_end_call_speaking);
		btnHangUp.setOnClickListener(this);
		
		tvCaller.setText(other);

		holder = ((SurfaceView) findViewById(R.id.sv)).getHolder();
		holder.setFixedSize(352, 288);
		holder.addCallback(this);
	}
	
	protected void onDestroy() {

		Media.stopAudio();
		Media.stopVideo();
		Network.stopNetwork();
		super.onDestroy();
	}


	public static void showImage(int colors[], int width, int height) {

		Canvas canvas = holder.lockCanvas();
		canvas.drawBitmap(colors, 0, width, 0, 0, width, height, false, null);
		holder.unlockCanvasAndPost(canvas);
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		this.holder = holder;
		Log.d(LOG_TAG, "surfaceChanged, width = " + width + ", height = " + height);
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		this.holder = holder;
		NativeInterface.setStatus(NativeInterface.SPEAKING);
		Log.d(LOG_TAG, "surfaceCreated");
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		this.holder = null;
	}

	@Override
	public void onClick(View v) {

		switch (v.getId()) {
		case R.id.btn_end_call_speaking:
			Sip.endCall();
			break;

		default:
			break;
		}
	}
}