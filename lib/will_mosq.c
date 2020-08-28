/*
Copyright (c) 2010-2020 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include "config.h"

#include <stdio.h>
#include <string.h>

#ifdef WITH_BROKER
#include "mosquitto_broker_internal.h"
#endif

#include "mosquitto.h"
#include "mosquitto_internal.h"
#include "logging_mosq.h"
#include "messages_mosq.h"
#include "memory_mosq.h"
#include "mqtt_protocol.h"
#include "net_mosq.h"
#include "read_handle.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "will_mosq.h"

/*   将遗嘱消息保存到对应客户端的上下文中    */
int will__set(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, 
                                        int qos, bool retain, mosquitto_property *properties)
{
    int rc = MOSQ_ERR_SUCCESS;
    mosquitto_property *p;

    if(!mosq || !topic) return MOSQ_ERR_INVAL;
    if(payloadlen < 0 || payloadlen > MQTT_MAX_PAYLOAD) return MOSQ_ERR_PAYLOAD_SIZE;
    if(payloadlen > 0 && !payload) return MOSQ_ERR_INVAL;

    /* 检查遗嘱主题是否有效 */
    if(mosquitto_pub_topic_check(topic)) return MOSQ_ERR_INVAL;
    /* 检查遗嘱主题是否是有效的UTF-8编码字符串 */
    if(mosquitto_validate_utf8(topic, strlen(topic))) return MOSQ_ERR_MALFORMED_UTF8;

    if(properties){
        if(mosq->protocol != mosq_p_mqtt5){
            return MOSQ_ERR_NOT_SUPPORTED;
        }
        p = properties;
        while(p){
            /* 检查每个属性是否与正确的MQTT报文对应 */
            rc = mosquitto_property_check_command(CMD_WILL, p->identifier);
            if(rc) return rc;
            p = p->next;
        }
    }

    if(mosq->will){ /* 释放之前保存的遗嘱消息 */
        mosquitto__free(mosq->will->msg.topic);
        mosquitto__free(mosq->will->msg.payload);
        mosquitto_property_free_all(&mosq->will->properties);
        mosquitto__free(mosq->will);
    }

    mosq->will = mosquitto__calloc(1, sizeof(struct mosquitto_message_all));
    if(!mosq->will) return MOSQ_ERR_NOMEM;
    mosq->will->msg.topic = mosquitto__strdup(topic);
    if(!mosq->will->msg.topic){
        rc = MOSQ_ERR_NOMEM;
        goto cleanup;
    }
    mosq->will->msg.payloadlen = payloadlen;
    if(mosq->will->msg.payloadlen > 0){
        if(!payload){
            rc = MOSQ_ERR_INVAL;
            goto cleanup;
        }
        mosq->will->msg.payload = mosquitto__malloc(sizeof(char)*mosq->will->msg.payloadlen);
        if(!mosq->will->msg.payload){
            rc = MOSQ_ERR_NOMEM;
            goto cleanup;
        }

        memcpy(mosq->will->msg.payload, payload, payloadlen);
    }
    mosq->will->msg.qos = qos;
    mosq->will->msg.retain = retain;

    mosq->will->properties = properties;

    return MOSQ_ERR_SUCCESS;

cleanup:
    if(mosq->will){
        mosquitto__free(mosq->will->msg.topic);
        mosquitto__free(mosq->will->msg.payload);

        mosquitto__free(mosq->will);
        mosq->will = NULL;
    }

    return rc;
}

/*    清除对应客户端上下文中的遗嘱消息    */
int will__clear(struct mosquitto *mosq)
{
    if(!mosq->will) return MOSQ_ERR_SUCCESS;

    mosquitto__free(mosq->will->msg.topic);
    mosq->will->msg.topic = NULL;

    mosquitto__free(mosq->will->msg.payload);
    mosq->will->msg.payload = NULL;

    mosquitto_property_free_all(&mosq->will->properties);

    mosquitto__free(mosq->will);
    mosq->will = NULL;

    return MOSQ_ERR_SUCCESS;
}

