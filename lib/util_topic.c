/*
Copyright (c) 2009-2020 Roger Light <roger@atchoo.org>

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

#include <assert.h>
#include <string.h>

#include <sys/stat.h>


#ifdef WITH_BROKER
#include "mosquitto_broker_internal.h"
#endif

#include "mosquitto.h"
#include "memory_mosq.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "time_mosq.h"
#include "tls_mosq.h"
#include "util_mosq.h"

/* Check that a topic used for publishing is valid.
   Search for + or # in a topic. Return MOSQ_ERR_INVAL if found.
   Also returns MOSQ_ERR_INVAL if the topic string is too long.
   Returns MOSQ_ERR_SUCCESS if everything is fine.
 * 检查某个发布的主题是否有效：
   如果在主题中找到+或#字符，则返回MOSQ_ERR_INVAL；
   如果主题过长（长度大于65535字节），也返回MOSQ_ERR_INVAL；
   一切正常时返回成功。    */
int mosquitto_pub_topic_check(const char *str)
{
    int len = 0;
#ifdef WITH_BROKER
    int hier_count = 0;
#endif
    while(str && str[0]){
        if(str[0] == '+' || str[0] == '#'){
            return MOSQ_ERR_INVAL;
        }
#ifdef WITH_BROKER
        else if(str[0] == '/'){
            hier_count++;
        }
#endif
        len++;
        str = &str[1];
    }
    if(len > 65535) return MOSQ_ERR_INVAL;
#ifdef WITH_BROKER
    if(hier_count > TOPIC_HIERARCHY_LIMIT) return MOSQ_ERR_INVAL;
#endif

    return MOSQ_ERR_SUCCESS;
}

/* 函数作用同上，通过参数len限制了字符串长度 */
int mosquitto_pub_topic_check2(const char *str, size_t len)
{
    size_t i;
#ifdef WITH_BROKER
    int hier_count = 0;
#endif

    if(len > 65535) return MOSQ_ERR_INVAL;

    for(i=0; i<len; i++){
        if(str[i] == '+' || str[i] == '#'){
            return MOSQ_ERR_INVAL;
        }
#ifdef WITH_BROKER
        else if(str[i] == '/'){
            hier_count++;
        }
#endif
    }
#ifdef WITH_BROKER
    if(hier_count > TOPIC_HIERARCHY_LIMIT) return MOSQ_ERR_INVAL;
#endif

    return MOSQ_ERR_SUCCESS;
}

/* Check that a topic used for subscriptions is valid.
   Search for + or # in a topic, check they aren't in invalid positions such as
   foo/#/bar, foo/+bar or foo/bar#.
   Return MOSQ_ERR_INVAL if invalid position found.
   Also returns MOSQ_ERR_INVAL if the topic string is too long.
   Returns MOSQ_ERR_SUCCESS if everything is fine.
 * 检查某个订阅的主题是否有效：
   检查主题中的+和#字符是否在非法的位置，如：foo/#/bar, foo/+bar or foo/bar#，
   如果在非法位置则返回MOSQ_ERR_INVAL；
   如果主题过长（长度大于65535字节），也返回MOSQ_ERR_INVAL；
   一切正常时返回成功。    */
int mosquitto_sub_topic_check(const char *str)
{
    char c = '\0';
    int len = 0;
#ifdef WITH_BROKER
    int hier_count = 0;
#endif

    while(str && str[0]){
        if(str[0] == '+'){
            if((c != '\0' && c != '/') || (str[1] != '\0' && str[1] != '/')){
                return MOSQ_ERR_INVAL;
            }
        }else if(str[0] == '#'){
            if((c != '\0' && c != '/')  || str[1] != '\0'){
                return MOSQ_ERR_INVAL;
            }
        }
#ifdef WITH_BROKER
        else if(str[0] == '/'){
            hier_count++;
        }
#endif
        len++;
        c = str[0];
        str = &str[1];
    }
    if(len > 65535) return MOSQ_ERR_INVAL;
#ifdef WITH_BROKER
    if(hier_count > TOPIC_HIERARCHY_LIMIT) return MOSQ_ERR_INVAL;
#endif

    return MOSQ_ERR_SUCCESS;
}

/* 函数作用同上，通过参数len限制了字符串长度 */
int mosquitto_sub_topic_check2(const char *str, size_t len)
{
    char c = '\0';
    size_t i;
#ifdef WITH_BROKER
    int hier_count = 0;
#endif

    if(len > 65535) return MOSQ_ERR_INVAL;

    for(i=0; i<len; i++){
        if(str[i] == '+'){
            if((c != '\0' && c != '/') || (i<len-1 && str[i+1] != '/')){
                return MOSQ_ERR_INVAL;
            }
        }else if(str[i] == '#'){
            if((c != '\0' && c != '/')  || i<len-1){
                return MOSQ_ERR_INVAL;
            }
        }
#ifdef WITH_BROKER
        else if(str[i] == '/'){
            hier_count++;
        }
#endif
        c = str[i];
    }
#ifdef WITH_BROKER
    if(hier_count > TOPIC_HIERARCHY_LIMIT) return MOSQ_ERR_INVAL;
#endif

    return MOSQ_ERR_SUCCESS;
}

/* Does a topic match a subscription? 
   检查指定主题和订阅是否匹配 */
