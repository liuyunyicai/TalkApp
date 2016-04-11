#include <jni.h>

#ifndef NATIVE_INTERFACE_H
#define NATIVE_INTERFACE_H



/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Media
 * Method:    startAudio
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_startAudio(JNIEnv *env, jclass clazz);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Media
 * Method:    stopAudio
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_stopAudio(JNIEnv *env, jclass clazz);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Media
 * Method:    startVideo
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_startVideo(JNIEnv *env, jclass clazz, jboolean, jboolean);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Media
 * Method:    stopVideo
 * Signature: ()I
 */
JNIEXPORT void JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_stopVideo(JNIEnv *env, jclass clazz);



/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Network
 * Method:    startServer
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_startServer(JNIEnv *env, jclass clazz);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Network
 * Method:    startClient
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_startClient(JNIEnv *env, jclass clazz);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Network
 * Method:    stopNetwork
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_stopNetwork(JNIEnv *env, jclass clazz);



/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Sip
 * Method:    register
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_register(JNIEnv *env, jclass clazz, jstring, jstring, jstring);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Sip
 * Method:    unregister
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_unregister(JNIEnv *env, jclass clazz);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Sip
 * Method:    call
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_makeCall(JNIEnv *env, jclass clazz, jstring);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Sip
 * Method:    answerCall
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_answerCall(JNIEnv *env, jclass clazz);

/*
 * Class:     cn_edu_hust_buildingtalkback_jni_Sip
 * Method:    refuseCall
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_endCall(JNIEnv *env, jclass clazz);



void on_call_busy(JNIEnv *env);

void on_call_ended(JNIEnv *env);

void on_call_ringing(JNIEnv *env, const char *other, int outgoing);

void on_call_established(JNIEnv *env, char *other, int outgoing);

void on_reg_done(JNIEnv *env);

void on_reg_failed(JNIEnv *env);

void show_image(JNIEnv *env, int *colors, int length, int width, int height);

JNIEnv *attach_thread();

void detach_thread();


#endif


