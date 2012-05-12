/*
 * LibpurpleAdapter.cpp
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

/*
 * This file includes code from pidgin  (nullclient.c)
 *
 * pidgin
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */

#include "purple.h"

#include <glib.h>

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "LibpurpleAdapter.h"
#include "PalmImCommon.h"

//#include <cjson/json.h>
//#include <lunaservice.h>
//#include <json_utils.h>
#include "IMServiceApp.h"

#include <unordered_map>
#include <vector>
#include "Util.hpp"

static const guint PURPLE_GLIB_READ_COND  = (G_IO_IN | G_IO_HUP | G_IO_ERR);
static const guint PURPLE_GLIB_WRITE_COND = (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL);
static const guint CONNECT_TIMEOUT_SECONDS = 45;

static LoginCallbackInterface* s_loginState = NULL;
static IMServiceCallbackInterface* s_imServiceHandler = NULL;

std::hash<std::string> hash;

/**
 * List of accounts that are online
 */
static std::unordered_map<std::string, PurpleAccount*> s_onlineAccountData;
/**
 * List of accounts that are in the process of logging in
 */
static std::unordered_map<std::string, PurpleAccount*> s_pendingAccountData;
static std::unordered_map<std::string, PurpleAccount*> s_offlineAccountData;
static std::unordered_map<std::string, guint> s_accountLoginTimers;
static std::unordered_map<std::string, std::string> s_connectionTypeData;
static std::unordered_map<std::string, std::string> s_AccountIdsData;

/*
 * list of pending authorization requests
 */
static GHashTable* s_AuthorizeRequests = NULL;

static bool s_libpurpleInitialized = FALSE;
static bool s_registeredForAccountSignals = FALSE;
static bool s_registeredForPresenceUpdateSignals = FALSE;
/** 
 * Keeps track of the local IP address that we bound to when logging in to individual accounts 
 * key: accountKey, value: IP address 
 */
static std::unordered_map<std::string, std::string> s_ipAddressesBoundTo;

typedef struct _IOClosure
{
	guint result;
	gpointer data;
	PurpleInputFunction function;
} IOClosure;

// used for processing buddy invite requests
typedef struct _auth_and_add
{
	PurpleAccountRequestAuthorizationCb auth_cb;
	PurpleAccountRequestAuthorizationCb deny_cb;
	void *data;
	char *remote_user;
	char *alias;
	PurpleAccount *account;
} AuthRequest;

struct AccountMetaData
{
	const char* account_key;
	const char* servicename;
};

static void incoming_message_cb(PurpleConversation *conv, const char *who, const char *alias, const char *message,	PurpleMessageFlags flags, time_t mtime);
static void adapterUIInit(void);
static GHashTable* getClientInfo(void);
static gboolean adapterInvokeIO(GIOChannel *source, GIOCondition condition, gpointer data);
static guint adapterIOAdd(gint fd, PurpleInputCondition condition, PurpleInputFunction function, gpointer data);

//Prompt for authorization when someone adds this account to their buddy list.
//To authorize them to see this account's presence, call authorize_cb (user_data); otherwise call deny_cb (user_data);
//Returns:
//    a UI-specific handle, as passed to close_account_request.
//    void(* _PurpleAccountUiOps::close_account_request)(void *ui_handle)
//    Close a pending request for authorization.
//    ui_handle is a handle as returned by request_authorize.
//Parameters:
//    	account 	The account that was added
//    	remote_user 	The name of the user that added this account.
//    	id 	The optional ID of the local account. Rarely used.
//    	alias 	The optional alias of the remote user.
//    	message 	The optional message sent by the user wanting to add you.
//    	on_list 	Is the remote user already on the buddy list?
//    	auth_cb 	The callback called when the local user accepts
//    	deny_cb 	The callback called when the local user rejects
//    	user_data 	Data to be passed back to the above callbacks
static void *request_authorize_cb (PurpleAccount *account,
	const char *remote_user,
	const char *id,
	const char *alias,
	const char *message,
	gboolean on_list,
	PurpleAccountRequestAuthorizationCb authorize_cb,
	PurpleAccountRequestAuthorizationCb deny_cb,
	void *user_data);

void request_add_cb (PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message);

// AccountUIOps:
//    /** A buddy who is already on this account's buddy list added this account
//-	   *  to their buddy list.
//     */
//-	void (*notify_added)(PurpleAccount *account,
//-	                     const char *remote_user,
//-	                     const char *id,
//-	                     const char *alias,
//-	                     const char *message);
//-
//-	/** This account's status changed. */
//-	void (*status_changed)(PurpleAccount *account,
//-	                       PurpleStatus *status);
//-
//-	/** Someone we don't have on our list added us; prompt to add them. */
//-	void (*request_add)(PurpleAccount *account,
//-	                    const char *remote_user,
//-	                    const char *id,
//-	                    const char *alias,
//-	                    const char *message);
//-
//-	/** Prompt for authorization when someone adds this account to their buddy
//-	 * list.  To authorize them to see this account's presence, call \a
//-	 * authorize_cb (\a user_data); otherwise call \a deny_cb (\a user_data);
//-	 * @return a UI-specific handle, as passed to #close_account_request.
//-	 */
//-	void *(*request_authorize)(PurpleAccount *account,
//-	                           const char *remote_user,
//-	                           const char *id,
//-	                           const char *alias,
//-	                           const char *message,
//-	                           gboolean on_list,
//-	                           PurpleAccountRequestAuthorizationCb authorize_cb,
//-	                           PurpleAccountRequestAuthorizationCb deny_cb,
//-	                           void *user_data);
//-
//-	/** Close a pending request for authorization.  \a ui_handle is a handle
//-	 *  as returned by #request_authorize.
//-	 */
//-	void (*close_account_request)(void *ui_handle);
static PurpleAccountUiOps adapterAccountUIOps =
{
	NULL,                  //  notify_added
	NULL,                  //  status_changed
	request_add_cb,        //  request_add
	request_authorize_cb,  //  request_authorize
	NULL,                  //  close_account_request,

	// padding
	NULL,
	NULL,
	NULL,
	NULL,
};

static PurpleCoreUiOps adapterCoreUIOps =
{
	NULL, NULL, adapterUIInit, NULL, getClientInfo, NULL, NULL, NULL
};

static PurpleEventLoopUiOps adapterEventLoopUIOps =
{
	g_timeout_add, g_source_remove, adapterIOAdd, g_source_remove, NULL, g_timeout_add_seconds, NULL, NULL, NULL
};

static PurpleConversationUiOps adapterConversationUIOps  =
{
	NULL, NULL, NULL, NULL, incoming_message_cb, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL
};

// useful for debugging
static void authRequest_log_func(gpointer key, gpointer value, gpointer ud)
{
	AuthRequest *aa = (AuthRequest *)value;
	MojLogInfo(IMServiceApp::s_log, _T("  AuthRequest: key = %s, requester = %s"), (char*)key, aa->remote_user);
}
static void logAuthRequestTableValues()
{
	MojLogInfo(IMServiceApp::s_log, _T("Authorize Request Table:"));
	g_hash_table_foreach(s_AuthorizeRequests, authRequest_log_func, NULL);
}


void adapterUIInit(void)
{
	purple_conversations_set_ui_ops(&adapterConversationUIOps);
	purple_accounts_set_ui_ops(&adapterAccountUIOps);
}

void destroyNotify(gpointer dataToFree)
{
	g_free(dataToFree);
}

gboolean adapterInvokeIO(GIOChannel* ioChannel, GIOCondition ioCondition, gpointer data)
{
	IOClosure* ioClosure = (IOClosure*)data;
	int purpleCondition = 0;
	
	if (PURPLE_GLIB_READ_COND & ioCondition)
	{
		purpleCondition = purpleCondition | PURPLE_INPUT_READ;
	}
	
	if (PURPLE_GLIB_WRITE_COND & ioCondition)
	{
		purpleCondition = purpleCondition | PURPLE_INPUT_WRITE;
	}

	ioClosure->function(ioClosure->data, g_io_channel_unix_get_fd(ioChannel), (PurpleInputCondition)purpleCondition);

	return TRUE;
}

guint adapterIOAdd(gint fd, PurpleInputCondition purpleCondition, PurpleInputFunction inputFunction, gpointer data)
{
	GIOChannel* ioChannel;
	unsigned int ioCondition = 0;
	IOClosure* ioClosure = g_new0(IOClosure, 1);

	ioClosure->data = data;
	ioClosure->function = inputFunction;

	if (PURPLE_INPUT_READ & purpleCondition)
	{
		ioCondition = ioCondition | PURPLE_GLIB_READ_COND;
	}
	
	if (PURPLE_INPUT_WRITE & purpleCondition)
	{
		ioCondition = ioCondition | PURPLE_GLIB_WRITE_COND;
	}

	ioChannel = g_io_channel_unix_new(fd);
	ioClosure->result = g_io_add_watch_full(ioChannel, G_PRIORITY_DEFAULT, (GIOCondition)ioCondition, adapterInvokeIO, ioClosure,
			destroyNotify);

	g_io_channel_unref(ioChannel);
	return ioClosure->result;
}

/*
 * Helper methods 
 */

