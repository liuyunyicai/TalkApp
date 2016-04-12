package cn.edu.hust.buildingtalkback.jni;

import android.util.Log;

import cn.edu.hust.buildingtalkback.iinterface.INetwork;
import cn.edu.hust.buildingtalkback.main.MainActivity;

public class Network implements INetwork{

	// 单件模式
	private static Network instance = null;

	public static Network getInstance() {
		if (instance == null)
			instance = new Network();
		return instance;
	}

	public int waitForClient() {
		return NativeInterface.waitForClient();
	}

	public int startClient() {
		Log.d(MainActivity.LOG_TAG, "Network # startClient");
		return NativeInterface.startClient();
	}

	public void stopNetwork() {
		NativeInterface.stopNetwork();
	}
}
