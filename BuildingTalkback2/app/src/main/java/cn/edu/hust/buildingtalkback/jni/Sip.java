package cn.edu.hust.buildingtalkback.jni;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;

import cn.edu.hust.buildingtalkback.RingingActivity;
import cn.edu.hust.buildingtalkback.SpeakingActivity;
import cn.edu.hust.buildingtalkback.util.CallRecord;
import cn.edu.hust.buildingtalkback.util.Entry;
import android.R.integer;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

public class Sip {

	private static final String LOG_TAG = "BT_SIP";

	private static int RINGING_REQ = 1;
	private static int SPEAKING_REQ = 2;

	private static String who;
	private static Calendar begin;
	private static SessionState state;
	
	private static SimpleDateFormat formatter;
	
	static {
		formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm");
	}

	public static int register(String serverIp, String username, String password) {
		return NativeInterface.register(serverIp, username, password);
	}
	public static int unregister() {
		return NativeInterface.unregister();
	}

	public static int makeCall(String username) {
		return NativeInterface.makeCall(username);
	}

	public static int answerCall() {
		return NativeInterface.answerCall();
	}

	public static int endCall() {
		return NativeInterface.endCall();
	}



	public static void onCallBusy(final Activity act) {

		act.runOnUiThread(new Runnable() {

			@Override
			public void run() {
				Toast.makeText(act, "Subscriber you dial is busy",
						Toast.LENGTH_SHORT).show();
			}
		});

		onCallEnded(act);
	}

	public static void onCallEnded(final Activity act) {

		Log.d(LOG_TAG, "onCallEnded: " + state.toString());
		
		String beginString = formatter.format(begin.getTime());
		CallRecord record = new CallRecord(act);
		
		if (state.isEstablished()) {
			
			Calendar calendar = Calendar.getInstance();
			int duration = (int) (calendar.getTimeInMillis() - begin.getTimeInMillis()) / 1000;
			
			if (state.isIncome())
				record.addIncomingCall(who, beginString, duration);
			else
				record.addOutgoingCall(who, beginString, duration);

			act.finishActivity(SPEAKING_REQ);
		}
		else {
			
			if (state.isIncome())
				record.addMissedCall(who, beginString);
			else
				record.addOutgoingCall(who, beginString, 0);
			
			act.finishActivity(RINGING_REQ);
		}
		
		state = SessionState.NONE;
		record.close();
	}

	public static void onCallRinging(final Activity act, String other, boolean outgoing) {

		who = other;
		begin = Calendar.getInstance();
		
		if (outgoing) {
			state = SessionState.OUTGOING_RINGING;
		}
		else {
			state = SessionState.INCOMING_RINGING;
		}
		
		Intent intent = new Intent(act, RingingActivity.class);
		intent.putExtra("other", other);
		intent.putExtra("outgoing", outgoing);
		act.startActivityForResult(intent, RINGING_REQ);
	}

	public static void onCallEstablished(final Activity act, String other, boolean outgoing) {

		who = other;
		begin = Calendar.getInstance();
		
		if (outgoing) {
			state = SessionState.OUTGOING_SPEAKING;
		}
		else {
			state = SessionState.INCOMING_SPEAKING;
		}
		
		act.finishActivity(RINGING_REQ);
		
		Intent intent = new Intent(act, SpeakingActivity.class);
		intent.putExtra("other", other);
		intent.putExtra("outgoing", outgoing);
		act.startActivityForResult(intent, SPEAKING_REQ);
	}


	public static void onRegistrationDone(final Activity act) {

		act.runOnUiThread(new Runnable() {

			@Override
			public void run() {
				Toast.makeText(act, "Registration Succeed",
						Toast.LENGTH_SHORT).show();
			}
		});
	}

	public static void onRegistrationFailed(final Activity act) {

		act.runOnUiThread(new Runnable() {

			@Override
			public void run() {
				Toast.makeText(act, "Registration Failed",
						Toast.LENGTH_SHORT).show();
			}
		});
	}


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
}
