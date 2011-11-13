/*
 * LibpurpleAdapter.h
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
 * The LibpurpleAdapter is a simple adapter between libpurple.so and the
 * IMLibpurpleService transport
 */

#ifndef LIBPURPLEADAPTER_H_
#define LIBPURPLEADAPTER_H_

#include <purple.h>
//#include <lunaservice.h> //TODO remove this once LS is removed from the adapter
#include <syslog.h>
#include "PalmImCommon.h"
#include "core/MojService.h"
#include "db/MojDbServiceClient.h"

#define CUSTOM_USER_DIRECTORY  "/var/preferences/org.webosinternals.messaging"
#define CUSTOM_PLUGIN_PATH     "/media/cryptofs/apps/usr/palm/applications/org.webosinternals.messaging/plugins"
#define PLUGIN_SAVE_PREF       "/purple/nullclient/plugins/saved"
#define UI_ID                  "adapter"

class LibpurplePrefsHandler
{
public:	
	void init();
	MojErr LoadAccountPreferences(const char* templateId, const char* UserName);
	
	MojErr GetServerPreferences(const char* templateId, const char* UserName, char*& ServerName, char*& ServerPort, bool& ServerTLS, bool& BadCert);
	virtual char* GetStringPreference(const char* Preference, const char* templateId, const char* UserName) = 0;
	virtual bool GetBoolPreference(const char* Preference, const char* templateId, const char* UserName) = 0;
	virtual void setaccountprefs(MojString templateId, PurpleAccount* account) = 0;
};

/*
 * The adapter callbacks interface
 */
class LoginCallbackInterface
{
public:
	typedef enum {
		LOGIN_SUCCESS, // successful login
		LOGIN_FAILED, //
		LOGIN_TIMEOUT, //
		LOGIN_SIGNED_OFF
	} LoginResult;

	virtual void loginResult(const char* serviceName, const char* username, LoginResult type, bool loggedOut, const char* errCode, bool noRetry) = 0;
	virtual void buddyListResult(const char* serviceName, const char* username, MojObject& buddyList, bool fullList) = 0;
};

/*
 * Incoming IM message callback
 */
class IMServiceCallbackInterface
{
public:
	typedef enum {
		RECEIVE_SUCCESS, // successful login
		RECEIVE_FAILED, //
	} ReceiveResult;

	virtual bool incomingIM(const char* serviceName, const char* username, const char* usernameFrom, const char* message) = 0;
	virtual bool updateBuddyStatus(const char* accountId, const char* serviceName, const char* username, int availability,
				const char* customMessage, const char* groupName, const char* buddyAvatarLoc) = 0;
	virtual bool receivedBuddyInvite(const char* serviceName, const char* username, const char* usernameFrom, const char* message) = 0;
	virtual bool buddyInviteDeclined(const char* serviceName, const char* username, const char* usernameFrom) = 0;
};

class LibpurpleAdapter
{	
public:
	typedef enum {
		OK,
		ALREADY_LOGGED_IN,
		FAILED,
		INVALID_CREDENTIALS
	} LoginResult;

	// return values for sending commands to libpurple
	typedef enum {
		SENT,
		USER_NOT_LOGGED_IN,
		INVALID_PARAMS,
		SEND_FAILED
	} SendResult;

	static void init();
	static void assignIMLoginState(LoginCallbackInterface* loginState);
	static void assignIMServiceHandler(IMServiceCallbackInterface* incomingIMHandler);
	static void assignPrefsHandler(LibpurplePrefsHandler* PrefsHandler);
	static LoginResult login(LoginParams* params, LoginCallbackInterface* loginState);
	// return false if already logged out
	static bool logout(const char* serviceName, const char* username, LoginCallbackInterface* loginState);
	static bool getFullBuddyList(const char* serviceName, const char* username);
	static bool setMyAvailability(const char* serviceName, const char* username, int availability);
	static bool setMyCustomMessage(const char* serviceName, const char* username, const char* customMessage);
	static SendResult blockBuddy(const char* serviceName, const char* username, const char* buddyUsername, bool block);
	static SendResult removeBuddy(const char* serviceName, const char* username, const char* buddyUsername);
	static SendResult addBuddy(const char* serviceName, const char* username, const char* buddyUsername, const char* groupname);
	static SendResult authorizeBuddy(const char* serviceName, const char* username, const char* buddyUsername);
	static SendResult declineBuddy(const char* serviceName, const char* username, const char* buddyUsername);
	static SendResult sendMessage(const char *serviceName, const char *username, const char *usernameTo, const char *messageText);
	static bool queuePresenceUpdates(bool enable);
	static bool deviceConnectionClosed(bool all, const char* ipAddress);
};

#endif /* LIBPURPLEADAPTER_H_ */
