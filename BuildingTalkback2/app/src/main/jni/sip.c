#include <android/log.h>

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>

#include "sip.h"
#include "network.h"
#include "native_interface.h"

#define ENABLE_TRACE

#include "libexosip/include/eXosip2/eXosip.h"


#define LOG_TAG		"BT_NATIVE_SIP"
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_FUNC()	LOGD("current line: %d, function: %s", __LINE__, __func__)

// #define LOG_EVENT(e)                                            \
//     LOGD("eXosip event coming, type = %d", e->type);   			\
//     if (e->request)     LOG_OSIP_EVENT(request, e->request);    \
//     if (e->response)    LOG_OSIP_EVENT(response, e->response);  \
//     if (e->ack)         LOG_OSIP_EVENT(ack, e->ack)

#define LOG_EVENT(e)		LOGD("eXosip event coming, type = %d", e->type)

#define LOG_OSIP_EVENT(p,e)		LOGD("----" #p ": %d, %s", e->status_code, e->reason_phrase)


#define LOCK()		eXosip_lock(ctx)
#define UNLOCK()	eXosip_unlock(ctx)

static struct eXosip_t *ctx = NULL;

static osip_message_t *ack = NULL;
static osip_message_t *reg = NULL;
static osip_message_t *invite = NULL;
static osip_message_t *answer = NULL;

static pthread_t event_handler;

static int sip_on;

static int reg_id;

static int cid;
static int did;
static int tid;

static char usr[32];
static char psw[32];
static char srv[32];

static char contact_ip[128];


void set_ids(int _cid, int _did, int _tid)
{
	cid = _cid;
	did = _did;
	tid = _tid;
}

void reset_ids()
{
	cid = -1;
	did = -1;
	tid = -1;
}


int end_call(int _cid, int _did)
{
	int res;

	LOCK();
	LOGD("%s, _cid = %d, cid = %d, _did = %d, did = %d", __func__, _cid, cid, _did, did);
	res = eXosip_call_terminate(ctx, _cid, _did);
	UNLOCK();

	return res;
}

void call_ended(JNIEnv *env)
{
	on_call_ended(env);
	reset_ids();
}


void resolve_ip(osip_message_t *msg)
{
    char *key = "c=IN IP4 ";

    osip_body_t *body;
    osip_message_get_body(msg, 0, &body);

    char *start = strstr(body->body, key) + strlen(key);
    char *contact = contact_ip;

    while (*start != '\0' && (isdigit(*start) || *start == '.'))
        *contact++ = *start++;
    *contact = '\0';

    LOGD("resolved ip = %s", contact_ip);
}


void *event_handler_entry(void *arg)
{
	JNIEnv *env = attach_thread();

	int ret;
	eXosip_event_t *event;

	reset_ids();

	while (sip_on)
	{
		event = eXosip_event_wait(ctx, 1, 0);

		if (event == NULL)
			continue;

		eXosip_lock(ctx);
		eXosip_default_action(ctx, event);
		eXosip_automatic_action(ctx);
		eXosip_unlock(ctx);

		if (event == NULL)
			continue;

		LOGD("event->type = %2d, info = %s", event->type, event->textinfo);

		switch (event->type)
		{
		case EXOSIP_REGISTRATION_SUCCESS:   // event_type = 1

			on_reg_done(env);
			break;

		case EXOSIP_REGISTRATION_FAILURE:   //  event_type = 2

			if (event->response->status_code != 401 && event->response->status_code != 407)
				on_reg_failed(env);
			break;

		case EXOSIP_CALL_INVITE:            // event_tpye = 5

			if (cid == -1)
			{
				set_ids(event->cid, event->did, event->tid);

				LOCK();
				eXosip_call_send_answer(ctx, event->tid, 180, NULL);
				UNLOCK();

				resolve_ip(event->request);
				set_server_ip(contact_ip);

				on_call_ringing(env, event->response->from->url->username, 0);
			}
			else
			{
				sip_end_call(event->cid, event->did);
			}

			break;


		case EXOSIP_CALL_RINGING:           // event_type = 9

			set_ids(event->cid, event->did, event->tid);
			on_call_ringing(env, event->response->to->url->username, 1);
			break;


		case EXOSIP_CALL_ANSWERED:          // event_type = 10

			LOCK();
			eXosip_call_build_ack(ctx, event->did, &ack);
			eXosip_call_send_ack(ctx, event->did, ack);
			UNLOCK();

			LOGD("on_call_established, username = %s", event->response->to->url->username);
			on_call_established(env, event->response->to->url->username, 1);
			break;


		case EXOSIP_CALL_REQUESTFAILURE:    // event_type = 12

			LOGD("EXOSIP_CALL_REQUESTFAILURE, status_code = %d", event->response->status_code);
			switch (event->response->status_code)
			{
			case 486:
				on_call_busy(env);
				reset_ids();
				break;

			case 480:
			case 487:
			case 408:
				on_call_ended(env);
				reset_ids();
				break;

			default:
				break;
			}

			break;

		case EXOSIP_CALL_GLOBALFAILURE:

			switch (event->response->status_code)
			{
			case 603:
				on_call_ended(env);
				break;

			default:
				break;
			}
			break;


		case EXOSIP_CALL_ACK:               // event_type = 15

			LOGD("on_call_established, username = %s", event->response->from->url->username);
			on_call_established(env, event->response->from->url->username, 0);
			break;


		case EXOSIP_CALL_CANCELLED:         // event_type = 16
			break;

		case EXOSIP_CALL_MESSAGE_ANSWERED:	// event_type = 20
		case EXOSIP_CALL_CLOSED:            // event_type = 25

			if (event->cid == cid)
			{
				on_call_ended(env);
				reset_ids();
			}
			break;


		case EXOSIP_CALL_RELEASED:          // event_type = 26
			break;

		default:
			break;
		}

		eXosip_event_free(event);
	}

	detach_thread();
	return NULL;
}

