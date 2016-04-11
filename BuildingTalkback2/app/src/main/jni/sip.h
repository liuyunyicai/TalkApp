#include <jni.h>

#ifndef SIP_H
#define SIP_H

int sip_register(const char *server_addr, const char *username, const char *password);

int sip_unregister();

int sip_make_call(const char *user);

int sip_answer_call();

int sip_end_call();

#endif
