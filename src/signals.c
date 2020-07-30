/*
Copyright (c) 2016-2020 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
   Dmitry Kaukov - windows named events implementation.
*/

#include "config.h"

#include <unistd.h>
#include <grp.h>
#include <assert.h>

#include <pwd.h>
#include <sys/time.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#ifdef WITH_SYSTEMD
#include <systemd/sd-daemon.h>
#endif
#ifdef WITH_WRAP
#include <tcpd.h>
#endif
#ifdef WITH_WEBSOCKETS
#include <libwebsockets.h>
#endif

#include "mosquitto_broker_internal.h"
#include "memory_mosq.h"
#include "util_mosq.h"

extern bool flag_reload;
#ifdef WITH_PERSISTENCE
extern bool flag_db_backup;
#endif
extern bool flag_tree_print;
extern int run;

#ifdef SIGHUP
/* Signal handler for SIGHUP - flag a config reload. */
void handle_sighup(int signal)
{
    UNUSED(signal);

    flag_reload = true;
}
#endif

/* Signal handler for SIGINT and SIGTERM - just stop gracefully. */
void handle_sigint(int signal)
{
    UNUSED(signal);

    run = 0;
}

/* Signal handler for SIGUSR1(eg. "kill -USR1 pid") - backup the db. */
void handle_sigusr1(int signal)
{
    UNUSED(signal);

#ifdef WITH_PERSISTENCE
    flag_db_backup = true;
#endif
}

/* Signal handler for SIGUSR2(eg. "kill -USR2 pid") - print subscription / retained tree. */
void handle_sigusr2(int signal)
{
    UNUSED(signal);

    flag_tree_print = true;
}