int sip_init()
{
	int res;
	TRACE_INITIALIZE(9, NULL);

	ctx = eXosip_malloc();
	if (ctx == NULL)
	{
		LOGD("eXosip_malloc() failed");
		return -1;
	}

	res = eXosip_init(ctx);
	if (res)
	{
		LOGD("eXosip_init() failed, %d", res);
		return -1;
	}

	res = eXosip_listen_addr(ctx, IPPROTO_UDP, NULL, 5060, AF_INET, 0);
	if (res)
	{
		LOGD("eXosip_listen_addr() failed, %d", res);
		eXosip_quit(ctx);
		return -1;
	}

	sip_on = 1;
	res = pthread_create(&event_handler, NULL, event_handler_entry, NULL);
	if (res != 0)
	{
		LOGD("pthread_create() failed, %d", res);
		eXosip_quit(ctx);
		return -1;
	}

	LOGD("%s done", __func__);

	return 0;
}

void sip_uninit()
{
	usleep(500000);
	sip_on = 0;
	pthread_join(event_handler, NULL);

	LOCK();
	eXosip_quit(ctx);
	UNLOCK();
}

/*****	External API Calls	******/

int sip_register(const char *serverip, const char *username, const char *password)
{
	LOG_FUNC();

	if (sip_init() != 0)
		return -1;

	int res;
	char local_ip[128];
	char sip_srv[128];
	char sip_usr_srv[128];

	strcpy(usr, username);
	strcpy(psw, password);
	strcpy(srv, serverip);

	eXosip_guess_localip(ctx, AF_INET, local_ip, 128);

	sprintf(sip_srv, "sip:%s", srv);
	sprintf(sip_usr_srv, "sip:%s@%s", usr, local_ip);

	LOCK();

	eXosip_clear_authentication_info(ctx);
	eXosip_add_authentication_info(ctx, usr, usr, psw, "MD5", NULL);

	LOGD("begin to register: server ip = %s, username = %s, password = %s", srv, usr, psw);

	reg_id = eXosip_register_build_initial_register(ctx, sip_usr_srv, sip_srv, NULL, 1800, &reg);
	if (reg_id < 0)
	{
		UNLOCK();
		LOGD("eXosip_register_build_initial_register() failed, %d", reg_id);
		return -1;
	}

	res = eXosip_register_send_register(ctx, reg_id, reg);
	if (res != 0)
	{
		UNLOCK();
		LOGD("eXosip_register_send_register() failed, %d", res);
		return -1;
	}

	UNLOCK();

	return 0;
}

int sip_unregister()
{
	int res;

	LOCK();

	res = eXosip_register_build_register(ctx, reg_id, 0, &reg);
	if (res < 0)
	{
		LOGD("eXosip_register_build_register() failed, %d", res);
		goto UNINIT;
	}
	eXosip_register_send_register(ctx, reg_id, reg);

	UNLOCK();

UNINIT:
	sip_uninit();

	return 0;
}

int sip_make_call(const char *user)
{
	int res;

	char local_ip[128];
	char to[128], from[128];

	eXosip_guess_localip(ctx, AF_INET, local_ip, 128);
	snprintf(to, 128, "<sip:%s@%s>", user, srv);
	snprintf(from, 128, "<sip:%s@%s>", usr, local_ip);

	LOCK();

	res = eXosip_call_build_initial_invite(ctx, &invite, to, from, NULL, "Test");
	if (res != 0)
	{
		LOGD("eXosip_call_build_initial_invite() failed, %d", res);
		UNLOCK();
		return -1;
	}

	{
		char tmp[4096];
		char local_ip[128];
		eXosip_guess_localip(ctx, AF_INET, local_ip, 128);
		LOGD("guess_localip = %s", local_ip);
		snprintf(tmp, 4096,
		         "o=%s@%s 0 0 IN IP4 %s\r\n"
		         "s=Session SIP/SDP\r\n"
		         "c=IN IP4 %s\r\n"
		         "t=0 0\r\n"
		         "m=audio 21000 RTP/AVP 8 0 101"
		         "a=rtpmap:8 PCMA/8000"
		         "a=rtpmap:0 PCMU/8000"
		         "a=rtpmap:101 telephone-event/8000"
		         "a=fmtp:101 0-15"
		         "m=video 21070 RTP/AVP 103"
		         "a=rtpmap:103 h263-1998/90000",
		         usr, srv, local_ip, local_ip);
		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_message_set_content_type(invite, "application/sdp");
	}

	osip_message_set_supported(invite, "100rel");

	cid = eXosip_call_send_initial_invite(ctx, invite);
	UNLOCK();

	return 0;
}

int sip_answer_call()
{
	int res;

	LOCK();

	res = eXosip_call_build_answer(ctx, tid, 200, &answer);
	if (res != 0)
	{
		res = eXosip_call_send_answer(ctx, tid, 400, NULL);
	}
	else
	{
		res = eXosip_call_send_answer(ctx, tid, 200, answer);
	}

	LOGD("eXosip_call_send_answer(), %d", res);

	UNLOCK();

	return 0;
}

int sip_end_call()
{
	return end_call(cid, did);
}