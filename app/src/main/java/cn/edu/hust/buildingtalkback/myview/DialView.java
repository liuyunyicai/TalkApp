package cn.edu.hust.buildingtalkback.myview;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.widget.LinearLayout;

import cn.edu.hust.buildingtalkback.R;

/**
 * Created by admin on 2016/1/15.
 */
public class DialView extends LinearLayout {
    private String bt_num_str, bt_txt_str;

    public DialView(Context context) {
        super(context);
    }

    public DialView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public DialView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        // 获取自定义的参数
        TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.DialButtonStyle, defStyleAttr, 0);
        // 获取两个String
        bt_num_str = ta.getString(R.styleable.DialButtonStyle_bt_num);
        bt_txt_str = ta.getString(R.styleable.DialButtonStyle_bt_txt);

        ta.recycle();
    }
}
