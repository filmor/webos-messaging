/*
 * IMDefines.h
 *
 * Copyright 2010 Palm, Inc. All rights reserved.
 *
 * This program is free software and licensed under the terms of the GNU
 * General Public License Version 2 as published by the Free
 * Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-
 * 1301, USA
 *
 * IMLibpurpleservice uses libpurple.so to implement a fully functional IM
 * Transport service for use on a mobile device.
 *
 * Database kind definitions
 */

#ifndef IM_DEFINES_H_
#define IM_DEFINES_H_

/*
 * DB Kinds
 */
#define IM_LOGINSTATE_KIND "com.palm.imloginstate.libpurple:1"
#define IM_IMMESSAGE_KIND "com.palm.immessage.libpurple:1"
#define IM_IMCOMMAND_KIND "com.palm.imcommand.libpurple:1"
#define IM_CONTACT_KIND "com.palm.contact.libpurple:1"

/*
 * temp db kinds
 */
#define IM_BUDDYSTATUS_KIND "com.palm.imbuddystatus.libpurple:1"
#define IM_SYNCSTATE_KIND "com.palm.account.syncstate:1"


#endif /* IM_DEFINES_H_ */
