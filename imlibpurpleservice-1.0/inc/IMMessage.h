/*
 * IMMessage.h
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
 * IMMessage class handles saving and retrieving an immessage object from the DB
 */

#ifndef INCOMINGIMMESSAGE_H_
#define INCOMINGIMMESSAGE_H_

#include "core/MojObject.h"

// Status is used to describe if the message is pending, has a failure or was successfully sent/received.
// Default is successful. When messages are moved to the outbox, we change the status to pending. The transports
// should set the status to successful or failed when the message is moved to the sent folder.
// The difference between failed and undeliverable is that the latter
// indicates that given another chance we know it will fail again because something is
// wrong with it
typedef enum {
	Successful = 0,
	Pending,
	Failed,
	Undeliverable,
	// used to indicate that the message can not be completed until a suitable data
	// connection can be made.
	WaitingForConnection,
} IMStatus;

typedef enum {
	Inbox = 0,
	Outbox,
	Drafts,
	System,
	Transient
} IMFolder;

// DB property names
#define MOJDB_FROM 					_T("from")
#define MOJDB_ADDRESS 				_T("addr")
#define MOJDB_FROM_ADDRESS 			_T("from.addr")
#define MOJDB_TO_ADDRESS 			_T("to.addr")
#define MOJDB_TO 					_T("to")
#define MOJDB_MSG_TEXT				_T("messageText")
#define MOJDB_DEVICE_TIMESTAMP		_T("localTimestamp")
#define MOJDB_SERVER_TIMESTAMP		_T("timestamp")
#define MOJDB_USERNAME				_T("username")
#define MOJDB_SERVICENAME           _T("serviceName") // type_gtalk, type_aim, etc
#define MOJDB_FOLDER				_T("folder")
#define MOJDB_STATUS       			_T("status")
#define MOJDB_ID					_T("_id")
#define MOJDB_IDS					_T("ids")
#define MOJDB_ERROR_CODE	        _T("errorCode")
#define MOJDB_ERROR_CATEGORY	    _T("errorCategory")

// libpurple transport property names
#define XPORT_SERVICE_TYPE          _T("serviceName") // gmail, aol etc
#define XPORT_FROM_ADDRESS 			_T("usernameFrom")
#define XPORT_TO_ADDRESS 			_T("usernameTo")
#define XPORT_USER 		        	_T("username")
#define XPORT_MSG_TEXT				_T("messageText")
#define XPORT_ERROR_TEXT	        _T("errorText")
#define XPORT_CAPABILITY_PROVIDERS  _T("capabilityProviders")

#define PALM_DB_IMMESSAGE_KIND 		"org.webosinternals.immessage.libpurple:1"

class IMMessage : public MojRefCounted {

public:
	// IM messages type
	static const char* MSG_TYPE_IM;
	static const char* statusStrings[];
	static const char* folderStrings[];
	static const char* trustedTags[];

	IMMessage();
	virtual ~IMMessage();

	MojErr initFromCallback(const char* serviceName, const char* username, const char* usernameFrom, const char* message);
	MojErr createDBObject(MojObject& returnObject);
	MojErr unformatFromAddress(const MojString formattedScreenName, MojString& unformattedName);

private:
	MojString msgText;
	MojString fromAddress;
	MojString toAddress;

	// only meaningful for incoming messages - this is the time the message was received on the device.
	MojInt64 deviceTimestamp; // millisec.
	// time message was received on server - incoming timestamp
	MojInt64 serverTimestamp; // millisec.

	// successful, pending, failed etc.
	IMStatus status;

	// inbox, outbox etc.
	IMFolder folder;

	// gmail, aol etc
    MojString msgType;

};

#endif /* INCOMINGIMMESSAGE_H_ */
