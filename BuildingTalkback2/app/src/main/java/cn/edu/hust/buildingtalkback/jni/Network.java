package cn.edu.hust.buildingtalkback.jni;

public class Network {

	public static int waitForClient() {
		return NativeInterface.waitForClient();
	}
	
	public static int startClient() {
		return NativeInterface.startClient();
	}
	
	public static void stopNetwork() {
		NativeInterface.stopNetwork();
	}
}
