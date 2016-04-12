package cn.edu.hust.buildingtalkback.jni;

import android.util.Log;

import cn.edu.hust.buildingtalkback.activities.RingingActivity;
import cn.edu.hust.buildingtalkback.activities.SpeakingActivity;
import cn.edu.hust.buildingtalkback.iinterface.IMedia;
import cn.edu.hust.buildingtalkback.main.MainActivity;
import cn.edu.hust.buildingtalkback.ringing.CallInRingActivity;
import cn.edu.hust.buildingtalkback.speaking.SpeakingOutActivity;

public class Media implements IMedia{

	private static Media instance = null;

	public static Media getInstance() {
		if (instance == null)
			instance = new Media();
		return instance;
	}

	public int startAudio() {
		return NativeInterface.startAudio();
	}

	public void stopAudio() {
		NativeInterface.stopAudio();
	}

	@Override
	public int initVideo(int camera_width, int camera_height) {
		return NativeInterface.initVideo(camera_width, camera_height);
	}

	@Override
	public void sendVideoFrame(byte[] buffer) {
		NativeInterface.sendVideo(buffer, buffer.length);
	}

	public void asyncSendFrame(byte[] buffer) {
		NativeInterface.asyncSendVideo(buffer, buffer.length);
	}

	public int startVideo(boolean capture, boolean display) {
		return NativeInterface.startVideo(capture, display);
	}

	public void stopVideo() {
		NativeInterface.stopVideo();
	}

	public void ringingShowImage(int colors[], int width, int height) {
		Log.w(MainActivity.LOG_TAG, "ringingShowImage");
		CallInRingActivity.showImage(colors, width, height);
	}

	public void speakingShowImage(int colors[], int width, int height) {
		SpeakingOutActivity.showImage(colors, width, height);
	}
}
