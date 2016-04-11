package cn.edu.hust.buildingtalkback.jni;

import cn.edu.hust.buildingtalkback.RingingActivity;
import cn.edu.hust.buildingtalkback.SpeakingActivity;

public class Media {

	public static int startAudio() {
		return NativeInterface.startAudio();
	}

	public static void stopAudio() {
		NativeInterface.stopAudio();
	}

	public static int startVideo(boolean capture, boolean display) {
		return NativeInterface.startVideo(capture, display);
	}

	public static void stopVideo() {
		NativeInterface.stopVideo();
	}
	
	public static void ringingShowImage(int colors[], int width, int height) {
		RingingActivity.showImage(colors, width, height);
	}

	public static void speakingShowImage(int colors[], int width, int height) {
		SpeakingActivity.showImage(colors, width, height);
	}
}
