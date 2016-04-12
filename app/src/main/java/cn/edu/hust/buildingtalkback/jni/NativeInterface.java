package cn.edu.hust.buildingtalkback.jni;

import android.app.Activity;
import android.util.Log;

import cn.edu.hust.buildingtalkback.iinterface.IMedia;
import cn.edu.hust.buildingtalkback.iinterface.ISip;
import cn.edu.hust.buildingtalkback.main.MainActivity;
import cn.edu.hust.buildingtalkback.sip.Sip;

public class NativeInterface {

	private static final String LOG_TAG = "BT_NATIVEINTERFACE";

	public static final int NONE = 0;
	public static final int RINGING = 1;
	public static final int SPEAKING = 2;

	private static int status;
	private static Activity activity;

	private static ISip sip;
	private static IMedia media;
//	private static INetwork network;

	static {
		sip = Sip.getInstance();
		media = Media.getInstance();
//        network = Network.getInstance();
	}


	public static void setActivity(Activity act) {
		activity = act;
	}

	// SIP环境初始化
	public static native int sipInit();
	public static native int register(String serverIp, String username, String password);
	public static native int unregister();

	public static native int makeCall(String username);
	public static native int answerCall();
	public static native int endCall();

    // 初始化视频编解码环境
    public static native int initVideo(int camera_width, int camera_height);
    // 发送Video帧数据
    public static native void sendVideo(byte[] data, int buffer_size);
    // 异步发送数据
    public static native void asyncSendVideo(byte[] data, int buffer_size);

	public static native int startAudio();
	public static native void stopAudio();
	public static native int startVideo(boolean capture, boolean display);
	public static native void stopVideo();


	public static native int waitForClient();
	public static native int startClient();
	public static native void stopNetwork();

	public static void onCallBusy() {
		sip.onCallBusy(activity);
		status = NONE;
	}

	public static void onCallEnded() {
		sip.onCallEnded();
		status = NONE;
	}

	public static void onCallRinging(String other, boolean outgoing) {
        sip.onCallRinging(other, outgoing);
	}

	public static void onCallEstablished(String other, boolean outgoing) {
		sip.onCallEstablished(other, outgoing);
	}

	public static void onRegistrationDone(final String targetIP) {
		sip.onRegistrationDone(targetIP);
	}

	public static void onRegistrationFailed(final String targetIP) {
		sip.onRegistrationFailed(targetIP);
	}

	public static void showImage(int colors[], int width, int height) {
		Log.w(MainActivity.LOG_TAG, "Curent status ==" + status);

		if (status == RINGING)
            media.ringingShowImage(colors, width, height);
		else if (status == SPEAKING)
            media.speakingShowImage(colors, width, height);
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