static const char* getMojoFriendlyErrorCode(PurpleConnectionError type)
{
	const char* mojoFriendlyErrorCode;
	if (type == PURPLE_CONNECTION_ERROR_INVALID_USERNAME)
	{
		mojoFriendlyErrorCode = ERROR_BAD_USERNAME;
	}
	else if (type == PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED)
	{
		mojoFriendlyErrorCode = ERROR_AUTHENTICATION_FAILED;
	}
	else if (type == PURPLE_CONNECTION_ERROR_NETWORK_ERROR)
	{
		mojoFriendlyErrorCode = ERROR_NETWORK_ERROR;
	}
	else if (type == PURPLE_CONNECTION_ERROR_NAME_IN_USE)
	{
		mojoFriendlyErrorCode = ERROR_NAME_IN_USE;
	}
	else
	{
		MojLogInfo(IMServiceApp::s_log, _T("PurpleConnectionError was %i"), type);
		mojoFriendlyErrorCode = ERROR_GENERIC_ERROR;
	}
	return mojoFriendlyErrorCode;
}

static std::string getAccountKey(std::string const& username, std::string const& serviceName)
{
	return username + "_" + serviceName;
}

static char* getAuthRequestKey(const char* username, const char* serviceName, const char* remoteUsername)
{
	if (!username)
	{
		MojLogError(IMServiceApp::s_log, _T("getAuthRequestKey - empty username"));
		return strdup("");
	}
	if (!serviceName)
	{
		MojLogError(IMServiceApp::s_log, _T("getAuthRequestKey - empty serviceName"));
		return strdup("");
	}
	if (!remoteUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("getAuthRequestKey - empty remoteUsername"));
		return strdup("");
	}
	char *authRequestKey = NULL;
	// asprintf allocates appropriate-sized buffer
	asprintf(&authRequestKey, "%s_%s_%s", username, serviceName, remoteUsername);
	MojLogInfo(IMServiceApp::s_log, _T("getAuthRequestKey - username: %s, serviceName: %s, remoteUser: %s key: %s"), username, serviceName, remoteUsername, authRequestKey);
	return authRequestKey;
}

static const char* getAccountKeyFromPurpleAccount(PurpleAccount* account)
{
	if (!account || !account->ui_data)
	{
		MojLogError(IMServiceApp::s_log, _T("getAccountKeyFromPurpleAccount called with empty account"));
		return "";
	}
	return ((AccountMetaData*)account->ui_data)->account_key;
}

static const char* getServiceNameFromPurpleAccount(PurpleAccount* account)
{
	if (!account || !account->ui_data)
		return "";

	return ((AccountMetaData*)account->ui_data)->servicename;
}

/**
 * Returns a GString containing the special stanza to enable server-side presence update queue
 * Clean up after yourself using g_string_free when you're done with the return value
 */
static GString* getEnableQueueStanza(PurpleAccount* account)
{
	GString* stanza = NULL;
	if (account != NULL)
	{
		if (strcmp(account->protocol_id, "prpl-jabber") == 0)
		{
			stanza = g_string_new("");
			PurpleConnection* pc = purple_account_get_connection(account);
			if (pc == NULL)
			{
				return NULL;
			}
			const char* displayName = purple_connection_get_display_name(pc);
			if (displayName == NULL)
			{
				return NULL;
			}
			g_string_append(stanza, "<iq from='");
			g_string_append(stanza, displayName);
			g_string_append(stanza, "' type='set'><query xmlns='google:queue'><enable/></query></iq>");
		}
		else if (strcmp(account->protocol_id, "prpl-aim") == 0)
		{
			MojLogInfo(IMServiceApp::s_log, _T("getEnableQueueStanza for AIM"));
			stanza = g_string_new("true");
		}
	}
	return stanza;
}

/**
 * Returns a GString containing the special stanza to disable and flush the server-side presence update queue
 * Clean up after yourself using g_string_free when you're done with the return value
 */
static GString* getDisableQueueStanza(PurpleAccount* account)
{
	GString* stanza = NULL;
	if (account != NULL)
	{
		if (strcmp(account->protocol_id, "prpl-jabber") == 0)
		{
			stanza = g_string_new("");
			PurpleConnection* pc = purple_account_get_connection(account);
			if (pc == NULL)
			{
				return NULL;
			}
			const char* displayName = purple_connection_get_display_name(pc);
			if (displayName == NULL)
			{
				return NULL;
			}
			g_string_append(stanza, "<iq from='");
			g_string_append(stanza, displayName);
			g_string_append(stanza, "' type='set'><query xmlns='google:queue'><disable/><flush/></query></iq>");
		}
		else if (strcmp(account->protocol_id, "prpl-aim") == 0)
		{
			MojLogInfo(IMServiceApp::s_log, _T("getDisableQueueStanza for AIM"));
			stanza = g_string_new("false");
		}
	}
	return stanza;
}

static void enableServerQueueForAccount(PurpleAccount* account)
{
	if (!account)
	{
		MojLogError(IMServiceApp::s_log, _T("enableServerQueueForAccount called with empty account"));
		return;
	}

	PurplePluginProtocolInfo* prpl_info = NULL;
	PurpleConnection* gc = purple_account_get_connection(account);
	PurplePlugin* prpl = NULL;
	
	if (gc != NULL)
	{
		prpl = purple_connection_get_prpl(gc);
	}

	if (prpl != NULL)
	{
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);
	}

	if (prpl_info && prpl_info->send_raw)
	{
		GString* enableQueueStanza = getEnableQueueStanza(account);
		if (enableQueueStanza != NULL) 
		{
			MojLogInfo(IMServiceApp::s_log, _T("Enabling server queue for %s"), account->protocol_id);
			prpl_info->send_raw(gc, enableQueueStanza->str, enableQueueStanza->len);
			g_string_free(enableQueueStanza, TRUE);
		}
	}
}

static void disableServerQueueForAccount(PurpleAccount* account)
{
	if (!account)
	{
		MojLogError(IMServiceApp::s_log, _T("disableServerQueueForAccount called with empty account"));
		return;
	}
	PurplePluginProtocolInfo* prpl_info = NULL;
	PurpleConnection* gc = purple_account_get_connection(account);
	PurplePlugin* prpl = NULL;
	
	if (gc != NULL)
	{
		prpl = purple_connection_get_prpl(gc);
	}

	if (prpl != NULL)
	{
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);
	}

	if (prpl_info && prpl_info->send_raw)
	{
		GString* disableQueueStanza = getDisableQueueStanza(account);
		if (disableQueueStanza != NULL) 
		{
			MojLogInfo(IMServiceApp::s_log, _T("Disabling server queue"));
			prpl_info->send_raw(gc, disableQueueStanza->str, disableQueueStanza->len);
			g_string_free(disableQueueStanza, TRUE);
		}
	}
}

/**
 * Asking the gtalk server to enable/disable queueing of presence updates 
 * This is called when the screen is turned off (enable:true) or turned on (enable:false)
 */
bool LibpurpleAdapter::queuePresenceUpdates(bool enable)
{
	typedef std::unordered_map<std::string, PurpleAccount*>::const_iterator
		iter_type;
	
	for (iter_type i = s_onlineAccountData.begin(); i != s_onlineAccountData.end(); ++i)
	{
		PurpleAccount* account = i->second;
		if (account)
		{
			/*
			 * enabling/disabling server queue is supported by gtalk (jabber) or aim
			 */
 			if ((strcmp(account->protocol_id, "prpl-jabber") == 0) ||
			    (strcmp(account->protocol_id, "prpl-aim") == 0))
 			{
				if (enable)
				{
					enableServerQueueForAccount(account);
				}
				else
				{
					disableServerQueueForAccount(account);
				}
 			}
		}
	}
	return TRUE;
}


static int getPalmAvailabilityFromPurpleAvailability(int prplAvailability)
{
	switch (prplAvailability)
	{
	case PURPLE_STATUS_UNSET:
		return PalmAvailability::NO_PRESENCE;
	case PURPLE_STATUS_OFFLINE:
		return PalmAvailability::OFFLINE;
	case PURPLE_STATUS_AVAILABLE:
		return PalmAvailability::ONLINE;
	case PURPLE_STATUS_UNAVAILABLE:
		return PalmAvailability::IDLE;
	case PURPLE_STATUS_INVISIBLE:
		return PalmAvailability::INVISIBLE;
	case PURPLE_STATUS_AWAY:
		return PalmAvailability::IDLE;
	case PURPLE_STATUS_EXTENDED_AWAY:
		return PalmAvailability::IDLE;
	case PURPLE_STATUS_MOBILE:
		return PalmAvailability::MOBILE;
	case PURPLE_STATUS_TUNE:
		return PalmAvailability::ONLINE;
	default:
		return PalmAvailability::OFFLINE;
	}
}


static PurpleStatusPrimitive getPurpleAvailabilityFromPalmAvailability(int palmAvailability)
{
	switch (palmAvailability)
	{
	case PalmAvailability::ONLINE:
		return PURPLE_STATUS_AVAILABLE;
	case PalmAvailability::MOBILE:
		return PURPLE_STATUS_MOBILE;
	case PalmAvailability::IDLE:
		return PURPLE_STATUS_AWAY;
	case PalmAvailability::INVISIBLE:
		return PURPLE_STATUS_INVISIBLE;
	case PalmAvailability::OFFLINE:
		return PURPLE_STATUS_OFFLINE;
	default:
		return PURPLE_STATUS_OFFLINE;
	}
}

