package cn.edu.hust.buildingtalkback.jni;

import android.R.integer;
import android.app.Activity;
import android.util.Log;

public class NativeInterface {
	
	private static final String LOG_TAG = "BT_NATIVEINTERFACE";
	
	public static final int NONE = 0;
	public static final int RINGING = 1;
	public static final int SPEAKING = 2;
	
	private static int status;
	
	private static Activity activity;
	
	public static void setActivity(Activity act) {
		activity = act;
	}

	public static native int register(String serverIp, String username, String password);
	public static native int unregister();

	public static native int makeCall(String username);
	public static native int answerCall();
	public static native int endCall();


	public static native int startAudio();
	public static native void stopAudio();
	public static native int startVideo(boolean capture, boolean display);
	public static native void stopVideo();


	public static native int waitForClient();
	public static native int startClient();
	public static native void stopNetwork();

	public static void onCallBusy() {
		Sip.onCallBusy(activity);
		status = NONE;
	}
	
	public static void onCallEnded() {
		Sip.onCallEnded(activity);
		status = NONE;
	}
	
	public static void onCallRinging(String other, boolean outgoing) {
		Sip.onCallRinging(activity, other, outgoing);
	}
	
	public static void onCallEstablished(String other, boolean outgoing) {
		Sip.onCallEstablished(activity, other, outgoing);
	}

	public static void onRegistrationDone() {
		Sip.onRegistrationDone(activity);
	}
	
	public static void onRegistrationFailed() {
		Sip.onRegistrationFailed(activity);
	}
	
	public static void showImage(int colors[], int width, int height) {

		if (status == RINGING)
			Media.ringingShowImage(colors, width, height);
		else if (status == SPEAKING)
			Media.speakingShowImage(colors, width, height);
	}
	
	public static void setStatus(int status) {
		NativeInterface.status = status;
	}

	private static String strLib[] = {
		"osip",
		"exosip",
		"avutil-52",
		"avcodec-55",
		"swscale-2",
		"native-interface"
	};

	static {
		for (String str : strLib)
			System.loadLibrary(str);
	}

}
