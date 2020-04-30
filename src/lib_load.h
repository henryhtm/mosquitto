/*
Copyright (c) 2012-2020 Roger Light <roger@atchoo.org>

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

#ifndef LIB_LOAD_H
#define LIB_LOAD_H

#include <dlfcn.h>

#define LIB_LOAD(A) dlopen(A, RTLD_NOW|RTLD_GLOBAL)
#define LIB_CLOSE(A) dlclose(A)
#define LIB_SYM(HANDLE, SYM) dlsym(HANDLE, SYM)

#define LIB_SYM_EASY(MEMBER, HANDLE, SYM) if(!(MEMBER = LIB_SYM(HANDLE, SYM)) return 1
#endif
