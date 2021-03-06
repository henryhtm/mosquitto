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

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include <dirent.h>

#include <strings.h>
#include <netdb.h>
#include <sys/socket.h>

#include <sys/syslog.h>

#include "mosquitto_broker_internal.h"
#include "memory_mosq.h"
#include "tls_mosq.h"
#include "util_mosq.h"
#include "mqtt_protocol.h"

/********************************************************************************* 
* FUNC: 比较两个字符串是否相同 
* Parameters:
*   p1      : [IN] 第1个待比较的字符串
*   p2      : [IN] 第2个待比较的字符串
* Return: 
*   [int] 两个字符串第一个不同字符的差值
*********************************************************************************/
int scmp_p(const void *p1, const void *p2)
{
    const char *s1 = *(const char **)p1;
    const char *s2 = *(const char **)p2;
    int result;

    while(s1[0] && s2[0]){
        /* Sort by case insensitive part first */
        result = toupper(s1[0]) - toupper(s2[0]);
        if(result == 0){
            /* Case insensitive part matched, now distinguish between case */
            result = s1[0] - s2[0];
            if(result != 0){
                return result;
            }
        }else{
            /* Return case insensitive match fail */
            return result;
        }
        s1++;
        s2++;
    }
    return s1[0] - s2[0];
}

/********************************************************************************* 
* FUNC: 获取指定目录中的文件列表 
* Parameters:
*   include_dir : [IN] 需要查找配置文件的目录路径
*   files       : [OUT] 配置文件的文件名列表（包含目录路径）
*   file_count  : [OUT] 配置文件的个数
* Return:
*   [int]   0   : 函数处理成功
*   [int] 其他    : 函数处理出错
*********************************************************************************/
int config__get_dir_files(const char *include_dir, char ***files, int *file_count)
{
    char **l_files = NULL;
    int l_file_count = 0;
    char **files_tmp;
    int len;
    int i;

    DIR *dh;
    struct dirent *de;

    dh = opendir(include_dir);
    if(!dh){
        log__printf(NULL, MOSQ_LOG_ERR, "Error: Unable to open include_dir '%s'.", include_dir);
        return 1;
    }
    while((de = readdir(dh)) != NULL){
        if(strlen(de->d_name) > 5){
            if(!strcmp(&de->d_name[strlen(de->d_name)-5], ".conf")){
                len = strlen(include_dir)+1+strlen(de->d_name)+1;

                l_file_count++;
                files_tmp = mosquitto__realloc(l_files, l_file_count*sizeof(char *));
                if(!files_tmp){
                    for(i=0; i<l_file_count-1; i++){
                        mosquitto__free(l_files[i]);
                    }
                    mosquitto__free(l_files);
                    closedir(dh);
                    return MOSQ_ERR_NOMEM;
                }
                l_files = files_tmp;

                l_files[l_file_count-1] = mosquitto__malloc(len+1);
                if(!l_files[l_file_count-1]){
                    for(i=0; i<l_file_count-1; i++){
                        mosquitto__free(l_files[i]);
                    }
                    mosquitto__free(l_files);
                    closedir(dh);
                    return MOSQ_ERR_NOMEM;
                }
                snprintf(l_files[l_file_count-1], len, "%s/%s", include_dir, de->d_name);
                l_files[l_file_count-1][len] = '\0';
            }
        }
    }
    closedir(dh);

    if(l_files){
        qsort(l_files, l_file_count, sizeof(char *), scmp_p);
    }
    *files = l_files;
    *file_count = l_file_count;

    return 0;
}


