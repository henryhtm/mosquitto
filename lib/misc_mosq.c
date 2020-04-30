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

/* This contains general purpose utility functions that are not specific to
 * Mosquitto/MQTT features. */

#include "config.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>


FILE *mosquitto__fopen(const char *path, const char *mode, bool restrict_read)
{
    if (restrict_read) {
        FILE *fptr;
        mode_t old_mask;

        old_mask = umask(0077);
        fptr = fopen(path, mode);
        umask(old_mask);

        return fptr;
    }else{
        return fopen(path, mode);
    }
}


char *misc__trimblanks(char *str)
{
    char *endptr;

    if(str == NULL) return NULL;

    while(isspace(str[0])){
        str++;
    }
    endptr = &str[strlen(str)-1];
    while(endptr > str && isspace(endptr[0])){
        endptr[0] = '\0';
        endptr--;
    }
    return str;
}


char *fgets_extending(char **buf, int *buflen, FILE *stream)
{
    char *rc;
    char endchar;
    int offset = 0;
    char *newbuf;

    if(stream == NULL || buf == NULL || buflen == NULL || *buflen < 1){
        return NULL;
    }

    do{
        rc = fgets(&((*buf)[offset]), (*buflen)-offset, stream);
        if(feof(stream)){
            return rc;
        }

        endchar = (*buf)[strlen(*buf)-1];
        if(endchar == '\n'){
            return rc;
        }
        /* No EOL char found, so extend buffer */
        offset = (*buflen)-1;
        *buflen += 1000;
        newbuf = realloc(*buf, *buflen);
        if(!newbuf){
            return NULL;
        }
        *buf = newbuf;
    }while(1);
}