/*
 * End of helper methods 
 */

/*
 * Callbacks
 */

static void buddy_signed_on_off_cb(PurpleBuddy* buddy, gpointer data)
{
	LSError lserror;
	LSErrorInit(&lserror);

	PurpleAccount* account = purple_buddy_get_account(buddy);

	std::string const& accountKey = getAccountKeyFromPurpleAccount(account);

	if (s_AccountIdsData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("buddy_signed_on_off_cb: accountId not found in table."));
		return;
	}

	std::string const& accountId = s_AccountIdsData[accountKey];

	std::string const& serviceName = getServiceNameFromPurpleAccount(account);
	PurpleStatus* activeStatus = purple_presence_get_active_status(purple_buddy_get_presence(buddy));
	/*
	 * Getting the new availability
	 */
	int newStatusPrimitive = purple_status_type_get_primitive(purple_status_get_type(activeStatus));
	int newAvailabilityValue = getPalmAvailabilityFromPurpleAvailability(newStatusPrimitive);
	PurpleBuddyIcon* icon = purple_buddy_get_icon(buddy);
	char* buddyAvatarLocation = NULL;

	if (icon)
		buddyAvatarLocation = purple_buddy_icon_get_full_path(icon);
		
	if (buddy->alias)
		buddy->alias = (char*)"";

	const char* customMessage = purple_status_get_attr_string(activeStatus, "message");
	if (!customMessage)
		customMessage = "";

	PurpleGroup* group = purple_buddy_get_group(buddy);
	const char* groupName = purple_group_get_name(group);
	if (!groupName)
		groupName = "";

	// call into the imlibpurpletransport
	// buddy->name is stored in the imbuddyStatus DB kind in the libpurple format - ie. for AIM without the "@aol.com" so that is how we need to search for it
	s_imServiceHandler->updateBuddyStatus(accountId.c_str(), serviceName.c_str(), buddy->name, newAvailabilityValue, customMessage, groupName, buddyAvatarLocation);

	g_message(
			"%s says: %s's presence: availability: '%i', custom message: '%s', avatar location: '%s', display name: '%s', group name: '%s'",
			__FUNCTION__, buddy->name, newAvailabilityValue, customMessage, buddyAvatarLocation, buddy->alias, groupName);
	
	if (buddyAvatarLocation)
	{
		g_free(buddyAvatarLocation);
	}
}

static void buddy_status_changed_cb(PurpleBuddy* buddy, PurpleStatus* old_status, PurpleStatus* new_status,
		gpointer unused)
{
	/*
	 * Getting the new availability
	 */
	int newStatusPrimitive = purple_status_type_get_primitive(purple_status_get_type(new_status));
	int newAvailabilityValue = getPalmAvailabilityFromPurpleAvailability(newStatusPrimitive);

	/*
	 * Getting the new custom message
	 */
	const char* customMessage = purple_status_get_attr_string(new_status, "message");
	if (!customMessage)
		customMessage = "";

	LSError lserror;
	LSErrorInit(&lserror);

	PurpleAccount* account = purple_buddy_get_account(buddy);
	std::string const& accountKey = getAccountKeyFromPurpleAccount(account);

	if (s_AccountIdsData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("buddy_status_changed_cb: accountId not found in table."));
		return;
	}

	std::string const& accountId = s_AccountIdsData[accountKey];

	std::string const& serviceName = getServiceNameFromPurpleAccount(account);

	PurpleBuddyIcon* icon = purple_buddy_get_icon(buddy);
	char* buddyAvatarLocation = NULL;	
	if (icon)
		buddyAvatarLocation = purple_buddy_icon_get_full_path(icon);

	PurpleGroup* group = purple_buddy_get_group(buddy);
	const char* groupName = purple_group_get_name(group);
	if (!groupName)
		groupName = "";

	// call into the imlibpurpletransport
	// buddy->name is stored in the imbuddyStatus DB kind in the libpurple format - ie. for AIM without the "@aol.com" so that is how we need to search for it
	s_imServiceHandler->updateBuddyStatus(accountId.c_str(), serviceName.c_str(), buddy->name, newAvailabilityValue, customMessage, groupName, buddyAvatarLocation);

	g_message(
			"%s says: %s's presence: availability: '%i', custom message: '%s', avatar location: '%s', display name: '%s', group name: '%s'",
			__FUNCTION__, buddy->name, newAvailabilityValue, customMessage, buddyAvatarLocation, buddy->alias, groupName);
	
	if (buddyAvatarLocation)
		g_free(buddyAvatarLocation);
}

static void buddy_avatar_changed_cb(PurpleBuddy* buddy)
{
	PurpleStatus* activeStatus = purple_presence_get_active_status(purple_buddy_get_presence(buddy));
	MojLogInfo(IMServiceApp::s_log, _T("buddy avatar changed for %s"), buddy->name);
	buddy_status_changed_cb(buddy, activeStatus, activeStatus, NULL);
}

/*
 * Called after we remove a buddy from our list
 */
static void buddy_removed_cb(PurpleBuddy* buddy)
{
	// nothing to do...
	MojLogInfo(IMServiceApp::s_log, _T("buddy removed %s"), buddy->name);
}

/*
 * Called after we block a buddy from our list
 */
static void buddy_blocked_cb(PurpleBuddy* buddy)
{
	// nothing to do...
	MojLogInfo(IMServiceApp::s_log, _T("buddy blocked %s"), buddy->name);
}

/*
 * Called both after we add a buddy to our list and when we accept a remote users' invitation to add us to their list
 * buddy is the new buddy
 */
static void buddy_added_cb(PurpleBuddy* buddy)
{
	// nothing to do...
	MojLogInfo(IMServiceApp::s_log, _T("buddy added %s"), buddy->name);
}

static void account_logged_in_cb(PurpleConnection* gc, gpointer loginState)
{
	void* blist_handle = purple_blist_get_handle();
	static int handle;

	PurpleAccount* loggedInAccount = purple_connection_get_account(gc);
	g_return_if_fail(loggedInAccount != NULL);

	std::string const& serviceName = getServiceNameFromPurpleAccount(loggedInAccount);
	std::string const& username = Util::getMojoUsername(loggedInAccount->username, loggedInAccount->protocol_id);
	std::string const& accountKey = getAccountKeyFromPurpleAccount(loggedInAccount);

	if (s_onlineAccountData.find(accountKey) != s_onlineAccountData.end())
	{
		// we were online. why are we getting notified that we're connected again? we were never disconnected.
		// mark the account online just to be sure?
		MojLogError(IMServiceApp::s_log, _T("account_logged_in_cb: account already online. why are we getting notified?"));
		return;
	}

	/*
	 * cancel the connect timeout for this account
	 */
	if (s_onlineAccountData.find(accountKey) != s_onlineAccountData.end())
	{
		guint timerHandle = s_accountLoginTimers[accountKey];
		purple_timeout_remove(timerHandle);
		s_accountLoginTimers.erase(accountKey);
	}

	MojLogInfo(IMServiceApp::s_log, _T("account_logged_in_cb: inserting account into onlineAccountData hash table. accountKey %s"), accountKey.c_str());
	s_onlineAccountData[accountKey] = s_pendingAccountData[accountKey];
	s_pendingAccountData.erase(accountKey);

	MojLogInfo(IMServiceApp::s_log, _T("Account connected..."));

	// reply with login success
	if (loginState)
	{
		((LoginCallbackInterface*)loginState)->loginResult(serviceName.c_str(), username.c_str(), LoginCallbackInterface::LOGIN_SUCCESS, false, ERROR_NO_ERROR, false);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, _T("ERROR: account_logged_in_cb called with loginState=NULL"));
	}

	if (s_registeredForPresenceUpdateSignals == FALSE)
	{
		purple_signal_connect(blist_handle, "buddy-status-changed", &handle, PURPLE_CALLBACK(buddy_status_changed_cb),
				loginState);
		purple_signal_connect(blist_handle, "buddy-signed-on", &handle, PURPLE_CALLBACK(buddy_signed_on_off_cb),
				GINT_TO_POINTER(TRUE));
		purple_signal_connect(blist_handle, "buddy-signed-off", &handle, PURPLE_CALLBACK(buddy_signed_on_off_cb),
				GINT_TO_POINTER(FALSE));
		purple_signal_connect(blist_handle, "buddy-icon-changed", &handle, PURPLE_CALLBACK(buddy_avatar_changed_cb),
				GINT_TO_POINTER(FALSE));
		purple_signal_connect(blist_handle, "buddy-removed", &handle, PURPLE_CALLBACK(buddy_removed_cb),
				GINT_TO_POINTER(FALSE));
		purple_signal_connect(blist_handle, "buddy-added", &handle, PURPLE_CALLBACK(buddy_added_cb),
				GINT_TO_POINTER(FALSE));
		purple_signal_connect(blist_handle, "buddy-privacy-changed", &handle, PURPLE_CALLBACK(buddy_blocked_cb),
				GINT_TO_POINTER(FALSE));

		// testing. Doesn't work: error - "Signal data for sent-im-msg not found". Need to figure out the right handle
//		purple_signal_connect(purple_connections_get_handle(), "sent-im-msg", &handle, PURPLE_CALLBACK(sent_message_cb),
//						GINT_TO_POINTER(FALSE));
		s_registeredForPresenceUpdateSignals = TRUE;
	}
}

