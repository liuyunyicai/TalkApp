package cn.edu.hust.buildingtalkback.viewhelper;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import android.widget.LinearLayout;

import cn.edu.hust.buildingtalkback.main.MainActivity;


/**
 * Created by admin on 2015/12/17.
 */
public class HorizontalScrollLayout extends LinearLayout {
    private float mTouchSlop;

    private float mLastX, mLastY;

    public HorizontalScrollLayout(Context context) {
        super(context);
        mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
    }

    public HorizontalScrollLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
    }

    public HorizontalScrollLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        boolean isIntercepted = false;
        float curX = ev.getX();
        float curY = ev.getY();

        switch (ev.getAction()) {
            case MotionEvent.ACTION_DOWN:
                mLastX = ev.getX();
                mLastY = ev.getY();
                break;
            case MotionEvent.ACTION_MOVE:
                // 判断是否是以水平滑动为主
                float deltaX = curX - mLastX;
                float deltaY = curY - mLastY;

                if (Math.abs(deltaX) > (Math.abs(deltaY) + mTouchSlop))
                    isIntercepted = true;
                else
                    isIntercepted = false;
                break;
            case MotionEvent.ACTION_UP:
                isIntercepted = false;
                break;
        }

        mLastX = curX;
        mLastY = curY;
        Log.i(MainActivity.LOG_TAG, "isIntercepted == " + isIntercepted);
        return isIntercepted;
    }
}
