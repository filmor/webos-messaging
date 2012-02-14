/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJCONFIG_H_
#define MOJCONFIG_H_

// arch
#if !(defined(MOJ_ARM) || defined(MOJ_X86))
#	if (__arm__)
#		define MOJ_ARM
#	elif (__i386__ || __x86_64__)
#		define MOJ_X86
#	endif
#endif

#if (defined(MOJ_X86) && defined(MOJ_ARM))
#   error Invalid arch!
#endif

// byte order
#if (defined(MOJ_X86) || defined(MOJ_ARM))
#	define MOJ_LITTLE_ENDIAN
#endif

// memory alignment
#ifdef MOJ_X86
#	define MOJ_UNALIGNED_MEM_ACCESS
#endif

#ifdef MOJ_LINUX
#	include "core/internal/MojConfigLinux.h"
#elif MOJ_MAC
#	include "core/internal/MojConfigMac.h"
#elif MOJ_WIN
#	include "core/internal/MojConfigWin.h"
#endif

#endif /* MOJCONFIG_H_ */