static void account_signed_off_cb(PurpleConnection* gc, gpointer loginState)
{
	PurpleAccount* account = purple_connection_get_account(gc);
	g_return_if_fail(account != NULL);

	std::string const& accountKey = getAccountKeyFromPurpleAccount(account);
	if (s_onlineAccountData.count(accountKey))
	{
		MojLogInfo(IMServiceApp::s_log, _T("account_signed_off_cb: removing account from onlineAccountData hash table. accountKey %s"), accountKey.c_str());
		s_onlineAccountData.erase(accountKey);
	}
	else if (s_pendingAccountData.count(accountKey))
	{
		s_pendingAccountData.erase(accountKey);
	}
	else
	{
		// Already signed off this account (or never signed in) so just return
		return;
	}

	s_ipAddressesBoundTo.erase(accountKey);
	//g_hash_table_remove(connectionTypeData, accountKey);

	MojLogInfo(IMServiceApp::s_log, _T("Account disconnected..."));

	if (s_offlineAccountData.count(accountKey))
	{
		/*
		 * Keep the PurpleAccount struct to reuse in future logins
		 */
		s_offlineAccountData[accountKey] = account;
	}
	
	// reply with signed off
	if (loginState)
	{
		std::string const& serviceName = getServiceNameFromPurpleAccount(account);
		std::string const& username = Util::getMojoUsername(account->username, account->protocol_id);
		((LoginCallbackInterface*)loginState)->loginResult(serviceName.c_str(), username.c_str(), LoginCallbackInterface::LOGIN_SIGNED_OFF, false, ERROR_NO_ERROR, true);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, _T("ERROR: account_logged_in_cb called with loginState=NULL"));
	}
}

/*
 * This callback is called if a) the login attempt failed, or b) login was successful but the session was closed 
 * (e.g. connection problems, etc).
 */
static void account_login_failed_cb(PurpleConnection* gc, PurpleConnectionError type, const gchar* description,
		gpointer loginState)
{
	MojLogInfo(IMServiceApp::s_log, _T("account_login_failed is called with description %s"), description);

	PurpleAccount* account = purple_connection_get_account(gc);
	g_return_if_fail(account != NULL);

	gboolean loggedOut = FALSE;
	bool noRetry = true;
	std::string const& accountKey = getAccountKeyFromPurpleAccount(account);

	if (s_onlineAccountData.count(accountKey))
	{
		/* 
		 * We were online on this account and are now disconnected because either a) the data connection is dropped, 
		 * b) the server is down, or c) the user has logged in from a different location and forced this session to
		 * get closed.
		 */
		// Special handling for broken network connection errors (due to bad coverage or flight mode)
		if (type == 0)//PURPLE_CONNECTION_ERROR_NETWORK_ERROR)
		{
			MojLogInfo(IMServiceApp::s_log, _T("We had a network error. Reason: %s, prpl error code: %i"), description, type);
			noRetry = false;
		}
		else
		{
			loggedOut = TRUE;
			MojLogInfo(IMServiceApp::s_log, _T("We were logged out. Reason: %s, prpl error code: %i"), description, type);
		}
	}
	else
	{
		/*
		 * cancel the connect timeout for this account
		 */
		if (s_accountLoginTimers.count(accountKey))
		{
			guint timerHandle = s_accountLoginTimers[accountKey];
			purple_timeout_remove(timerHandle);
			s_accountLoginTimers.erase(accountKey);
		}

		if (s_pendingAccountData.count(accountKey) == 0)
		{
			/*
			 * This account was in neither of the account data lists (online or pending). We must have logged it out 
			 * and not cared about letting mojo know about it (probably because mojo went down and came back up and
			 * thought that the account was logged out anyways)
			 */
			MojLogError(IMServiceApp::s_log, _T("account_login_failed_cb: account in neither online or pending list. Why are we getting logged out? description %s:"), description);
			return;
		}
		else
		{
			s_pendingAccountData.erase(accountKey);
		}
	}

	const char* mojoFriendlyErrorCode = getMojoFriendlyErrorCode(type);
	MojLogInfo(IMServiceApp::s_log, _T("account_login_failed_cb: removing account from onlineAccountData hash table. accountKey %s"), accountKey.c_str());

	
	s_onlineAccountData.erase(accountKey);
	s_ipAddressesBoundTo.erase(accountKey);
	s_connectionTypeData.erase(accountKey);

	s_offlineAccountData[accountKey] = account;
 
	// reply with login failed
	if (loginState != NULL)
	{
		std::string const& serviceName = getServiceNameFromPurpleAccount(account);
		std::string const& username = Util::getMojoUsername(account->username, account->protocol_id);
		//TODO: determine if there are cases where noRetry should be false
		//TODO: include "description" parameter because it had useful details?
		((LoginCallbackInterface*)loginState)->loginResult(serviceName.c_str(), username.c_str(), LoginCallbackInterface::LOGIN_FAILED, loggedOut, mojoFriendlyErrorCode, noRetry);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, _T("ERROR: account_login_failed_cb called with loginState=NULL"));
	}
}

static void account_status_changed(PurpleAccount* account, PurpleStatus* oldStatus, PurpleStatus* newStatus, gpointer loginState)
{
	printf("\n\n ACCOUNT STATUS CHANGED \n\n");
}

/*
 *  This gets called then we decline a remote user's buddy invite
 *  TODO - what signal gets emitted when a remote user declines our invitation???
 */
static void account_auth_deny_cb(PurpleAccount* account, const char* remote_user)
{
	MojLogInfo(IMServiceApp::s_log, _T("account_auth_deny_cb called. account: %s, remote_user: %s"), account->username, remote_user);

	// TODO this needs to happen when remote user declines our invite, not here...
//	char* serviceName = getServiceNameFromPrplProtocolId(account->protocol_id);
//	char* username = getMojoFriendlyUsername(account->username, serviceName);
//	char* usernameFromStripped = stripResourceFromGtalkUsername(remote_user, serviceName);
//
//	// tell transport to delete the buddy and contacts
//	s_imServiceHandler->buddyInviteDeclined(serviceName, username, usernameFromStripped);
//
//	// clean up
//	free(serviceName);
//	free(username);
//	free(usernameFromStripped);


}

/*
 * This gets called then we accept a remote user's buddy invite
 *      log: account_auth_accept_cb called. account: palm@gmail.com/Home, buddy: palm3@gmail.com
 */
static void account_auth_accept_cb(PurpleAccount* account, const char* remote_user)
{
	// nothing to do
	MojLogInfo(IMServiceApp::s_log, _T("account_auth_accept_cb called. account: %s, remote_user: %s"), account->username, remote_user);
}


void incoming_message_cb(PurpleConversation* conv, const char* who, const char* alias, const char* message,
		PurpleMessageFlags flags, time_t mtime)
{
	/*
	 * snippet taken from nullclient
	 */
	const char* usernameFrom;
	if (who && *who)
		usernameFrom = who;
	else if (alias && *alias)
		usernameFrom = alias;
	else
		usernameFrom = "";

	if ((flags & PURPLE_MESSAGE_RECV) != PURPLE_MESSAGE_RECV)
	{
		/* this is a sent message. ignore it. */
		return;
	}

	PurpleAccount* account = purple_conversation_get_account(conv);

	// these never return null...
	std::string const& serviceName = getServiceNameFromPurpleAccount(account);
	std::string const& username = Util::getMojoUsername(account->username, account->protocol_id);

	// call the transport service incoming message handler
	s_imServiceHandler->incomingIM(serviceName.c_str(), username.c_str(), usernameFrom, message);
}

/*
 * Called when a remote user requests authorization to be our buddy
 *
 * Note: this method will get called every time user logs in if there is a pending invitation.
 */
static void *request_authorize_cb (PurpleAccount *account, const char *remote_user, const char *id,	const char *alias, const char *message,
	gboolean on_list, PurpleAccountRequestAuthorizationCb authorize_cb,	PurpleAccountRequestAuthorizationCb deny_cb,
	void *user_data)
{

	MojLogInfo(IMServiceApp::s_log, _T("request_authorize_cb called. remote user: %s, id: %s, message: %s"), remote_user, id, message);

	// these never return null...
	std::string const& serviceName = getServiceNameFromPurpleAccount(account);
	std::string const& username = Util::getMojoUsername(account->username, account->protocol_id);
	const char* usernameFromStripped = remote_user;

	// Save off the authorize/deny callbacks to use later
	AuthRequest *aa = g_new0(AuthRequest, 1);
	aa->auth_cb = authorize_cb;
	aa->deny_cb = deny_cb;
	aa->data = user_data;
	aa->remote_user = g_strdup(remote_user);
	aa->alias = g_strdup(alias);
	aa->account = account;

	char *authRequestKey = getAuthRequestKey(username.c_str(), serviceName.c_str(), usernameFromStripped);
	// if there is already an entry for this, we need to replace it since callback function pointers will change on login
	// old object gets deleted by our destroy functions specified in the hash table construction
	g_hash_table_replace(s_AuthorizeRequests, authRequestKey, aa);
	// log the table
	logAuthRequestTableValues();

	// call back into IMServiceHandler to create a receivedBuddyInvite imCommand.
	s_imServiceHandler->receivedBuddyInvite(serviceName.c_str(), username.c_str(), usernameFromStripped, message);

	// don't free the authRequestKey - it is not copied, but held onto for the life of the hash table once inserted
	return NULL;
}

