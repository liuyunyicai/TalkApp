package cn.edu.hust.buildingtalkback.iinterface;

import java.text.SimpleDateFormat;

import android.app.Activity;

public interface ISip {
	// SIP的相关状态
    int SIP_UNREGISTER = -1; // 未注册状态
	int SIP_CONNECTING = 0;  // 正在注册中状态
    int SIP_REGISTERED = 1;  // 已注册状态

    // SIP通话状态
    int SIP_CALL_FREE    = 2; // 空闲状态
    int SIP_CALL_CALLOUT = 3; // 正在呼出电话
    int SIP_CALL_CALLIN  = 4; // 呼出电话


	SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm");

	enum SessionState {

		NONE(false, false),
		OUTGOING_RINGING(false, false),
		OUTGOING_SPEAKING(false, true),
		INCOMING_RINGING(true, false),
		INCOMING_SPEAKING(true, true);

		private boolean income;
		private boolean established;

		private SessionState(boolean income, boolean established) {

			this.income = income;
			this.established = established;
		}

		public boolean isIncome() {
			return income;
		}

		public boolean isEstablished() {
			return established;
		}
	}

	// 初始化SIP环境
	int sip_init();

	// SIP注册
	int register(String serverIp, String username, String password);

	// 解注册
	int unregister();

	// 拨打电话
	int makeCall(String username);

	// 接通电话
	int answerCall();

	// 关闭电话
	int endCall();

	// 拨打用户正在通话中
	void onCallBusy(final Activity act);

	// 结束通话
	void onCallEnded();

	// 当前用户被呼叫
	void onCallRinging(String other, boolean outgoing);

	// 建立呼叫连接的响应
	void onCallEstablished(String other, boolean outgoing);

	// 完成注册的响应
	void onRegistrationDone(final String targetIp);

	// 注册失败的响应
	void onRegistrationFailed(final String targetIp);

}
