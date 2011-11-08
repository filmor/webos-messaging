/* 
 * (c) Copyright 1995, 1997-1999 Hewlett-Packard Development Company, L.P. 
 * 
 * This file is dual-licensed, as follows: 
 * 
 * (1) 
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. 
 * 
 * (2) 
 * IN ADDITION, HP/Palm may make this file available under the terms of HP/Palm proprietary software licenses. 
 * 
 */

#ifndef _LUNASERVICE_ERRORS_H_
#define _LUNASERVICE_ERRORS_H_

/**
 * @defgroup LunaServiceErrorCodes LunaServiceErrorCodes
 * @brief Error codes for LSError
 * @ingroup LunaServiceError
 * @{
 */
#define _LS_ERROR_CODE_OFFSET           1024    /**< Try to avoid colliding with errno.h. See _LSErrorSetFromErrno() */

#define LS_ERROR_CODE_UNKNOWN_ERROR     (-1 - _LS_ERROR_CODE_OFFSET)    /**< unknown */
#define LS_ERROR_CODE_OOM               (-2 - _LS_ERROR_CODE_OFFSET)    /**< out of memory */
#define LS_ERROR_CODE_PERMISSION        (-3 - _LS_ERROR_CODE_OFFSET)    /**< permissions */
#define LS_ERROR_CODE_DUPLICATE_NAME    (-4 - _LS_ERROR_CODE_OFFSET)    /**< duplicate name */
#define LS_ERROR_CODE_CONNECT_FAILURE   (-5 - _LS_ERROR_CODE_OFFSET)    /**< connection failure */
#define LS_ERROR_CODE_DEPRECATED        (-6 - _LS_ERROR_CODE_OFFSET)    /**< API is deprecated */
#define LS_ERROR_CODE_NOT_PRIVILEGED    (-7 - _LS_ERROR_CODE_OFFSET)    /**< service is not privileged */

/** @} LunaServiceErrorCodes */

/** Lunabus service name */
#define LUNABUS_SERVICE_NAME        "com.palm.bus"

#define LUNABUS_SERVICE_NAME_OLD    "com.palm.lunabus"

/** Category for lunabus signal addmatch */
#define LUNABUS_SIGNAL_CATEGORY "/com/palm/bus/signal"

#define LUNABUS_SIGNAL_REGISTERED "registered"
#define LUNABUS_SIGNAL_SERVERSTATUS "ServerStatus"

/** Category for lunabus errors */
#define LUNABUS_ERROR_CATEGORY "/com/palm/bus/error"

/***
 * Error Method names
 */

/** Sent to callback when method is not handled by service. */
#define LUNABUS_ERROR_UNKNOWN_METHOD "UnknownMethod"

/** Sent to callback when service is down. */
#define LUNABUS_ERROR_SERVICE_DOWN "ServiceDown"

/** Sent to callback when permissions restrict the call from being made */
#define LUNABUS_ERROR_PERMISSION_DENIED "PermissionDenied"

/** Sent to callback when service does not exist (not in service file) */
#define LUNABUS_ERROR_SERVICE_NOT_EXIST "ServiceNotExist"

/** Out of memory */
#define LUNABUS_ERROR_OOM "OutOfMemory"

/**
 * UnknownError is usually as:
 * 'UnknownError (some dbus error name we don't handle yet)'
 */
#define LUNABUS_ERROR_UNKNOWN_ERROR "UnknownError"

#endif  /* _LUNASERVICE_ERRORS_H_ */