/*
 * Not used
 */
void request_add_cb(PurpleAccount *account, const char *remote_user, const char *id, const char *alias, const char *message) {

	MojLogInfo(IMServiceApp::s_log, _T("request_add_cb called. remote user: %s"), remote_user);
}

gboolean connectTimeoutCallback(gpointer data)
{
	std::string* data_ptr = reinterpret_cast<std::string*> (data);
	std::string accountKey = *data_ptr; 
	delete data_ptr;
	if (s_pendingAccountData.count(accountKey) == 0)
	{
		/*
		 * If the account is not pending anymore (which means login either already failed or succeeded) 
		 * then we shouldn't have gotten to this point since we should have cancelled the timer
		 */
		MojLogError(IMServiceApp::s_log,
				_T("WARNING: we shouldn't have gotten to connectTimeoutCallback since login had already failed/succeeded."));
		return FALSE;
	}

	PurpleAccount* account = s_pendingAccountData[accountKey];

	/*
	 * abort logging in since our connect timeout has hit before login either failed or succeeded
	 */
	s_accountLoginTimers.erase(accountKey);
	s_pendingAccountData.erase(accountKey);
	s_ipAddressesBoundTo.erase(accountKey);

	purple_account_disconnect(account);

	if (s_loginState)
	{
		std::string const& serviceName = getServiceNameFromPurpleAccount(account);
		std::string const& username = Util::getMojoUsername(account->username, account->protocol_id);

		s_loginState->loginResult(serviceName.c_str(), username.c_str(), LoginCallbackInterface::LOGIN_TIMEOUT, false, ERROR_NETWORK_ERROR, true);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, _T("ERROR: connectTimeoutCallback called with s_loginState=NULL."));
	}

	return FALSE;
}

/*
 * End of callbacks
 */

/*
 * libpurple initialization methods
 */

static GHashTable* getClientInfo(void)
{
	GHashTable* clientInfo = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	g_hash_table_insert(clientInfo, (void*)"name", (void*)"Palm Messaging");
	g_hash_table_insert(clientInfo, (void*)"version", (void*)"");

	return clientInfo;
}

static void initializeLibpurple()
{
	signal(SIGCHLD, SIG_IGN);

	/* Set a custom user directory (optional) */
	purple_util_set_user_dir(CUSTOM_USER_DIRECTORY);

	/* We do not want any debugging for now to keep the noise to a minimum. */
	purple_debug_set_enabled(TRUE);

	/* Set the core-uiops, which is used to
	 * 	- initialize the ui specific preferences.
	 * 	- initialize the debug ui.
	 * 	- initialize the ui components for all the modules.
	 * 	- uninitialize the ui components for all the modules when the core terminates.
	 */
	purple_core_set_ui_ops(&adapterCoreUIOps);

	purple_eventloop_set_ui_ops(&adapterEventLoopUIOps);

	// TODO - there is a memory leak in here...at least on desktop
	if (!purple_core_init(UI_ID))
	{
		MojLogInfo(IMServiceApp::s_log, _T("libpurple initialization failed."));
		abort();
	}

	/* Create and load the buddylist. */
	purple_set_blist(purple_blist_new());
	purple_blist_load();

	purple_buddy_icons_set_cache_dir("/var/luna/data/im-avatars");

	s_libpurpleInitialized = TRUE;
	MojLogInfo(IMServiceApp::s_log, _T("libpurple initialized.\n"));
}
/*
 * End of libpurple initialization methods
 */

/*
 * Service methods
 */
LibpurpleAdapter::LoginResult LibpurpleAdapter::login(LoginParams const& params, LoginCallbackInterface* loginState)
{
	LoginResult result = OK;

	PurpleAccount* account;
	bool accountIsAlreadyOnline = FALSE;
	bool accountIsAlreadyPending = FALSE;

	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	PurpleAccount* alreadyActiveAccount = NULL;

	if (params.serviceName.empty() || params.username.empty())
	{
		MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login with empty username or serviceName"));
		return INVALID_CREDENTIALS;
	}

	MojLogInfo(IMServiceApp::s_log, _T("Parameters: accountId %s, servicename %s, connectionType %s"), params.accountId.data(), params.serviceName.data(), params.connectionType.data());

	if (s_libpurpleInitialized == FALSE)
	{
		initializeLibpurple();
	}

	/* libpurple variables */
	std::string const& accountKey = getAccountKey(params.username.data(), params.serviceName.data());

	// If this account id isn't yet stored, then keep track of it now.
	s_AccountIdsData[accountKey] = params.accountId.data();

	/*
	 * Let's check to see if we're already logged in to this account or that we're already in the process of logging in 
	 * to this account. This can happen when mojo goes down and comes back up.
	 */
	if (s_onlineAccountData.count(accountKey))
	{
		accountIsAlreadyOnline = true;
		alreadyActiveAccount = s_onlineAccountData[accountKey];
	}
	else
	{
		if (s_pendingAccountData.count(accountKey))
		{
			alreadyActiveAccount = s_pendingAccountData[accountKey];
			accountIsAlreadyPending = TRUE;
		}
	}

	if (alreadyActiveAccount)
	{
		/*
		 * We're either already logged in to this account or we're already in the process of logging in to this account 
		 * (i.e. it's pending; waiting for server response)
		 */
		std::string const& accountBoundToIpAddress = s_ipAddressesBoundTo[accountKey];
		if (params.localIpAddress.data() == accountBoundToIpAddress)
		{
			/*
			 * We're using the right interface for this account
			 */
			if (accountIsAlreadyPending)
			{
				MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login: We were already in the process of logging in."));
				return OK;
			}
			else if (accountIsAlreadyOnline)
			{
				MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login: We were already logged in to the requested account."));
				return ALREADY_LOGGED_IN;
			}
		}
		else
		{
			/*
			 * We're not using the right interface. Close the current connection for this account and create a new one
			 */
			MojLogError(IMServiceApp::s_log,
					_T("LibpurpleAdapter::login: We have to logout and login again since the local IP address has changed. Logging out from account."));
			/*
			 * Once the current connection is closed we don't want to let mojo know that the account was disconnected.
			 * Since mojo went down and came back up it didn't know that the account was connected anyways.
			 * So let's take the account out of the account data hash and then disconnect it.
			 */
			if (s_onlineAccountData.count(accountKey))
			{
				MojLogInfo(IMServiceApp::s_log, _T("LibpurpleAdapter::login: removing account from onlineAccountData hash table. accountKey %s"), accountKey.c_str());
				s_onlineAccountData.erase(accountKey);
			}
			if (s_pendingAccountData.count(accountKey))
				s_pendingAccountData.erase(accountKey);

			purple_account_disconnect(alreadyActiveAccount);
		}
	}

	/*
	 * Let's go through our usual login process
	 */

	// TODO this currently ignores authentication token, but should check it as well when support for auth token is added
	if (params.password.empty())
	{
		MojLogError(IMServiceApp::s_log, _T("Error: null or empty password trying to log in to servicename %s"), params.serviceName.data());
	    return INVALID_CREDENTIALS;
	}
	else
	{
		/* save the local IP address that we need to use */
		// TODO - move this to #ifdef. If you are running imlibpurpletransport on desktop, but tethered to device, params->localIpAddress needs to be set to
		// NULL otherwise login will fail...
		if (!params.localIpAddress.empty())
		{
			purple_prefs_remove("/purple/network/preferred_local_ip_address");
			purple_prefs_add_string("/purple/network/preferred_local_ip_address", params.localIpAddress.data());
		}
		else
		{
#ifdef DEVICE
			/*
			 * If we're on device you should not accept an empty ipAddress; it's mandatory to be provided
			 */
			MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login with missing localIpAddress"));
			return FAILED;
#endif
		}

		/* save the local IP address that we need to use */
		s_connectionTypeData[accountKey] = params.connectionType;

		/*
		 * If we've already logged in to this account before then re-use the old PurpleAccount struct
		 */
		if (s_offlineAccountData.count(accountKey))
			account = s_offlineAccountData[accountKey];
		else
		{
			/* Create the account */
			account = Util::createPurpleAccount(params.username, params.prpl, params.config);
			if (!account)
			{
				MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login failed to create new Purple account"));
				return FAILED;
			}

			AccountMetaData* amd = new AccountMetaData;
			amd->account_key = strndup(accountKey.c_str(), accountKey.size());
			amd->servicename = params.serviceName.data();

			account->ui_data = (void*)amd;
		}

		MojLogInfo(IMServiceApp::s_log, _T("Logging in..."));

		purple_account_set_password(account, params.password.data());
	}

	if (result == OK)
	{
		/* mark the account as pending */
		s_pendingAccountData[accountKey] = account;

        /* keep track of the local IP address that we bound to when logging in to this account */
        s_ipAddressesBoundTo[accountKey] = params.localIpAddress;

		/* It's necessary to enable the account first. */
		purple_account_set_enabled(account, UI_ID, TRUE);

		/* Now, to connect the account, create a status and activate it. */

		/*
		 * Create a timer for this account's login so it can fail the login after a timeout.
		 */
		guint timerHandle = purple_timeout_add_seconds(CONNECT_TIMEOUT_SECONDS, connectTimeoutCallback, new std::string(accountKey));
		s_accountLoginTimers[accountKey] = timerHandle;

		PurpleStatusPrimitive prim = getPurpleAvailabilityFromPalmAvailability(params.availability);
		PurpleSavedStatus* savedStatus = purple_savedstatus_new(NULL, prim);
		if (!params.customMessage.empty())
		{
			purple_savedstatus_set_message(savedStatus, params.customMessage);
		}
		purple_savedstatus_activate_for_account(savedStatus, account);
	}

	return result;
}

