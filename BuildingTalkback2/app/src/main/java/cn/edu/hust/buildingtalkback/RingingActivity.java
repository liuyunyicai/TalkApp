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

public class RingingActivity extends Activity implements OnClickListener, Callback {

	private final String LOG_TAG = "BT_RINGINGACTIVITY";

	private SurfaceView sv;

	private TextView tvCaller;

	private Button btnAnswerCall;
	private Button btnEndCall;

	private static SurfaceHolder holder;

	private String other;
	private boolean outgoing;

	@Override
	protected void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_ringing);

		Intent intent = getIntent();
		other = intent.getStringExtra("other");
		outgoing = intent.getBooleanExtra("outgoing", true);

		initView();

		new Thread(new Runnable() {

			@Override
			public void run() {

				if (outgoing) {
					Network.waitForClient();
					Media.startVideo(true, false);
				}
				else {
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
					Network.startClient();
					Media.startVideo(false, true);
				}
			}
		}).start();
//		});
	}

	private void initView() {

		sv = (SurfaceView) findViewById(R.id.sv_ringing);

		holder = sv.getHolder();
		holder.setFixedSize(352, 288);
		holder.addCallback(this);

		tvCaller = (TextView) findViewById(R.id.tv_caller_ringring);

		btnAnswerCall = (Button) findViewById(R.id.btn_answer_call_ringring);
		btnAnswerCall.setOnClickListener(this);

		btnEndCall = (Button) findViewById(R.id.btn_end_call_ringring);
		btnEndCall.setOnClickListener(this);

		if (outgoing) {
			sv.setVisibility(View.GONE);
			btnAnswerCall.setVisibility(View.GONE);
			tvCaller.setText("正在呼叫" + other);
		}
		else {
			tvCaller.setText(other);
		}
	}

	@Override
	public void onClick(View v) {

		switch (v.getId()) {

			case R.id.btn_answer_call_ringring:
				Sip.answerCall();
				break;

			case R.id.btn_end_call_ringring:
				Sip.endCall();
				break;

			default:
				break;
		}
	}

	@Override
	protected void onDestroy() {

		Media.stopVideo();
		Network.stopNetwork();
		super.onDestroy();
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
							   int height) {
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

	public static void showImage(int colors[], int width, int height) {

		Canvas canvas = holder.lockCanvas();
		canvas.drawBitmap(colors, 0, width, 0, 0, width, height, false, null);
		holder.unlockCanvasAndPost(canvas);
	}
}
