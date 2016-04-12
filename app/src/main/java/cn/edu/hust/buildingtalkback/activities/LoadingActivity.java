package cn.edu.hust.buildingtalkback.activities;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Window;
import android.widget.ImageView;
import android.widget.TextView;

import cn.edu.hust.buildingtalkback.R;
import cn.edu.hust.buildingtalkback.main.MainActivity;
import cn.edu.hust.buildingtalkback.sip.SipService;

/**
 * Created by admin on 2015/11/19.
 */

/**
 * 应用加载界面
 **/
public class LoadingActivity extends Activity {

    private ImageView loadingImg;
    private TextView loadingProgress;
    // 加载动画时间
    private final static int DURATION_TIME = 3000;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.loading_layout);

        initView();

        initService();
    }

    // 初始化Servive
    private void initService() {
        startService(new Intent(this, SipService.class));
    }

    // 初始化界面
    private void initView() {
        loadingImg = (ImageView) findViewById(R.id.loadingImg);
        loadingProgress = (TextView) findViewById(R.id.loadingProgress);

        // 组合形式使用动画
        PropertyValuesHolder pvhX = PropertyValuesHolder.ofFloat("scaleX", 1f, 1.3f);
        PropertyValuesHolder pvhY = PropertyValuesHolder.ofFloat("scaleY", 1f, 1.3f);
        ObjectAnimator animator = ObjectAnimator.ofPropertyValuesHolder(loadingImg, pvhX, pvhY);
        animator.setDuration(DURATION_TIME);
        // 添加动画监听器
        animator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                startActivity(new Intent(LoadingActivity.this, MainActivity.class));
                LoadingActivity.this.finish();
            }
        });
        animator.start();

        ObjectAnimator.ofFloat(loadingProgress, "scaleX", 0f, 1.0f).setDuration(DURATION_TIME).start();
    }
}
