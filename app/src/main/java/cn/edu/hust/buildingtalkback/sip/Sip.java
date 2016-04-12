package cn.edu.hust.buildingtalkback.sip;

import java.util.Calendar;

import cn.edu.hust.buildingtalkback.jni.NativeInterface;
import cn.edu.hust.buildingtalkback.main.MainActivity;
import cn.edu.hust.buildingtalkback.activities.RingingActivity;
import cn.edu.hust.buildingtalkback.activities.SpeakingActivity;
import cn.edu.hust.buildingtalkback.iinterface.ISip;
import cn.edu.hust.buildingtalkback.main.MainEvent;
import cn.edu.hust.buildingtalkback.ringing.CalloutRingActivity;
import cn.edu.hust.buildingtalkback.ringing.RingEvent;
import cn.edu.hust.buildingtalkback.speaking.SpeakingEvent;
import cn.edu.hust.buildingtalkback.util.CallRecord;
import de.greenrobot.event.EventBus;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;
import android.widget.Toast;

public class Sip implements ISip{

	private static final String LOG_TAG = "BT_SIP";

	private static int RINGING_REQ = 1;
	private static int SPEAKING_REQ = 2;

	private static String who;
	private static Calendar begin;
	private static SessionState state;

    private static Sip instance = null;

    // 单键模式
    public static Sip getInstance() {
        if (instance == null) {
            synchronized (LOG_TAG) {
                if (instance == null) {
                    instance = new Sip();
                }
            }
        }
        return instance;
    }

	@Override
	// SIP环境初始化
	public int sip_init() {
		return NativeInterface.sipInit();
	}

	public int register(String serverIp, String username, String password) {
		return NativeInterface.register(serverIp, username, password);
	}
	public int unregister() {
		return NativeInterface.unregister();
	}

    // 拨打电话
	public int makeCall(String target_name) {
		return NativeInterface.makeCall(target_name);
	}

	public int answerCall() {
		return NativeInterface.answerCall();
	}

	public int endCall() {
		return NativeInterface.endCall();
	}



	public void onCallBusy(final Activity act) {

		// 提示呼叫的用户正忙
        EventBus.getDefault().post(new SipEvent(SipEvent.SIP_CALLIN_BUSY));

		onCallEnded();
	}

	public void onCallEnded() {

//		Log.d(LOG_TAG, "onCallEnded: " + state.toString());

//		String beginString = formatter.format(begin.getTime());
//		CallRecord record = new CallRecord(act);
//
//		if (state.isEstablished()) {
//
//			Calendar calendar = Calendar.getInstance();
//			int duration = (int) (calendar.getTimeInMillis() - begin.getTimeInMillis()) / 1000;
//
//			if (state.isIncome())
//				record.addIncomingCall(who, beginString, duration);
//			else
//				record.addOutgoingCall(who, beginString, duration);
//
////			act.finishActivity(SPEAKING_REQ);
//
//		}
//		else {
//
//			if (state.isIncome())
//				record.addMissedCall(who, beginString);
//			else
//				record.addOutgoingCall(who, beginString, 0);
//
////			act.finishActivity(RINGING_REQ);
//		}

        // 销毁Activity（被关闭一方）
        Log.w(MainActivity.LOG_TAG, "onCallEnded");
        EventBus.getDefault().post(new RingEvent(RingEvent.RING_HANGOUT_SUCCESS));

        EventBus.getDefault().post(new SpeakingEvent(SpeakingEvent.RING_HANGOUT_SUCCESS));

		state = SessionState.NONE;
//		record.close();
	}

	public void onCallRinging(String other, boolean outgoing) {
		who = other;
		begin = Calendar.getInstance();

		if (outgoing) {
			state = SessionState.OUTGOING_RINGING;
		}
		else {
			state = SessionState.INCOMING_RINGING;
		}

		Log.w(MainActivity.LOG_TAG, "Call Type ==" + (outgoing ? SipEvent.SIP_CALLOUT_RINGING : SipEvent.SIP_CALLIN_RINGING));
        if (outgoing) {
            CalloutRingActivity.isClientConnected = true;
            Log.w(MainActivity.LOG_TAG, "CalloutRingActivity.isClientConnected == " + CalloutRingActivity.isClientConnected);
        } else {
            EventBus.getDefault().post(new MainEvent(SipEvent.SIP_CALLIN_RINGING, other));
        }

        // 向SipService发送呼出电话、呼入电话消息
//        EventBus.getDefault().post(
//                new MainEvent(outgoing ? SipEvent.SIP_CALLOUT_RINGING : SipEvent.SIP_CALLIN_RINGING, other));
	}

	public void onCallEstablished(String other, boolean outgoing) {
		who = other;
		begin = Calendar.getInstance();

		if (outgoing) {
			state = SessionState.OUTGOING_SPEAKING;
		}
		else {
			state = SessionState.INCOMING_SPEAKING;
		}

        Log.w(MainActivity.LOG_TAG, "onCallEstablished send Event == " +
                (outgoing ? RingEvent.RING_CALLOUT_RECV_SUCCESS : RingEvent.RING_CALLIN_RECV_SUCCESS));
        // 发送处理消息
        EventBus.getDefault().post(new RingEvent(
                outgoing ? RingEvent.RING_CALLOUT_RECV_SUCCESS : RingEvent.RING_CALLIN_RECV_SUCCESS, other));
	}


	public void onRegistrationDone(final String targetIp) {
		Log.w(MainActivity.LOG_TAG, targetIp + " SIP_REGISTER_SUCCESS");
		EventBus.getDefault().post(new SipEvent(SipEvent.SIP_REGISTER_SUCCESS, targetIp));
	}

	public void onRegistrationFailed(final String targetIp) {
		Log.w(MainActivity.LOG_TAG, targetIp + "SIP_REGISTER_FAIL");
		EventBus.getDefault().post(new SipEvent(SipEvent.SIP_REGISTER_FAIL, targetIp));
	}
}
