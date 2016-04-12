package cn.edu.hust.buildingtalkback.util;

public enum CallType {

	INCOMING(0),
	OUTGOING(1),
	MISSED(2),
	ALL(3);
	
	private int flag;
	
	private CallType(int flag) {
		this.flag = flag;
	}

	public int getInt() {
		return flag;
	}
}
