/*
Copyright (c) 2019-2020 Roger Light <roger@atchoo.org>

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

#include "mosquitto.h"
#include "alias_mosq.h"
#include "memory_mosq.h"

/* 将主题及其别名数值加入对应的上下文中 */
int alias__add(struct mosquitto *mosq, const char *topic, int alias)
{
    int i;
    struct mosquitto__alias *aliases;

    for(i=0; i<mosq->alias_count; i++){ 
        if(mosq->aliases[i].alias == alias){ /* 待增加的主题别名数值已经存在，替换之前的主题名称 */
            mosquitto__free(mosq->aliases[i].topic);
            mosq->aliases[i].topic = mosquitto__strdup(topic);
            if(mosq->aliases[i].topic){
                return MOSQ_ERR_SUCCESS;
            }else{
                return MOSQ_ERR_NOMEM;
            }
        }
    }

    /* 加入新的主题及其对应的别名数值 */
    aliases = mosquitto__realloc(mosq->aliases, sizeof(struct mosquitto__alias)*(mosq->alias_count+1));
    if(!aliases) return MOSQ_ERR_NOMEM;

    mosq->aliases = aliases;
    mosq->aliases[mosq->alias_count].alias = alias;
    mosq->aliases[mosq->alias_count].topic = mosquitto__strdup(topic);
    if(!mosq->aliases[mosq->alias_count].topic){
        return MOSQ_ERR_NOMEM;
    }
    mosq->alias_count++;

    return MOSQ_ERR_SUCCESS;
}

/* 查找指定别名数值对应的主题名称 */
int alias__find(struct mosquitto *mosq, char **topic, int alias)
{
    int i;

    for(i=0; i<mosq->alias_count; i++){
        if(mosq->aliases[i].alias == alias){
            *topic = mosquitto__strdup(mosq->aliases[i].topic);
            if(*topic){
                return MOSQ_ERR_SUCCESS;
            }else{
                return MOSQ_ERR_NOMEM;
            }
        }
    }
    return MOSQ_ERR_INVAL;
}

/* 清除对应上下文中所有的主题别名 */
void alias__free_all(struct mosquitto *mosq)
{
    int i;

    for(i=0; i<mosq->alias_count; i++){
        mosquitto__free(mosq->aliases[i].topic);
    }
    mosquitto__free(mosq->aliases);
    mosq->aliases = NULL;
    mosq->alias_count = 0;
}

