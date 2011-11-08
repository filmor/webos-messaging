/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJCONFIGMAC_H_
#define MOJCONFIGMAC_H_

#define MOJ_USE_READDIR_R
#define MOJ_USE_POSIX_STRERROR_R
#define MOJ_USE_RANDOM
#define MOJ_USE_SRANDOM

#define MOJ_NEED_ATOMIC_INIT
#define MOJ_NEED_ATOMIC_DESTROY
#define MOJ_NEED_ATOMIC_GET
#define MOJ_NEED_ATOMIC_SET

#define MOJ_NEED_MEMRCHR

#include "core/internal/MojConfigUnix.h"

#endif /* MOJCONFIGMAC_H_ */