bool LibpurpleAdapter::logout(const char* serviceName, const char* username, LoginCallbackInterface* loginState)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!username || !serviceName)
	{
		MojLogError(IMServiceApp::s_log, _T("Invalid logout parameter. Please double check the passed parameters."));
		return FALSE;
	}

	bool success = true;

	MojLogInfo(IMServiceApp::s_log, _T("Parameters: servicename %s"), serviceName);

	std::string const& accountKey = getAccountKey(username, serviceName);

	// Remove the accountId since a logout could be from the user removing the account
	s_AccountIdsData.erase(accountKey);

	PurpleAccount* accountTologoutFrom = 0;

	if (s_onlineAccountData.count(accountKey))
		accountTologoutFrom = s_onlineAccountData[accountKey];
	else if (s_pendingAccountData.count(accountKey))
		accountTologoutFrom = s_pendingAccountData[accountKey];
	else
	{
		MojLogError(IMServiceApp::s_log, _T("Trying to logout from an account that is not logged in. service name %s"), serviceName);
		success = false;
	}

	if (accountTologoutFrom != 0)
	{
		delete (AccountMetaData*)accountTologoutFrom->ui_data;
		purple_account_disconnect(accountTologoutFrom);
	}

	return success;
}

bool LibpurpleAdapter::setMyAvailability(const char* serviceName, const char* username, int availability)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName || !username)
	{
		MojLogError(IMServiceApp::s_log, _T("setMyAvailability: Invalid parameter. Please double check the passed parameters."));
		return false;
	}

	MojLogInfo(IMServiceApp::s_log, _T("Parameters: serviceName %s, availability %i"), serviceName, availability);

	std::string accountKey = getAccountKey(username, serviceName);
	if (s_onlineAccountData.count(accountKey) == 0)
	{
		//this should never happen based on MessagingService's logic
		MojLogError(IMServiceApp::s_log,
				_T("setMyAvailability was called on an account that wasn't logged in. serviceName: %s, availability: %i"),
				serviceName, availability);
		return false;
	}

	PurpleAccount* account = s_onlineAccountData[accountKey];

	/*
	 * Let's get the current custom message and set it as well so that we don't overwrite it with ""
	 */
	PurplePresence* presence = purple_account_get_presence(account);
	const PurpleStatus* status = purple_presence_get_active_status(presence);
	const PurpleValue* value = purple_status_get_attr_value(status, "message");
	const char* customMessage = NULL;
	if (value != NULL)
	{
		customMessage = purple_value_get_string(value);
	}
	if (customMessage == NULL)
	{
		customMessage = "";
	}

	PurpleStatusPrimitive prim = getPurpleAvailabilityFromPalmAvailability(availability);
	PurpleStatusType* type = purple_account_get_status_type_with_primitive(account, prim);
	GList* attrs = NULL;
	attrs = g_list_append(attrs, (void*)"message");
	attrs = g_list_append(attrs, (char*)customMessage);
	purple_account_set_status_list(account, purple_status_type_get_id(type), TRUE, attrs);

	return true;
}

bool LibpurpleAdapter::setMyCustomMessage(const char* serviceName, const char* username, const char* customMessage)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName || !username || !customMessage)
	{
		MojLogError(IMServiceApp::s_log, _T("setMyCustomMessage: Invalid parameter. Please double check the passed parameters."));
		return FALSE;
	}

	MojLogInfo(IMServiceApp::s_log, _T("Parameters: serviceName %s"), serviceName);

	std::string accountKey = getAccountKey(username, serviceName);

	if (s_onlineAccountData.count(accountKey) == 0)
	{
		//this should never happen based on MessagingService's logic
		MojLogError(IMServiceApp::s_log,
				_T("setMyCustomMessage was called on an account that wasn't logged in. serviceName: %s"),
				serviceName);
		return false;
	}

	PurpleAccount* account = s_onlineAccountData[accountKey];
	// get the account's current status type
	PurpleStatusType* type = purple_status_get_type(purple_account_get_active_status(account));
	GList* attrs = NULL;
	attrs = g_list_append(attrs, (void*)"message");
	attrs = g_list_append(attrs, (char*)customMessage);
	purple_account_set_status_list(account, purple_status_type_get_id(type), TRUE, attrs);

	return true;
}

/*
 * Block this user from sending us messages
 */
LibpurpleAdapter::SendResult LibpurpleAdapter::blockBuddy(const char* serviceName, const char* username, const char* buddyUsername, bool block)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName)
	{
		MojLogError(IMServiceApp::s_log, _T("blockBuddy: null serviceNname"));
		return INVALID_PARAMS;
	}
	if (!username)
	{
		MojLogError(IMServiceApp::s_log, _T("blockBuddy: null username"));
		return INVALID_PARAMS;
	}
	if (!buddyUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("blockBuddy: null buddyUsername"));
		return INVALID_PARAMS;
	}

	std::string accountKey = getAccountKey(username, serviceName);

	if (s_onlineAccountData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, "blockBuddy: Trying to send from an account that is not logged in. service name %s", serviceName);
		return USER_NOT_LOGGED_IN;
	}

	PurpleAccount* account = s_onlineAccountData[accountKey];

	// strip off the "@aol.com" if needed
	std::string const& transportFriendlyUserName = Util::getPurpleUsername(serviceName, buddyUsername);
	if (block)
	{
		MojLogInfo(IMServiceApp::s_log, _T("blockBuddy: deny %s. account perm_deny %d"), transportFriendlyUserName.c_str(), account->perm_deny);
		purple_privacy_deny(account, transportFriendlyUserName.c_str(), false, true);
		//bool success = purple_privacy_deny_add(account, transportFriendlyUserName, false);
		//if (!success) {
		//	MojLogError(IMServiceApp::s_log, "blockBuddy: purple_privacy_deny_add - returned false");
		//	GSList *l;
		//	for (l = account->deny; l != NULL; l = l->next) {
		//		MojLogError(IMServiceApp::s_log, "account deny list: %s", purple_normalize(account, (char *)l->data));
		//	}
		//}
	}
	else
	{
		MojLogInfo(IMServiceApp::s_log, _T("blockBuddy: allow %s"), transportFriendlyUserName.c_str());
		purple_privacy_allow(account, transportFriendlyUserName.c_str(), false, true);
	}

	return SENT;
}

/*
 * Remove a buddy from our account
 */
LibpurpleAdapter::SendResult LibpurpleAdapter::removeBuddy(const char* serviceName, const char* username, const char* buddyUsername)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName)
	{
		MojLogError(IMServiceApp::s_log, _T("removeBuddy: null serviceNname"));
		return INVALID_PARAMS;
	}
	if (!username)
	{
		MojLogError(IMServiceApp::s_log, _T("removeBuddy: null username"));
		return INVALID_PARAMS;
	}
	if (!buddyUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("removeBuddy: null buddyUsername"));
		return INVALID_PARAMS;
	}

	std::string accountKey = getAccountKey(username, serviceName);

	if (s_onlineAccountData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, "removeBuddy: Trying to send from an account that is not logged in. service name %s", serviceName);
		return USER_NOT_LOGGED_IN;
	}

	PurpleAccount* account = s_onlineAccountData[accountKey];
	// strip off the "@aol.com" if needed
	std::string const& transportFriendlyUserName = Util::getPurpleUsername(serviceName, buddyUsername);
	MojLogInfo(IMServiceApp::s_log, _T("removeBuddy: %s"), transportFriendlyUserName.c_str());

	PurpleBuddy* buddy = purple_find_buddy(account, transportFriendlyUserName.c_str());
	if (NULL == buddy) {
		MojLogError(IMServiceApp::s_log, _T("could not find buddy in list - cannot remove"));
		return INVALID_PARAMS;
	}
	else {
		PurpleGroup* group = purple_buddy_get_group(buddy);
		// remove from server list
		purple_account_remove_buddy(account, buddy, group);

		// remove from buddy list - generates a "buddy-removed" signal
		purple_blist_remove_buddy(buddy);
	}
	return SENT;
}

/*
 * Add a buddy to our buddy list. Some services (GTalk) will require the buddy to authorize us to add them
 */
