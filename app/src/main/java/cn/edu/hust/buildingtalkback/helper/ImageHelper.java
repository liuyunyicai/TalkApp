package cn.edu.hust.buildingtalkback.helper;

import android.graphics.Canvas;
import android.util.Log;
import android.view.SurfaceHolder;

import cn.edu.hust.buildingtalkback.main.MainActivity;

/**
 * Created by admin on 2016/1/18.
 */
public class ImageHelper {

    public static void YUV420spRotate90(byte[]dst, byte[] src, int srcWidth, int height) {
        int nWidth = 0, nHeight = 0;
        int wh = 0;
        int uvHeight = 0;
        if(srcWidth != nWidth || height != nHeight)
        {
            nWidth = srcWidth;
            nHeight = height;
            wh = srcWidth * height;
            uvHeight = height >> 1;//uvHeight = height / 2
        }

        //旋转Y
        int k = 0;
        for(int i = 0; i < srcWidth; i++){
            int nPos = srcWidth - 1;
            for(int j = 0; j < height; j ++) {
                dst[k] = src[nPos - i];
                k++;
                nPos += srcWidth;
            }
        }

        // 旋转UV
        for(int i = 0; i < srcWidth; i+=2){
            int nPos = wh + srcWidth - 1;
            for(int j = 0; j < uvHeight; j++) {
                dst[k] = src[nPos - i - 1];
                dst[k + 1] = src[nPos - i];
                k += 2;
                nPos += srcWidth;
            }
        }
    }

    // 显示图片
    public static void showImage(SurfaceHolder holder, int colors[], int width, int height) {
        try {
            if (holder != null) {
                Canvas canvas = holder.lockCanvas();
                canvas.drawBitmap(colors, 0, width, 0, 0, width, height, false, null);
                holder.unlockCanvasAndPost(canvas);
            }
        } catch (Exception e) {
            Log.e(MainActivity.LOG_TAG, "showImage Error:" + e.toString());
        }
    }
}