int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result)
{
    return mosquitto_topic_matches_sub2(sub, 0, topic, 0, result);
}

/* Does a topic match a subscription? 
   检查指定主题和订阅是否匹配 */
int mosquitto_topic_matches_sub2(const char *sub, size_t sublen, const char *topic, size_t topiclen, bool *result)
{
    size_t spos;

    UNUSED(sublen);
    UNUSED(topiclen);

    if(!result) return MOSQ_ERR_INVAL;
    *result = false;

    if(!sub || !topic || sub[0] == 0 || topic[0] == 0){
        return MOSQ_ERR_INVAL;
    }

    if((sub[0] == '$' && topic[0] != '$')
            || (topic[0] == '$' && sub[0] != '$')){
        return MOSQ_ERR_SUCCESS;
    }

    spos = 0;

    while(sub[0] != 0){
        /* 异常场景：主题中不允许存在通配符 */
        if(topic[0] == '+' || topic[0] == '#'){
            return MOSQ_ERR_INVAL;
        }
        /* 当前订阅字符和主题字符不相同，检查订阅字符是否是通配符 */
        /* HEQ: 下面的条件语句中“|| topic[0] == 0”是否多余？ */
        if(sub[0] != topic[0] || topic[0] == 0){
            if(sub[0] == '+'){
                /* 异常场景：当前订阅字符是"+"，但上一个订阅字符不是"/"，如："test+"，"test/he+" */
                if(spos > 0 && sub[-1] != '/'){
                    return MOSQ_ERR_INVAL;
                }
                /* 异常场景：当前订阅字符是"+"，但下一个订阅字符不是"/"，如："+test"，"/test/+he" */
                if(sub[1] != 0 && sub[1] != '/'){
                    return MOSQ_ERR_INVAL;
                }
                spos++;
                sub++;
                /* 一个"+"符号匹配主题中的一个层级 */
                while(topic[0] != 0 && topic[0] != '/'){
                    /* 异常场景：主题中不允许存在通配符 */
                    if(topic[0] == '+' || topic[0] == '#'){
                        return MOSQ_ERR_INVAL;
                    }
                    topic++;
                }
                /* 匹配成功 */
                if(topic[0] == 0 && sub[0] == 0){
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }
            }else if(sub[0] == '#'){
                /* 异常场景：当前订阅字符是"#"，但上一个订阅字符不是"/"，如："test#"，"test/he#" */
                if(spos > 0 && sub[-1] != '/'){
                    return MOSQ_ERR_INVAL;
                }
                /* 异常场景："#"字符不是订阅中的最后一个字符，如："test/#foo" */
                if(sub[1] != 0){
                    return MOSQ_ERR_INVAL;
                }else{
                    /* 订阅中的"#"字符匹配主题中剩下的所有内容 */
                    while(topic[0] != 0){
                        /* 异常场景：主题中不允许存在通配符 */
                        if(topic[0] == '+' || topic[0] == '#'){
                            return MOSQ_ERR_INVAL;
                        }
                        topic++;
                    }
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }
            }else{ /* 订阅中的当前字符不是“+”和“#” */
                /* 特殊场景：主题字符串结束但订阅中的当前字符是"/"，并且上一个字符是"+"，下一个字符是"#"，
                   如："test/+/#" ，这种情况下和主题"test/go"可以匹配成功 */
                if(topic[0] == 0 && spos > 0
                   && sub[-1] == '+' && sub[0] == '/' && sub[1] == '#')
                {
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }

                /* 订阅与主题不匹配，检查订阅中的每个字符来确认订阅是否有效 */
                while(sub[0] != 0){
                    if(sub[0] == '#' && sub[1] != 0){
                        return MOSQ_ERR_INVAL;
                    }
                    spos++;
                    sub++;
                }

                /* 订阅的主题有效但匹配不成功 */
                return MOSQ_ERR_SUCCESS;
            }
        }else{ /* 当前订阅字符和主题字符相同 */
            if(topic[1] == 0){
                /* 特殊场景：主题字符串结束但订阅字符串多包含"/#"两个字符。
                   例如： 主题"test"和订阅"test/#"匹配    */
                if(sub[1] == '/'
                        && sub[2] == '#'
                        && sub[3] == 0){
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }
            }
            spos++;
            sub++;
            topic++;
            if(sub[0] == 0 && topic[0] == 0){
                *result = true;
                return MOSQ_ERR_SUCCESS;
            }else if(topic[0] == 0 && sub[0] == '+' && sub[1] == 0){ /* 主题字符串已经结束，订阅字符串最后字符为"+" */
                if(spos > 0 && sub[-1] != '/'){ /* 异常场景：订阅中的格式无效，如："test+" */
                    return MOSQ_ERR_INVAL;
                }
                spos++;
                sub++;
                *result = true;
                return MOSQ_ERR_SUCCESS;
            }
        }
    }    /* 订阅字符串结束 */
    
    if((topic[0] != 0 || sub[0] != 0)){
        *result = false;
    }
    while(topic[0] != 0){
        /* 异常场景：主题中不允许存在通配符 */
        if(topic[0] == '+' || topic[0] == '#'){
            return MOSQ_ERR_INVAL;
        }
        topic++;
    }
    return MOSQ_ERR_SUCCESS;
}
