package cn.edu.hust.buildingtalkback.util;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

public class Entry extends Application {

	private static Context context;

	@Override
	public void onCreate() {

		super.onCreate();
		context = getApplicationContext();
	}

	public static Context getContext() {
		return context;
	}
	
	public static void startActivity(Class<?> cls) {
		startActivity(cls, null);
	}

	public static void startActivity(Class<?> cls, Bundle extras) {

		Intent intent = new Intent(context, cls);
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		if (extras != null)
			intent.putExtras(extras);
		context.startActivity(intent);
	}
}
