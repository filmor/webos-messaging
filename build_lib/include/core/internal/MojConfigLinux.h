/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJCONFIGLINUX_H_
#define MOJCONFIGLINUX_H_

#define MOJ_USE_EPOLL
#define MOJ_USE_READDIR
#define MOJ_USE_MEMRCHR
#define MOJ_USE_GNU_STRERROR_R
#define MOJ_USE_RANDOM_R
#define MOJ_USE_SRANDOM_R

#define MOJ_NEED_ATOMIC_INIT
#define MOJ_NEED_ATOMIC_DESTROY
#define MOJ_NEED_ATOMIC_GET
#define MOJ_NEED_ATOMIC_SET

#include "core/internal/MojConfigUnix.h"

#endif /* MOJCONFIGLINUX_H_ */