LibpurpleAdapter::SendResult LibpurpleAdapter::addBuddy(const char* serviceName, const char* username, const char* buddyUsername, const char* groupName)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName)
	{
		MojLogError(IMServiceApp::s_log, _T("addBuddy: null serviceNname"));
		return INVALID_PARAMS;
	}
	if (!username)
	{
		MojLogError(IMServiceApp::s_log, _T("addBuddy: null username"));
		return INVALID_PARAMS;
	}
	if (!buddyUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("addBuddy: null buddyUsername"));
		return INVALID_PARAMS;
	}

	std::string accountKey = getAccountKey(username, serviceName);

	if (s_onlineAccountData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, "addBuddy: Trying to send from an account that is not logged in. service name %s", serviceName);
		return USER_NOT_LOGGED_IN;
	}

	PurpleAccount* account = s_onlineAccountData[accountKey];

	// strip off the "@aol.com" if needed
	std::string const& transportFriendlyUserName = Util::getPurpleUsername(serviceName, buddyUsername);
	MojLogInfo(IMServiceApp::s_log, _T("addBuddy: %s"), transportFriendlyUserName.c_str());

	PurpleBuddy* buddy = purple_buddy_new(account, transportFriendlyUserName.c_str(), /*alias*/ NULL);

	// add buddy to list
	/*
	void purple_blist_add_buddy  	(  	PurpleBuddy *   	 buddy,
			PurpleContact *  	contact,
			PurpleGroup *  	group,
			PurpleBlistNode *  	node
		)

	Adds a new buddy to the buddy list.
	The buddy will be inserted right after node or prepended to the group if node is NULL. If both are NULL, the buddy will be added to the "Buddies" group.
	*/
	PurpleGroup *group = NULL;
	if (NULL != groupName && *groupName != '\0') {
		group = purple_find_group(groupName);
		if (NULL == group){
			// group not there - add it
			MojLogInfo(IMServiceApp::s_log, _T("addBuddy: adding new group %s"), groupName);
			group = purple_group_new(groupName);
		}
	}
	purple_blist_add_buddy(buddy, NULL, group, NULL);

	// add to server - note: this has to be called AFTER the purple_blist_add_buddy() call otherwise it seg faults...
	purple_account_add_buddy(account, buddy);

	// note - there seems to be an inconsistency in libpurple where AIM buddies added via purple appear offline until the account is logged off and on...
	// see http://pidgin.im/pipermail/devel/2007-June/001517.html:
	/*	Such is not the case on AIM, however.  The behavior I see here is that the
		buddies appear in the Pidgin blist but always have an offline status, even
		though I know at least four of these people are online.  Looking at blist.xml
		after the next shown flush in the debug window shows that nothing has been added
		to the local list.  The alias and buddy notes are not added, either.  The status
		remains incorrect until I restart Pidgin, disable and enable the account, or
		switch to the offline status and then back to an online status.  At this point,
		the buddies are finally shown in blist.xml and statuses are correct; however,
		the alias and notes string that were set at import are missing.  Behavior is
		identical on ICQ when importing a list of AIM buddies (which to my limited
		knowledge does not require authorization).
	*/

	return SENT;
}

/*
 * Authorize the remote user to be our buddy
 */
LibpurpleAdapter::SendResult LibpurpleAdapter::authorizeBuddy(const char* serviceName, const char* username, const char* fromUsername)
{
	MojLogInfo(IMServiceApp::s_log, _T("authorizeBuddy: username: %s, serviceName: %s, buddyUsername: %s"), username, serviceName, fromUsername);

	if (!serviceName || !username || !fromUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("authorizeBuddy: null parameter. cannot process command"));
		return INVALID_PARAMS;
	}

	// if we got here, we need to be online...
	std::string const& accountKey = getAccountKey(username, serviceName);


	if (s_onlineAccountData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("authorizeBuddy: account not online"));
		return USER_NOT_LOGGED_IN;
	}

	// create the key and find the auth_and_add object in the s_AuthorizeRequests table
	char *authRequestKey = getAuthRequestKey(username, serviceName, fromUsername);
	AuthRequest *aa = (AuthRequest *)g_hash_table_lookup(s_AuthorizeRequests, authRequestKey);
	if (NULL == aa)
	{
		MojLogError(IMServiceApp::s_log, "authorizeBuddy: cannot find auth request object");
		// log the table
		logAuthRequestTableValues();
		free (authRequestKey);
		return SEND_FAILED;
	}

	// authorize account
	aa->auth_cb(aa->data);

	// TODO - do we need to add this user to our buddy list? Libpurple seems to add it automatically - appears at next login.

	// we are done with this request - remove it from list
	// object and key held by table get deleted by our destroy functions specified in the hash table construction
	g_hash_table_remove(s_AuthorizeRequests, authRequestKey);

	// free key used for look-up
	free (authRequestKey);

	return SENT;
}

/*
 * Decline the request from the remote user to be our buddy
 */
LibpurpleAdapter::SendResult LibpurpleAdapter::declineBuddy(const char* serviceName, const char* username, const char* fromUsername)
{
	MojLogInfo(IMServiceApp::s_log, _T("declineBuddy: username: %s, serviceName: %s, buddyUsername: %s"), username, serviceName, fromUsername);
	if (!serviceName)
	{
		MojLogError(IMServiceApp::s_log, _T("declineBuddy: null serviceNname"));
		return INVALID_PARAMS;
	}
	if (!username)
	{
		MojLogError(IMServiceApp::s_log, _T("declineBuddy: null username"));
		return INVALID_PARAMS;
	}
	if (!fromUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("declineBuddy: null fromUsername"));
		return INVALID_PARAMS;
	}

	// if we got here, we need to be online...
	std::string accountKey = getAccountKey(username, serviceName);
	if (s_onlineAccountData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("declineBuddy: account not online"));
		return USER_NOT_LOGGED_IN;
	}

	// create the key and find the auth_and_add object in the s_AuthorizeRequests table
	char *authRequestKey = getAuthRequestKey(username, serviceName, fromUsername);
	AuthRequest *aa = (AuthRequest *)g_hash_table_lookup(s_AuthorizeRequests, authRequestKey);
	if (NULL == aa)
	{
		MojLogError(IMServiceApp::s_log, "declineBuddy: cannot find auth request object");
		// log the table
		logAuthRequestTableValues();
		free (authRequestKey);
		return SEND_FAILED;
	}

	aa->deny_cb(aa->data);

	// we are done with this request - remove it from list
	// object gets deleted by our destroy functions specified in the hash table construction
	g_hash_table_remove(s_AuthorizeRequests, authRequestKey);

	// free key used for look-up
	free (authRequestKey);
	return SENT;
}

bool LibpurpleAdapter::getFullBuddyList(const char* serviceName, const char* username)
{
	MojLogInfo(IMServiceApp::s_log, "%s called.", __FUNCTION__);

	if (!serviceName || !username)
	{
		MojLogError(IMServiceApp::s_log, _T("getBuddyList: Invalid parameter. Please double check the passed parameters."));
		return FALSE;
	}

	if (s_loginState == NULL)
	{
		MojLogError(IMServiceApp::s_log, _T("getBuddyList: s_loginState still null."));
		return FALSE;
	}

	MojLogInfo(IMServiceApp::s_log, _T("getFullBuddyList: Parameters: serviceName %s, username %s"), serviceName, username);

	/*
	 * Send over the full buddy list if the account is already logged in
	 */
	bool success;
	std::string accountKey = getAccountKey(username, serviceName);
	if (s_onlineAccountData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("getFullBuddyList: ERROR: No account for user on %s"), serviceName);
		return false;;
	}

	PurpleAccount* account = s_onlineAccountData[accountKey];
	success = TRUE;
	GSList* buddyList = purple_find_buddies(account, NULL);
	MojObject buddyListObj;
	if (!buddyList)
	{
		MojLogError(IMServiceApp::s_log, _T("getFullBuddyList: WARNING: the buddy list was NULL, returning empty buddy list."));
		s_loginState->buddyListResult(serviceName, username, buddyListObj, true);
	}

	GSList* buddyIterator = NULL;
	PurpleBuddy* buddyToBeAdded = NULL;
	PurpleGroup* group = NULL;
	char* buddyAvatarLocation = NULL;

	for (buddyIterator = buddyList; buddyIterator != NULL; buddyIterator = buddyIterator->next)
	{
		MojObject buddyObj;
		buddyObj.clear(MojObject::TypeArray);
		buddyToBeAdded = (PurpleBuddy*)buddyIterator->data;

		buddyObj.putString("username", buddyToBeAdded->name);
		buddyObj.putString("serviceName", serviceName);

		group = purple_buddy_get_group(buddyToBeAdded);
		const char* groupName = purple_group_get_name(group);
		buddyObj.putString("group", groupName);

		/*
		 * Getting the availability
		 */
		PurpleStatus* status = purple_presence_get_active_status(purple_buddy_get_presence(buddyToBeAdded));
		int newStatusPrimitive = purple_status_type_get_primitive(purple_status_get_type(status));
		int availability = getPalmAvailabilityFromPurpleAvailability(newStatusPrimitive);
		buddyObj.putInt("availability", availability);

		if (buddyToBeAdded->alias != NULL)
		{
			buddyObj.putString("displayName", buddyToBeAdded->alias);
		}

		PurpleBuddyIcon* icon = purple_buddy_get_icon(buddyToBeAdded);
		if (icon != NULL)
		{
			buddyAvatarLocation = purple_buddy_icon_get_full_path(icon);
			buddyObj.putString("avatar", buddyAvatarLocation);
		}
		else
		{
			buddyObj.putString("avatar", "");
		}

		const char* customMessage = purple_status_get_attr_string(status, "message");
		if (customMessage != NULL)
		{
			buddyObj.putString("status", customMessage);
		}

		g_message("%s says: %s's presence: availability: '%d', custom message: '%s', avatar location: '%s', display name: '%s', group name:'%s'",
				__FUNCTION__, buddyToBeAdded->name, availability, customMessage, buddyAvatarLocation, buddyToBeAdded->alias, groupName);

		if (buddyAvatarLocation)
		{
			g_free(buddyAvatarLocation);
			buddyAvatarLocation = NULL;
		}

		buddyListObj.push(buddyObj);

		s_loginState->buddyListResult(serviceName, username, buddyListObj, true);
		//TODO free the buddyList object???
	}

	return true;
}

