package cn.edu.hust.buildingtalkback.helper;

import android.hardware.Camera;

/**
 * Created by admin on 2016/1/19.
 */
public class CameraHelper {
    // 找到前置摄像头
    public static int FindBackCamera(){
        int cameraCount = 0;
        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
        cameraCount = Camera.getNumberOfCameras(); // 获得摄像头的总数

        int backInx = -1; // 记录后置摄像头的Index
        for (int camIdx = 0; camIdx < cameraCount;camIdx++ ) {
            Camera.getCameraInfo(camIdx, cameraInfo);
            // 如果有前置摄像头，则优先打开前置摄像头
            if (cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_FRONT) {
                // 代表摄像头的方位，目前有定义值两个分别为CAMERA_FACING_FRONT前置和CAMERA_FACING_BACK后置
                return camIdx;
            }
            if (cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_BACK ) {
                backInx = camIdx;
            }
        }
        return backInx;
    }
}
