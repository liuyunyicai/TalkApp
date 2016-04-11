#include <stdio.h>

#include <android/log.h>

#include "sip.h"
#include "audio.h"
#include "video.h"
#include "network.h"


#define LOG_TAG		"BT_NATIVE_INTERFACE"
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_FUNC()	LOGD("current line: %d, function: %s", __LINE__, __func__)

JavaVM *jvm;

jclass native_interface;			/*	NativeInterface.java	*/

jmethodID mid_on_reg_done;
jmethodID mid_on_reg_failed;

jmethodID mid_on_call_busy;
jmethodID mid_on_call_ended;
jmethodID mid_on_call_ringing;
jmethodID mid_on_call_established;

jmethodID mid_show_image;
jintArray rgb_array;


const char *get_string(JNIEnv *env, jstring string)
{
	return (*env)->GetStringUTFChars(env, string, NULL);
}

void release_string(JNIEnv *env, jstring string, const char *str)
{
	(*env)->ReleaseStringUTFChars(env, string, str);
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env = NULL;
	jint result;

	if ((*vm)->GetEnv(vm, (void *) &env, JNI_VERSION_1_6) != JNI_OK)
	{
		LOGD("get JNIEnv failed, func: %s", __func__);
		return -1;
	}

	jvm = vm;

	jclass clazz			= (*env)->FindClass(env, "cn/edu/hust/buildingtalkback/jni/NativeInterface");
	native_interface		= (*env)->NewGlobalRef(env, clazz);

	mid_on_reg_done			= (*env)->GetStaticMethodID(env, native_interface, "onRegistrationDone", "()V");
	mid_on_reg_failed		= (*env)->GetStaticMethodID(env, native_interface, "onRegistrationFailed", "()V");

	mid_on_call_busy		= (*env)->GetStaticMethodID(env, native_interface, "onCallBusy", "()V");
	mid_on_call_ended		= (*env)->GetStaticMethodID(env, native_interface, "onCallEnded", "()V");
	mid_on_call_ringing		= (*env)->GetStaticMethodID(env, native_interface, "onCallRinging", "(Ljava/lang/String;Z)V");
	mid_on_call_established = (*env)->GetStaticMethodID(env, native_interface, "onCallEstablished", "(Ljava/lang/String;Z)V");

	mid_show_image 			= (*env)->GetStaticMethodID(env, native_interface, "showImage", "([III)V");

	rgb_array				= (*env)->NewIntArray(env, 704 * 576);
	rgb_array				= (*env)->NewGlobalRef(env, rgb_array);

	LOGD("func %s done", __func__);

	return JNI_VERSION_1_6;
}


JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void* reserved)
{
	JNIEnv *env = NULL;
	jint result;

	if ((*vm)->GetEnv(vm, (void *) &env, JNI_VERSION_1_6) != JNI_OK)
	{
		LOGD("get JNIEnv failed, func: %s", __func__);
		return;
	}

	(*env)->DeleteGlobalRef(env, native_interface);

	LOGD("func %s done", __func__);
}


JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_startAudio(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	return start_audio();
}


JNIEXPORT void JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_stopAudio(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	stop_audio();
}


JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_startVideo(
    JNIEnv *env, jclass clazz, jboolean capture, jboolean display)
{
	LOG_FUNC();

	int cap = (capture == JNI_TRUE);
	int dis = (display == JNI_TRUE);

	return start_video(cap, dis);
}


JNIEXPORT void JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_stopVideo(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	stop_video();
}


JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_waitForClient(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	return wait_for_client();
}


JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_startClient(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	return start_client("192.168.1.102");
}


JNIEXPORT void JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_stopNetwork(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	stop_network();
}


JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_register(
    JNIEnv *env, jclass clazz, jstring serverip, jstring username, jstring password)
{
	LOG_FUNC();

	const char *srv = get_string(env, serverip);
	const char *usr = get_string(env, username);
	const char *psw = get_string(env, password);

	int result = sip_register(srv, usr, psw);

	release_string(env, password, psw);
	release_string(env, username, usr);
	release_string(env, serverip, srv);

	return result;
}


JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_unregister(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	return sip_unregister();
}

JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_makeCall(
    JNIEnv *env, jclass clazz, jstring username)
{
	LOG_FUNC();

	const char *usr = get_string(env, username);

	int result = sip_make_call(usr);

	release_string(env, username, usr);

	return result;
}


JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_answerCall(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	return sip_answer_call();
}


JNIEXPORT jint JNICALL Java_cn_edu_hust_buildingtalkback_jni_NativeInterface_endCall(
    JNIEnv *env, jclass clazz)
{
	LOG_FUNC();
	return sip_end_call();
}


JNIEnv *attach_thread()
{
	JNIEnv *env = NULL;

	(*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_6);
	(*jvm)->AttachCurrentThread(jvm, &env, NULL);

	return env;
}

void detach_thread()
{
	(*jvm)->DetachCurrentThread(jvm);
}


/*	call related callback functions	*/
void on_call_busy(JNIEnv *env)
{
	(*env)->CallStaticVoidMethod(env, native_interface, mid_on_call_busy);
}

void on_call_ended(JNIEnv *env)
{
	LOG_FUNC();
	(*env)->CallStaticVoidMethod(env,native_interface, mid_on_call_ended);
}

void on_call_ringing(JNIEnv *env, char *other, int outgoing)
{
	jstring str = (*env)->NewStringUTF(env, other);
	jboolean bl = outgoing ? JNI_TRUE : JNI_FALSE;

	(*env)->CallStaticVoidMethod(env, native_interface, mid_on_call_ringing, str, bl);

	(*env)->DeleteLocalRef(env, str);
}

void on_call_established(JNIEnv *env, char *other, int outgoing)
{
	jstring str = (*env)->NewStringUTF(env, other);
	jboolean bl = outgoing ? JNI_TRUE : JNI_FALSE;

	(*env)->CallStaticVoidMethod(env, native_interface, mid_on_call_established, str, bl);

	(*env)->DeleteLocalRef(env, str);
}


/*	registration related callback functions	*/
void on_reg_done(JNIEnv *env)
{
	(*env)->CallStaticVoidMethod(env, native_interface, mid_on_reg_done);
}

void on_reg_failed(JNIEnv *env)
{
	(*env)->CallStaticVoidMethod(env, native_interface, mid_on_reg_failed);
}


/*	video display related functions			*/

void show_image(JNIEnv *env, int *colors, int length, int width, int height)
{
	(*env)->SetIntArrayRegion(env, rgb_array, 0, length, (jint *) colors);
	(*env)->CallStaticVoidMethod(env, native_interface, mid_show_image, rgb_array, width, height);
}