LibpurpleAdapter::SendResult LibpurpleAdapter::sendMessage(const char* serviceName, const char* username, const char* usernameTo, const char* messageText)
{
	if (!serviceName || !username || !usernameTo || !messageText)
	{
		MojLogError(IMServiceApp::s_log, _T("sendMessage: Invalid parameter. Please double check the passed parameters."));
		return LibpurpleAdapter::INVALID_PARAMS;
	}

	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	std::string accountKey = getAccountKey(username, serviceName);

	if (s_onlineAccountData.count(accountKey) == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("sendMessage: Trying to send from an account that is not logged in. service name %s"), serviceName);
		return LibpurpleAdapter::USER_NOT_LOGGED_IN;
	}

	PurpleAccount* accountToSendFrom = s_onlineAccountData[accountKey];
	// strip off the "@aol.com" if needed
	std::string const& transportFriendlyUserName = Util::getPurpleUsername(serviceName, usernameTo);
	PurpleConversation* purpleConversation = purple_conversation_new(PURPLE_CONV_TYPE_IM, accountToSendFrom, transportFriendlyUserName.c_str());
	char* messageTextUnescaped = g_strcompress(messageText);

	// replace this with the lower level call so we can try to get an error code back...
	// calls common_send which calls serv_send_im (conversation.c)
//		purple_conv_im_send(purple_conversation_get_im_data(purpleConversation), messageTextUnescaped);
//				common_send(PurpleConversation *conv, const char *message, PurpleMessageFlags msgflags)
//					gc = purple_conversation_get_gc(conv);
//					err = serv_send_im(gc, purple_conversation_get_name(conv), sent, msgflags);
	// we still don't seem to get an error value back there...returns 1, even for an invalid recipient
	int err = serv_send_im(purple_conversation_get_gc(purpleConversation), purple_conversation_get_name(purpleConversation), messageTextUnescaped, (PurpleMessageFlags)0);

	if (err < 0) {
		MojLogError(IMServiceApp::s_log, _T("sendMessage: serv_send_im returned err %d"), err);
		free(messageTextUnescaped);
		return LibpurpleAdapter::SEND_FAILED;
	}

	free(messageTextUnescaped);
	return LibpurpleAdapter::SENT;
}


// Called by IMLoginState whenever a connection interface goes down.
// If all==true, then all interfaces went down.
bool LibpurpleAdapter::deviceConnectionClosed(bool all, const char* ipAddress)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!ipAddress)
	{
		return FALSE;
	}

	std::vector<std::string> accountToLogoutList;

	typedef std::unordered_map<std::string, std::string>::const_iterator iter;

	// s_ipAdressesBoundTo is abused as a joined store for online and pending
	// accounts
	for (iter i = s_ipAddressesBoundTo.begin(); i != s_ipAddressesBoundTo.end(); ++i)
	{
		std::string const& accountKey = i->first;

		std::string const& accountBoundToIpAddress = i->second;

		if (all == true || (accountBoundToIpAddress != "" && ipAddress == accountBoundToIpAddress))
		{
			bool accountWasLoggedIn = false;

			PurpleAccount* account;

			if (s_onlineAccountData.count(accountKey) == 0)
			{
				if (s_pendingAccountData.count(accountKey) == 0)
				{
					MojLogInfo(IMServiceApp::s_log, _T("account was not found in the hash"));
					continue;
				}
				
				account = s_pendingAccountData[accountKey];
				MojLogInfo(IMServiceApp::s_log, _T("Abandoning login"));
			}
			else
			{
				account = s_onlineAccountData[accountKey];
				accountWasLoggedIn = true;
				MojLogInfo(IMServiceApp::s_log, _T("Logging out"));
			}

			MojLogInfo(IMServiceApp::s_log, _T("deviceConnectionClosed: removing account from onlineAccountData hash table. accountKey %s"), accountKey.c_str());
			s_onlineAccountData.erase(accountKey);
			s_pendingAccountData.erase(accountKey);
			/*
			 * Keep the PurpleAccount struct to reuse in future logins
			 */
			s_offlineAccountData[accountKey] = account;

			purple_account_disconnect(account);

			accountToLogoutList.push_back(accountKey);

			MojLogInfo(IMServiceApp::s_log, _T("deviceConnectionClosed: removing account from onlineAccountData hash table. accountKey %s"), accountKey.c_str());
			// We can't remove this guy since we're iterating through its keys. We'll remove it after the break
			// g_hash_table_remove(ipAddressesBoundTo, accountKey);
		}
	}

	if (accountToLogoutList.empty())
	{
		MojLogInfo(IMServiceApp::s_log, _T("No accounts were connected on the requested ip address"));
	}
	else
	{
		for (std::vector<std::string>::iterator i = accountToLogoutList.begin();
				i != accountToLogoutList.end();
				++i)
		{
			s_ipAddressesBoundTo.erase(*i);
		}
	}

	return true;
}

void LibpurpleAdapter::assignIMLoginState(LoginCallbackInterface* loginState)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	static int handle = 0x1AD;
	s_loginState = loginState;

	if (s_registeredForAccountSignals == TRUE)
	{
		MojLogInfo(IMServiceApp::s_log, _T("Disconnecting old signals."));
		s_registeredForAccountSignals = FALSE;

		purple_signal_disconnect(purple_connections_get_handle(), "signed-on", &handle,
				PURPLE_CALLBACK(account_logged_in_cb));
		purple_signal_disconnect(purple_connections_get_handle(), "signed-off", &handle,
				PURPLE_CALLBACK(account_signed_off_cb));
		purple_signal_disconnect(purple_connections_get_handle(), "connection-error", &handle,
				PURPLE_CALLBACK(account_login_failed_cb));

		purple_signal_disconnect(purple_accounts_get_handle(), "account-status-changed", &handle,
				PURPLE_CALLBACK(account_status_changed));
		purple_signal_disconnect(purple_accounts_get_handle(), "account-authorization-denied", &handle,
				PURPLE_CALLBACK(account_auth_deny_cb));
		purple_signal_disconnect(purple_accounts_get_handle(), "account-authorization-granted", &handle,
				PURPLE_CALLBACK(account_auth_accept_cb));
	}

	if (loginState != NULL)
	{
		MojLogInfo(IMServiceApp::s_log, _T("Connecting new signals."));
		s_registeredForAccountSignals = TRUE;
		/*
		 * Listen for a number of different signals:
		 */
		purple_signal_connect(purple_connections_get_handle(), "signed-on", &handle,
				PURPLE_CALLBACK(account_logged_in_cb), loginState);
		purple_signal_connect(purple_connections_get_handle(), "signed-off", &handle,
				PURPLE_CALLBACK(account_signed_off_cb), loginState);
		purple_signal_connect(purple_connections_get_handle(), "connection-error", &handle,
				PURPLE_CALLBACK(account_login_failed_cb), loginState);

		// accounts signals
		purple_signal_connect(purple_accounts_get_handle(), "account-status-changed", &handle,
				PURPLE_CALLBACK(account_status_changed), loginState);
		purple_signal_connect(purple_accounts_get_handle(), "account-authorization-denied", &handle,
				PURPLE_CALLBACK(account_auth_deny_cb), loginState);
		purple_signal_connect(purple_accounts_get_handle(), "account-authorization-granted", &handle,
			   PURPLE_CALLBACK(account_auth_accept_cb), loginState);
	}
}

void LibpurpleAdapter::assignIMServiceHandler(IMServiceCallbackInterface* imServiceHandler)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	s_imServiceHandler = imServiceHandler;
}

/*
 * Value destroy function for s_AuthorizeRequests
 */
static void deleteAuthRequest(void* obj)
{
	AuthRequest *aa = (AuthRequest *)obj;
	MojLogInfo(IMServiceApp::s_log, _T("deleteAuthRequest: deleting auth request object. account: %s, remote_user: %s"), aa->account->username, aa->remote_user);
 	g_free(aa->remote_user);
	g_free(aa->alias);
	g_free(aa);
}

/*
 * Check if all the accounts are offline so we can shut down
 */
bool LibpurpleAdapter::allAccountsOffline()
{
	return s_onlineAccountData.empty() && s_pendingAccountData.empty();
}

void LibpurpleAdapter::init()
{
	initializeLibpurple();
}
