#include <jni.h>

#ifndef VIDEO_H
#define VIDEO_H

int start_video(int capture, int display);

void stop_video();

// 初始化Video相关的环境设置
int initVideo(int width, int height);

// 直接发送Video视频帧
void sendVideoFrame(uint8_t* data, int buf_size);


#endif
