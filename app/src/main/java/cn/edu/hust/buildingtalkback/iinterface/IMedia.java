package cn.edu.hust.buildingtalkback.iinterface;

/** 音频视频相关辅助接口 */
public interface IMedia {

	// 开启音频
	int startAudio();

	// 关闭音频
	void stopAudio();

    // 初始化视频编解码环境
    int initVideo(int camera_width, int camera_height);

	// 发送每一帧视频数据
    void sendVideoFrame(byte[] buffer);

	// 异步发送帧数据
    void asyncSendFrame(byte[] buffer);

	// 开始视频
	int startVideo(boolean capture, boolean display);

	// 关闭视频
	void stopVideo();

	// 响铃状态的视频显示
	void ringingShowImage(int colors[], int width, int height);

	// 通话状态的视频显示
	void speakingShowImage(int colors[], int width, int height);
}
