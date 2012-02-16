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
#include "LibpurpleAdapterPrefs.h"
#include "PalmImCommon.h"
#include "luna/MojLunaService.h"
#include "IMServiceApp.h"
#include "db/MojDbQuery.h"

static const guint PURPLE_GLIB_READ_COND  = (G_IO_IN | G_IO_HUP | G_IO_ERR);
static const guint PURPLE_GLIB_WRITE_COND = (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL);
//static const guint CONNECT_TIMEOUT_SECONDS = 45; Moved to preferences
//static const guint BUDDY_LIST_REFRESH = 10; Moved to preferences

static LoginCallbackInterface* s_loginState = NULL;
static IMServiceCallbackInterface* s_imServiceHandler = NULL;
static LibpurplePrefsHandler* s_PrefsHandler = NULL;

/**
 * List of accounts that are online
 */
static GHashTable* s_onlineAccountData = NULL;
/**
 * List of accounts that are in the process of logging in
 */
static GHashTable* s_pendingAccountData = NULL;
static GHashTable* s_offlineAccountData = NULL;
static GHashTable* s_accountLoginTimers = NULL;
static GHashTable* s_connectionTypeData = NULL;
static GHashTable* s_AccountIdsData = NULL;
static GHashTable* s_accountBuddyListTimers = NULL;

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
static GHashTable* s_ipAddressesBoundTo = NULL;

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

 /*
 * The messaging service expects the username to be in the username@domain.com format, whereas the AIM prpl uses the username only
 * Free the returned string when you're done with it 
 */
static char* getMojoFriendlyUsername(const char* username, const char* serviceName)
{
	if (!username || !serviceName)
	{
		return strdup("");
	}
	GString* mojoFriendlyUsername = g_string_new(username);
	if (strcmp(serviceName, SERVICENAME_AIM) == 0 && strchr(username, '@') == NULL)
	{
		g_string_append(mojoFriendlyUsername, "@aol.com");
	}
	else if (strcmp(serviceName, SERVICENAME_GTALK) == 0)
	{
		char* resource = (char*)memchr(username, '/', strlen(username));
		if (resource != NULL)
		{
			int charsToKeep = resource - username;
			g_string_erase(mojoFriendlyUsername, charsToKeep, -1);
		}
	}
	else if (strcmp(serviceName, SERVICENAME_JABBER) == 0)
	{
		if (strstr(username, "/") != NULL)
		{
			//If jabber resource is blank remove /
			char *resource = (char*)memchr(username, '/', strlen(username));
			if (resource != NULL)
			{
				int charsToKeep = resource - username;
				g_string_erase(mojoFriendlyUsername, charsToKeep, -1);
			}
		}
	}
	else if (strcmp(serviceName, SERVICENAME_SIPE) == 0)
	{
		char *resource = (char*)memchr(username, ',', strlen(username));
		if (resource != NULL)
		{
			int charsToKeep = resource - username;
			g_string_erase(mojoFriendlyUsername, charsToKeep, -1);
		}
	}
	char* mojoFriendlyUsernameToReturn = strdup(mojoFriendlyUsername->str);
	g_string_free(mojoFriendlyUsername, TRUE);
	return mojoFriendlyUsernameToReturn;
}

static char* getMojoFriendlyTemplateID (char* serviceName)
{
	if (strcmp(serviceName, SERVICENAME_AIM) == 0) {
		return (char*)CAPABILITY_AIM;
	}
	else if (strcmp(serviceName, SERVICENAME_FACEBOOK) == 0){
		return (char*)CAPABILITY_FACEBOOK;
	}
	else if (strcmp(serviceName, SERVICENAME_GTALK) == 0){
		return (char*)CAPABILITY_GTALK;
	}
	else if (strcmp(serviceName, SERVICENAME_GADU) == 0){
		return (char*)CAPABILITY_GADU;
	}
	else if (strcmp(serviceName, SERVICENAME_GROUPWISE) == 0){
		return (char*)CAPABILITY_GROUPWISE;
	}
	else if (strcmp(serviceName, SERVICENAME_ICQ) == 0){
		return (char*)CAPABILITY_ICQ;
	}
	else if (strcmp(serviceName, SERVICENAME_JABBER) == 0){
		return (char*)CAPABILITY_JABBER;
	}
	else if (strcmp(serviceName, SERVICENAME_LIVE) == 0){
		return (char*)CAPABILITY_LIVE;
	}
	else if (strcmp(serviceName, SERVICENAME_WLM) == 0){
		return (char*)CAPABILITY_WLM;
	}
	else if (strcmp(serviceName, SERVICENAME_MYSPACE) == 0){
		return (char*)CAPABILITY_MYSPACE;
	}
	else if (strcmp(serviceName, SERVICENAME_QQ) == 0){
		return (char*)CAPABILITY_QQ;
	}
	else if (strcmp(serviceName, SERVICENAME_SAMETIME) == 0){
		return (char*)CAPABILITY_SAMETIME;
	}
	else if (strcmp(serviceName, SERVICENAME_SIPE) == 0){
		return (char*)CAPABILITY_SIPE;
	}
	else if (strcmp(serviceName, SERVICENAME_XFIRE) == 0){
		return (char*)CAPABILITY_XFIRE;
	}
	else if (strcmp(serviceName, SERVICENAME_YAHOO) == 0){
		return (char*)CAPABILITY_YAHOO;
	}
	
	return (char*)"";
}

/*
 * This method handles special cases where the username passed by the mojo side does not satisfy a particular prpl's requirement
 * (e.g. for logging into AIM, the mojo service uses "palmpre@aol.com", yet the aim prpl expects "palmpre"; same scenario with yahoo)
 * Free the returned string when you're done with it 
 */
static char* getPrplFriendlyUsername(const char* serviceName, const char* username)
{
	if (!username || !serviceName)
	{
		return strdup("");
	}

	char* transportFriendlyUsername = strdup(username);
	if (strcmp(serviceName, SERVICENAME_AIM) == 0)
	{
		// Need to strip off @aol.com, but not @aol.com.mx
		const char* extension = strstr(username, "@aol.com");
		if (extension != NULL && strstr(extension, "aol.com.") == NULL)
		{
			strtok(transportFriendlyUsername, "@");
		}
	}
	
	const char* SIPEServerLogin;
	const char* JabberResource;
	char* templateId = getMojoFriendlyTemplateID((char*)serviceName);

	//Special Case for Office Communicator when DOMAIN\USER is set. Account name is USERNAME,DOMAIN\USER
	if (strcmp(serviceName, SERVICENAME_SIPE) == 0)
	{
		if (strstr(username, ",") != NULL)
		{
			//A "," exists in the sign in name already
			//transportFriendlyUsername = malloc(strlen(username) + 1);
			transportFriendlyUsername = strcpy(transportFriendlyUsername, username);
			return transportFriendlyUsername;
		}
		else
		{
			SIPEServerLogin = s_PrefsHandler->GetStringPreference("SIPEServerLogin", templateId, username);
			if (strcmp(SIPEServerLogin, "") != 0)
			{
				//transportFriendlyUsername = malloc(strlen(username) + 1);
				transportFriendlyUsername = strcpy(transportFriendlyUsername, username);

				//SIPE Account
				char *SIPEFullLoginName = NULL;
				SIPEFullLoginName = (char *)calloc(strlen(transportFriendlyUsername) + strlen(SIPEServerLogin) + 2, sizeof(char));
				strcat(SIPEFullLoginName, transportFriendlyUsername);
				strcat(SIPEFullLoginName, ",");
				strcat(SIPEFullLoginName, SIPEServerLogin);

				return SIPEFullLoginName;
			}
		}
	}

	//Special Case for jabber when resource is set. Account name is USERNAME/RESOURCE
	if (strcmp(serviceName, SERVICENAME_JABBER) == 0)
	{
		if (strstr(username, "/") != NULL)
		{
			//A "/" exists in the sign in name already
			transportFriendlyUsername = strcpy(transportFriendlyUsername, username);

			return transportFriendlyUsername;
		}
		else
		{
			JabberResource = s_PrefsHandler->GetStringPreference("JabberResource", templateId, username);
			
			if (strcmp(JabberResource, "") != 0)
			{
				transportFriendlyUsername = strcpy(transportFriendlyUsername, username);
				
				//Jabber Account
				char *JabberFullLoginName = NULL;
				JabberFullLoginName = (char *)calloc(strlen(transportFriendlyUsername) + strlen(JabberResource) + 2, sizeof(char));
				strcat(JabberFullLoginName, transportFriendlyUsername);
				strcat(JabberFullLoginName, "/");
				strcat(JabberFullLoginName, JabberResource);

				return JabberFullLoginName;
			}
		}
	}
	
	MojLogInfo(IMServiceApp::s_log, _T("getPrplFriendlyUsername: username: %s, transportFriendlyUsername: %s"), username, transportFriendlyUsername);
	return transportFriendlyUsername;
}

static char* stripResourceFromGtalkUsername(const char* username, const char* serviceName)
{
	if (!username)
	{
		return strdup("");
	}
	if ((strcmp(serviceName, SERVICENAME_GTALK) != 0) && (strcmp(serviceName, SERVICENAME_JABBER) != 0))
	{
		return strdup(username);
	}

	GString* mojoFriendlyUsername = g_string_new(username);
	char* resource = (char*)memchr(username, '/', strlen(username));
	if (resource != NULL)
	{
		int charsToKeep = resource - username;
		g_string_erase(mojoFriendlyUsername, charsToKeep, -1);
	}
	char* mojoFriendlyUsernameToReturn = strdup(mojoFriendlyUsername->str);
	g_string_free(mojoFriendlyUsername, TRUE);
	return mojoFriendlyUsernameToReturn;
}

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

/*
 * Given mojo-friendly serviceName, it will return prpl-specific protocol_id (e.g. given "type_aim", it will return "prpl-aim")
 * Free the returned string when you're done with it 
 */
static char* getPrplProtocolIdFromServiceName(const char* serviceName)
{
	if (!serviceName || serviceName[0] == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("getPrplProtocolIdFromServiceName called with empty serviceName"));
		return strdup("");
	}
	GString* prplProtocolId = g_string_new("prpl-");

	if ((strcmp(serviceName, SERVICENAME_GTALK) == 0) || (strcmp(serviceName, SERVICENAME_LIVE) == 0) || (strcmp(serviceName, SERVICENAME_FACEBOOK) == 0) || (strcmp(serviceName, SERVICENAME_SAMETIME) == 0) || (strcmp(serviceName, SERVICENAME_GROUPWISE) == 0) || (strcmp(serviceName, SERVICENAME_GADU) == 0) || (strcmp(serviceName, SERVICENAME_WLM) == 0))
	{
		if (strcmp(serviceName, SERVICENAME_GTALK) == 0)
		{
			// Special case for gtalk where the mojo serviceName is "type_gtalk" and the prpl protocol_id is "prpl-jabber"
			g_string_append(prplProtocolId, "jabber");
		}
		if (strcmp(serviceName, SERVICENAME_LIVE) == 0)
		{
			// Special case for live where the mojo serviceName is "type_live" and the prpl protocol_id is "prpl-msn"
			g_string_append(prplProtocolId, "msn");
		}
		if (strcmp(serviceName, SERVICENAME_WLM) == 0)
		{
			// Special case for live (pecan) where the mojo serviceName is "type_wlm" and the prpl protocol_id is "prpl-msn-pecan"
			g_string_append(prplProtocolId, "msn-pecan");
		}
		if (strcmp(serviceName, SERVICENAME_FACEBOOK) == 0)
		{
			// Special case for facebook where the mojo serviceName is "type_facebook" and the prpl protocol_id is "prpl-bigbrownchunx-facebookim"
			g_string_append(prplProtocolId, "bigbrownchunx-facebookim");
		}
		if (strcmp(serviceName, SERVICENAME_SAMETIME) == 0)
		{
			// Special case for sametime where the mojo serviceName is "type_sametime" and the prpl protocol_id is "prpl-meanwhile"
			g_string_append(prplProtocolId, "meanwhile");
		}
		if (strcmp(serviceName, SERVICENAME_GROUPWISE) == 0)
		{
			// Special case for groupwise where the mojo serviceName is "type_groupwise" and the prpl protocol_id is "prpl-novell"
			g_string_append(prplProtocolId, "novell");
		}
		if (strcmp(serviceName, SERVICENAME_GADU) == 0)
		{
			// Special case for gadu gadu where the mojo serviceName is "type_gadu" and the prpl protocol_id is "prpl-gg"
			g_string_append(prplProtocolId, "gg");
		}
	}
	else
	{
		const char* stringChopper = serviceName;
		stringChopper += strlen("type_");
		g_string_append(prplProtocolId, stringChopper);
	}
	char* prplProtocolIdToReturn = strdup(prplProtocolId->str);
	g_string_free(prplProtocolId, TRUE);
	return prplProtocolIdToReturn;
}

/*
 * Given the prpl-specific protocol_id, it will return mojo-friendly serviceName (e.g. given "prpl-aim", it will return "type_aim")
 * Free the returned string when you're done with it 
 */
static char* getServiceNameFromPrplProtocolId(PurpleAccount *account)
{
	char *prplProtocolId = account->protocol_id;
	if (!prplProtocolId)
	{
		MojLogError(IMServiceApp::s_log, _T("getServiceNameFromPrplProtocolId called with empty protocolId"));
		return strdup("type_default");
	}
	char* stringChopper = prplProtocolId;
	stringChopper += strlen("prpl-");
	GString* serviceName = g_string_new(stringChopper);

	if (strcmp(serviceName->str, "jabber") == 0)
	{
		// Special case for gtalk where the mojo serviceName is "type_gtalk" and the prpl protocol_id is "prpl-jabber"
		g_string_free(serviceName, TRUE);
		
		const char *Alias = purple_account_get_alias (account);

		if (Alias != NULL)
		{
			//Check account alias. gtalk for Gtalk, jabber for Jabber
			if (strcmp(Alias, "gtalk") == 0)
			{
				// Special case for gtalk where the java serviceName is "gmail" and the prpl protocol_id is "prpl-jabber"
				serviceName = g_string_new("gtalk");
			}
			else
			{
				// Special case for jabber where the java serviceName is "jabber" and the prpl protocol_id is "prpl-jabber"
				serviceName = g_string_new("jabber");
			}
		}
		else
		{
			//Account alias is blank for some reason. Guess gtalk
			serviceName = g_string_new("gtalk");
		}
	}
	if (strcmp(serviceName->str, "msn") == 0)
	{
		// Special case for live where the mojo serviceName is "type_live" and the prpl protocol_id is "prpl-msn"
		g_string_free(serviceName, TRUE);
		serviceName = g_string_new("live");
	}
        if (strcmp(serviceName->str, "msn-pecan") == 0)
        {
                // Special case for live where the mojo serviceName is "type_wlm" and the prpl protocol_id is "prpl-msn-pecan"
                g_string_free(serviceName, TRUE);
                serviceName = g_string_new("wlm");
        }
	if (strcmp(serviceName->str, "bigbrownchunx-facebookim") == 0)
	{
		// Special case for facebook where the mojo serviceName is "type_facebook" and the prpl protocol_id is "prpl-bigbrownchunx-facebookim"
		g_string_free(serviceName, TRUE);
		serviceName = g_string_new("facebook");
	}
	if (strcmp(serviceName->str, "meanwhile") == 0)
	{
		// Special case for sametime where the mojo serviceName is "type_sametime" and the prpl protocol_id is "prpl-meanwhile"
		g_string_free(serviceName, TRUE);
		serviceName = g_string_new("sametime");
	}
	if (strcmp(serviceName->str, "novell") == 0)
	{
		// Special case for groupwise where the mojo serviceName is "type_groupwise" and the prpl protocol_id is "prpl-novell"
		g_string_free(serviceName, TRUE);
		serviceName = g_string_new("groupwise");
	}
	if (strcmp(serviceName->str, "gg") == 0)
	{
		// Special case for gadu where the mojo serviceName is "type_gadu" and the prpl protocol_id is "prpl-gg"
		g_string_free(serviceName, TRUE);
		serviceName = g_string_new("gadu");
	}
	char* serviceNameToReturn = NULL;
	// asprintf allocates appropriate-sized buffer
	asprintf(&serviceNameToReturn, "type_%s", serviceName->str);
	g_string_free(serviceName, TRUE);
	return serviceNameToReturn;
}

static char* getAccountKey(const char* username, const char* serviceName)
{
	if (!username || !serviceName)
	{
		MojLogError(IMServiceApp::s_log, _T("getAccountKey called with username=\"%s\" and serviceName=\"%s\""), username, serviceName);
		return strdup("");
	}
	char *accountKey = NULL;
	// asprintf allocates appropriate-sized buffer
	asprintf(&accountKey, "%s_%s", username, serviceName);
	return accountKey;
}

static char* getAuthRequestKey(const char* username, const char* serviceName, const char* remoteUsername)
{
	if (!username || !serviceName || !remoteUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("getAuthRequestKey - empty input parameter. username: %s, serviceName: %s, remoteUsername: %s"), username, serviceName, remoteUsername);
		return strdup("");
	}
	char *authRequestKey = NULL;
	// asprintf allocates appropriate-sized buffer
	asprintf(&authRequestKey, "%s_%s_%s", username, serviceName, remoteUsername);
	MojLogInfo(IMServiceApp::s_log, _T("getAuthRequestKey - username: %s, serviceName: %s, remoteUser: %s key: %s"), username, serviceName, remoteUsername, authRequestKey);
	return authRequestKey;
}

static char* getAccountKeyFromPurpleAccount(PurpleAccount* account)
{
	if (!account)
	{
		MojLogError(IMServiceApp::s_log, _T("getAccountKeyFromPurpleAccount called with empty account"));
		return strdup("");
	}
	char* serviceName = getServiceNameFromPrplProtocolId(account);
	char* username = getMojoFriendlyUsername(account->username, serviceName);
	char* accountKey = getAccountKey(username, serviceName);

	free(serviceName);
	free(username);

	return accountKey;
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
	PurpleAccount* account;
	GList* onlineAccountKeys = NULL;
	GList* iterator = NULL;
	char* accountKey = NULL;
	
	onlineAccountKeys = g_hash_table_get_keys(s_onlineAccountData);
	for (iterator = onlineAccountKeys; iterator != NULL; iterator = g_list_next(iterator))
	{
		accountKey = (char*)iterator->data;
		account = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
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
	char* accountKey = getAccountKeyFromPurpleAccount(account);
	const char* accountId = (const char*)g_hash_table_lookup(s_AccountIdsData, accountKey);
	if (NULL == accountId) {
		MojLogError(IMServiceApp::s_log, _T("buddy_signed_on_off_cb: accountId not found in table. accountKey: %s"), accountKey);
		free(accountKey);
		return;
	}

	char* serviceName = getServiceNameFromPrplProtocolId(account);
	PurpleStatus* activeStatus = purple_presence_get_active_status(purple_buddy_get_presence(buddy));
	/*
	 * Getting the new availability
	 */
	int newStatusPrimitive = purple_status_type_get_primitive(purple_status_get_type(activeStatus));
	int newAvailabilityValue = getPalmAvailabilityFromPurpleAvailability(newStatusPrimitive);
	PurpleBuddyIcon* icon = purple_buddy_get_icon(buddy);
	const char* customMessage = "";
	char* buddyAvatarLocation = NULL;

	char* UserName = getMojoFriendlyUsername(account->username, serviceName);
	char* templateId = getMojoFriendlyTemplateID(serviceName);

	if (s_PrefsHandler->GetBoolPreference("Avatar", templateId, UserName) == true)
	{
		if (icon != NULL)
		{
			buddyAvatarLocation = purple_buddy_icon_get_full_path(icon);
		}
	}
	else
	{
		buddyAvatarLocation = (char*)"";
	}
	
	if (s_PrefsHandler->GetBoolPreference("Alias", templateId, UserName) == true)
	{
		if (buddy->server_alias == NULL)
		{
			buddy->server_alias = (char*)"";
		}
	}
	else
	{
		if (buddy->alias == NULL)
		{
			buddy->alias = (char*)"";
		}
	}

	customMessage = purple_status_get_attr_string(activeStatus, "message");
	if (customMessage == NULL)
	{
		customMessage = "";
	}

	PurpleGroup* group = purple_buddy_get_group(buddy);
	const char* groupName = purple_group_get_name(group);
	if (groupName == NULL)
	{
		groupName = "";
	}
	
	//Special for Office Communicator. (Remove 'sip:' prefix)
	if (strcmp(serviceName, SERVICENAME_SIPE) == 0)
	{
		if (strstr(buddy->name, "sip:") != NULL)
		{
			GString *SIPBuddyAlias = g_string_new(buddy->name);
			g_string_erase(SIPBuddyAlias, 0, 4);
			buddy->name = SIPBuddyAlias->str;
		}
	}

	// call into the imlibpurpletransport
	// buddy->name is stored in the imbuddyStatus DB kind in the libpurple format - ie. for AIM without the "@aol.com" so that is how we need to search for it
	s_imServiceHandler->updateBuddyStatus(accountId, serviceName, buddy->name, newAvailabilityValue,customMessage, groupName, buddyAvatarLocation);

	if (s_PrefsHandler->GetBoolPreference("Alias", templateId, UserName) == true)
	{
		g_message(
				"%s says: %s's presence: availability: '%i', custom message: '%s', avatar location: '%s', display name: '%s', group name: '%s'",
				__FUNCTION__, buddy->name, newAvailabilityValue, customMessage, buddyAvatarLocation, buddy->server_alias, groupName);
	}
	else
	{
			g_message(
				"%s says: %s's presence: availability: '%i', custom message: '%s', avatar location: '%s', display name: '%s', group name: '%s'",
				__FUNCTION__, buddy->name, newAvailabilityValue, customMessage, buddyAvatarLocation, buddy->alias, groupName);
	}
	
	free(serviceName);
	free(accountKey);

	/* if (buddyAvatarLocation)
	{
		g_free(buddyAvatarLocation);
	} */
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
	if (customMessage == NULL)
	{
		customMessage = "";
	}

	LSError lserror;
	LSErrorInit(&lserror);

	PurpleAccount* account = purple_buddy_get_account(buddy);
	char* accountKey = getAccountKeyFromPurpleAccount(account);
	const char* accountId = (const char*)g_hash_table_lookup(s_AccountIdsData, accountKey);
	if (NULL == accountId) {
		MojLogError(IMServiceApp::s_log, _T("buddy_status_changed_cb: accountId not found in table. accountKey: %s"), accountKey);
		free(accountKey);
		return;
	}
	char* serviceName = getServiceNameFromPrplProtocolId(account);

	PurpleBuddyIcon* icon = purple_buddy_get_icon(buddy);
	char* buddyAvatarLocation = NULL;
	
	char* UserName = getMojoFriendlyUsername(account->username, serviceName);
	char* templateId = getMojoFriendlyTemplateID(serviceName);
	
	if (s_PrefsHandler->GetBoolPreference("Avatar", templateId, UserName) == true)
	{
		if (icon != NULL)
		{
			buddyAvatarLocation = purple_buddy_icon_get_full_path(icon);
		}
	}
	else
	{
		buddyAvatarLocation = (char*)"";
	}

	PurpleGroup* group = purple_buddy_get_group(buddy);
	const char* groupName = purple_group_get_name(group);
	if (groupName == NULL)
	{
		groupName = "";
	}

	//Special for Office Communicator. (Remove 'sip:' prefix)
	if (strcmp(serviceName, SERVICENAME_SIPE) == 0)
	{
		if (strstr(buddy->name, "sip:") != NULL)
		{
			GString *SIPBuddyAlias = g_string_new(buddy->name);
			g_string_erase(SIPBuddyAlias, 0, 4);
			buddy->name = SIPBuddyAlias->str;
		}
	}
	
	// call into the imlibpurpletransport
	// buddy->name is stored in the imbuddyStatus DB kind in the libpurple format - ie. for AIM without the "@aol.com" so that is how we need to search for it
	s_imServiceHandler->updateBuddyStatus(accountId, serviceName, buddy->name, newAvailabilityValue, customMessage, groupName, buddyAvatarLocation);

	if (s_PrefsHandler->GetBoolPreference("Alias", templateId, UserName) == true)
	{
		g_message(
				"%s says: %s's presence: availability: '%i', custom message: '%s', avatar location: '%s', display name: '%s', group name: '%s'",
				__FUNCTION__, buddy->name, newAvailabilityValue, customMessage, buddyAvatarLocation, buddy->server_alias, groupName);
	}
	else
	{
		g_message(
			"%s says: %s's presence: availability: '%i', custom message: '%s', avatar location: '%s', display name: '%s', group name: '%s'",
			__FUNCTION__, buddy->name, newAvailabilityValue, customMessage, buddyAvatarLocation, buddy->alias, groupName);
	}
	
	if (serviceName)
	{
		free(serviceName);
	}
	free(accountKey);

	/* if (buddyAvatarLocation)
	{
		g_free(buddyAvatarLocation);
	} */
}

static void buddy_avatar_changed_cb(PurpleBuddy* buddy)
{
	PurpleStatus* activeStatus = purple_presence_get_active_status(purple_buddy_get_presence(buddy));
	MojLogInfo(IMServiceApp::s_log, _T("buddy avatar changed for %s"), buddy->name);
	buddy_status_changed_cb(buddy, activeStatus, activeStatus, NULL);
}

/*
 * Called after we remove a buddy from out list
 */
static void buddy_removed_cb(PurpleBuddy* buddy)
{
	// nothing to do...
	MojLogInfo(IMServiceApp::s_log, _T("buddy removed %s"), buddy->name);
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

// testing. Never gets called...
//static void sent_message_cb (PurpleAccount *account, const char *receiver, const char *message)
//{
//	MojLogInfo(IMServiceApp::s_log, _T("im message sent: %s, receiver %s"), message, receiver);
//}

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
	char* accountKey = getAccountKey(username, serviceName);
	PurpleAccount* account = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	if (account == NULL)
	{
		MojLogError(IMServiceApp::s_log, _T("getFullBuddyList: ERROR: No account for %s on %s"), username, serviceName);
		MojLogError(IMServiceApp::s_log, _T("getFullBuddyList !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! accountKey: %s"), accountKey);
		success = FALSE;
	}
	else
	{
		success = TRUE;
		GSList* buddyList = purple_find_buddies(account, NULL);
		MojObject buddyListObj;
		if (!buddyList)
		{
			MojLogError(IMServiceApp::s_log, _T("getFullBuddyList: WARNING: the buddy list was NULL, returning empty buddy list. accountKey %s"), accountKey);
			s_loginState->buddyListResult(serviceName, username, buddyListObj, true);
		}

		GSList* buddyIterator = NULL;
		PurpleBuddy* buddyToBeAdded = NULL;
		PurpleGroup* group = NULL;
		char* buddyAvatarLocation = NULL;

		//printf("\n\n\n\n\n\n ---------------- BUDDY LIST SIZE: %d----------------- \n\n\n\n\n\n", g_slist_length(buddyList));
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

			char* templateId = getMojoFriendlyTemplateID((char*)serviceName);
			
			//Special for Office Communicator. (Remove 'sip:' prefix)
			if (strcmp(serviceName, SERVICENAME_SIPE) == 0)
			{
				if (strstr(buddyToBeAdded->name, "sip:") != NULL)
				{
					GString *SIPBuddyAlias = g_string_new(buddyToBeAdded->name);
					g_string_erase(SIPBuddyAlias, 0, 4);
					buddyObj.putString("displayName", SIPBuddyAlias->str);
				}
				else
				{
					buddyObj.putString("displayName", buddyToBeAdded->name);
				}
			}
			else
			{
				if (s_PrefsHandler->GetBoolPreference("Alias", templateId, username) == true)
				{
					if (buddyToBeAdded->server_alias != NULL)
					{
						buddyObj.putString("displayName", buddyToBeAdded->server_alias);
					}
				}
				else
				{
					if (buddyToBeAdded->alias != NULL)
					{
						buddyObj.putString("displayName", buddyToBeAdded->alias);
					}
				}
			}

			PurpleBuddyIcon* icon = purple_buddy_get_icon(buddyToBeAdded);

			if (s_PrefsHandler->GetBoolPreference("Avatar", templateId, username) == true)
			{
				if (icon != NULL)
				{
					buddyAvatarLocation = purple_buddy_icon_get_full_path(icon);
					buddyObj.putString("avatar", buddyAvatarLocation);
				}
				else
				{
					buddyObj.putString("avatar", "");
				}
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

			if (s_PrefsHandler->GetBoolPreference("Alias", templateId, username) == true)
			{
				g_message("%s says: %s's presence: availability: '%d', custom message: '%s', avatar location: '%s', display name: '%s', group name:'%s'",
						__FUNCTION__, buddyToBeAdded->name, availability, customMessage, buddyAvatarLocation, buddyToBeAdded->server_alias, groupName);
			}
			else
			{
				g_message("%s says: %s's presence: availability: '%d', custom message: '%s', avatar location: '%s', display name: '%s', group name:'%s'",
						__FUNCTION__, buddyToBeAdded->name, availability, customMessage, buddyAvatarLocation, buddyToBeAdded->alias, groupName);
			}
			
			if (buddyAvatarLocation)
			{
				g_free(buddyAvatarLocation);
				buddyAvatarLocation = NULL;
			}

			buddyListObj.push(buddyObj);
		}

		s_loginState->buddyListResult(serviceName, username, buddyListObj, true);

		//TODO free the buddyList object???
	}

	if (accountKey)
	{
		free(accountKey);
	}
	return success;
}

typedef struct _LoginDetails
{
	PurpleAccount* account;
	char* accountKey;
	char* serviceName;
	char* username;
	gpointer loginState;
} LoginDetails;

gboolean BuddyListRefreshCallback(gpointer data)
{
	LoginDetails *logindetails = (LoginDetails*)data;

	void* blist_handle = purple_blist_get_handle();
	static int handle;
	gpointer loginState = logindetails->loginState;
	
	char* accountKey = logindetails->accountKey;
	PurpleAccount* loggedInAccount = (PurpleAccount*)logindetails->account;
	char* serviceName = logindetails->serviceName;
	char* username = logindetails->username;

	if (g_hash_table_lookup(s_onlineAccountData, accountKey) != NULL)
	{
		/*
		 * If the account is not pending anymore (which means login either already failed or succeeded) 
		 * then we shouldn't have gotten to this point since we should have cancelled the timer
		 */
		MojLogError(IMServiceApp::s_log,
				_T("WARNING: we shouldn't have gotten to BuddyListRefreshCallback since the account is not logged in. accountKey %s"), accountKey);
		free(accountKey);
		return TRUE;
	}

	/*
	 * Remove the timer
	 */
	guint BuddyListtimerHandle = (guint)g_hash_table_lookup(s_accountBuddyListTimers, accountKey);
	purple_timeout_remove(BuddyListtimerHandle);
	g_hash_table_remove(s_accountBuddyListTimers, accountKey);
	
	// Don't free accountKey because s_onlineAccountData has a reference to it.
	MojLogInfo(IMServiceApp::s_log, _T("account_logged_in_cb: inserting account into onlineAccountData hash table. accountKey %s"), accountKey);
	g_hash_table_insert(s_onlineAccountData, accountKey, loggedInAccount);
	g_hash_table_remove(s_pendingAccountData, accountKey);

	MojLogInfo(IMServiceApp::s_log, _T("Account connected..."));

	//reply with login success
	if (loginState)
	{
		((LoginCallbackInterface*)loginState)->loginResult(serviceName, username, LoginCallbackInterface::LOGIN_SUCCESS, false, ERROR_NO_ERROR, false);
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

		// testing. Doesn't work: error - "Signal data for sent-im-msg not found". Need to figure out the right handle
//		purple_signal_connect(purple_connections_get_handle(), "sent-im-msg", &handle, PURPLE_CALLBACK(sent_message_cb),
//						GINT_TO_POINTER(FALSE));
		s_registeredForPresenceUpdateSignals = TRUE;
	}
	
	//Refresh the full buddy list
	MojLogError(IMServiceApp::s_log, _T("BuddyListRefreshCallback accountKey: %s"), accountKey);
	
	return TRUE;
}

static void account_logged_in_cb(PurpleConnection* gc, gpointer loginState)
{
	PurpleAccount* loggedInAccount = purple_connection_get_account(gc);
	g_return_if_fail(loggedInAccount != NULL);

	char* serviceName = getServiceNameFromPrplProtocolId(loggedInAccount);
	char* username = getMojoFriendlyUsername(loggedInAccount->username, serviceName);
	char* accountKey = getAccountKey(username, serviceName);

	if (g_hash_table_lookup(s_onlineAccountData, accountKey) != NULL)
	{
		// we were online. why are we getting notified that we're connected again? we were never disconnected.
		// mark the account online just to be sure?
		MojLogError(IMServiceApp::s_log, _T("account_logged_in_cb: account already online. why are we getting notified? accountKey %s"), accountKey);
		free(serviceName);
		free(username);
		free(accountKey);
		return;
	}

	/*
	 * cancel the connect timeout for this account
	 */
	guint timerHandle = (guint)g_hash_table_lookup(s_accountLoginTimers, accountKey);
	purple_timeout_remove(timerHandle);
	g_hash_table_remove(s_accountLoginTimers, accountKey);
	
	//Create timer to refresh the buddy list after login
	LoginDetails* logindetails = g_new0(LoginDetails, 1);
	
	logindetails->accountKey = accountKey;
	logindetails->serviceName = serviceName;
	logindetails->username = username;
	logindetails->loginState = loginState;
	logindetails->account = loggedInAccount;
	
	char* templateId = getMojoFriendlyTemplateID((char*)serviceName);
	char* BuddyListTimeOut = s_PrefsHandler->GetStringPreference("BuddyListTimeOut", templateId, username);
	static const guint BUDDY_LIST_REFRESH = (guint)atoi(BuddyListTimeOut);
	guint BuddyListtimerHandle = purple_timeout_add_seconds(BUDDY_LIST_REFRESH, BuddyListRefreshCallback, logindetails);
	g_hash_table_insert(s_accountBuddyListTimers, accountKey, (gpointer)BuddyListtimerHandle);
	
	MojLogError(IMServiceApp::s_log, _T("Login successful. Waiting %s seconds to ensure successful buddy list retrieval."),BuddyListTimeOut);
}

static void account_signed_off_cb(PurpleConnection* gc, gpointer loginState)
{
	PurpleAccount* account = purple_connection_get_account(gc);
	g_return_if_fail(account != NULL);

	char* accountKey = getAccountKeyFromPurpleAccount(account);
	if (g_hash_table_lookup(s_onlineAccountData, accountKey) != NULL)
	{
		MojLogInfo(IMServiceApp::s_log, _T("account_signed_off_cb: removing account from onlineAccountData hash table. accountKey %s"), accountKey);
		g_hash_table_remove(s_onlineAccountData, accountKey);
	}
	else if (g_hash_table_lookup(s_pendingAccountData, accountKey) != NULL)
	{
		g_hash_table_remove(s_pendingAccountData, accountKey);
	}
	else
	{
		// Already signed off this account (or never signed in) so just return
		free(accountKey);
		return;
	}

	g_hash_table_remove(s_ipAddressesBoundTo, accountKey);
	//g_hash_table_remove(connectionTypeData, accountKey);

	MojLogInfo(IMServiceApp::s_log, _T("Account disconnected..."));

	if (g_hash_table_lookup(s_offlineAccountData, accountKey) == NULL)
	{
		/*
		 * Keep the PurpleAccount struct to reuse in future logins
		 */
		g_hash_table_insert(s_offlineAccountData, accountKey, account);
	}
	
	// reply with signed off
	if (loginState)
	{
		char* serviceName = getServiceNameFromPrplProtocolId(account);
		char* myMojoFriendlyUsername = getMojoFriendlyUsername(account->username, serviceName);
		((LoginCallbackInterface*)loginState)->loginResult(serviceName, myMojoFriendlyUsername, LoginCallbackInterface::LOGIN_SIGNED_OFF, false, ERROR_NO_ERROR, true);
		free(serviceName);
		free(myMojoFriendlyUsername);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, _T("ERROR: account_logged_in_cb called with loginState=NULL"));
	}

	free(accountKey);
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
	char* serviceName = getServiceNameFromPrplProtocolId(account);
	char* username = getMojoFriendlyUsername(account->username, serviceName);
	char* accountKey = getAccountKey(username, serviceName);

	if (g_hash_table_lookup(s_onlineAccountData, accountKey) != NULL)
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
		guint timerHandle = (guint)g_hash_table_lookup(s_accountLoginTimers, accountKey);
		purple_timeout_remove(timerHandle);
		g_hash_table_remove(s_accountLoginTimers, accountKey);
		
		//Cancel the buddy list timer
		guint BuddyListtimerHandle = (guint)g_hash_table_lookup(s_accountBuddyListTimers, accountKey);
		purple_timeout_remove(BuddyListtimerHandle);
		g_hash_table_remove(s_accountBuddyListTimers, accountKey);

		if (g_hash_table_lookup(s_pendingAccountData, accountKey) == NULL)
		{
			/*
			 * This account was in neither of the account data lists (online or pending). We must have logged it out 
			 * and not cared about letting mojo know about it (probably because mojo went down and came back up and
			 * thought that the account was logged out anyways)
			 */
			MojLogError(IMServiceApp::s_log, _T("account_login_failed_cb: account in neither online or pending list. Why are we getting logged out? accountKey: %s description %s:"),
					accountKey, description);
			// don't leak!
			free(serviceName);
			free(username);
			free(accountKey);
			return;
		}
		else
		{
			g_hash_table_remove(s_pendingAccountData, accountKey);
		}
	}

	const char* mojoFriendlyErrorCode = getMojoFriendlyErrorCode(type);
	const char* accountBoundToIpAddress = (char*)g_hash_table_lookup(s_ipAddressesBoundTo, accountKey);
	const char* connectionType = (char*)g_hash_table_lookup(s_connectionTypeData, accountKey);

	if (accountBoundToIpAddress == NULL)
	{
		accountBoundToIpAddress = "";
	}

	if (connectionType == NULL)
	{
		connectionType = "";
	}
	MojLogInfo(IMServiceApp::s_log, _T("account_login_failed_cb: removing account from onlineAccountData hash table. accountKey %s"), accountKey);
	g_hash_table_remove(s_onlineAccountData, accountKey);
	g_hash_table_remove(s_ipAddressesBoundTo, accountKey);
	g_hash_table_remove(s_connectionTypeData, accountKey);
 
	if (g_hash_table_lookup(s_offlineAccountData, accountKey) == NULL)
	{
		/*
		 * Keep the PurpleAccount struct to reuse in future logins
		 */
		g_hash_table_insert(s_offlineAccountData, accountKey, account);
		// don't free the accountKey in this case since now s_offlineAccountData points to it
		accountKey = NULL;
	}
	
	// reply with login failed
	if (loginState != NULL)
	{
		//TODO: determine if there are cases where noRetry should be false
		//TODO: include "description" parameter because it had useful details?
		((LoginCallbackInterface*)loginState)->loginResult(serviceName, username, LoginCallbackInterface::LOGIN_FAILED, loggedOut, mojoFriendlyErrorCode, noRetry);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, _T("ERROR: account_login_failed_cb called with loginState=NULL"));
	}
	free(serviceName);
	free(username);
	if (NULL != accountKey)
		free(accountKey);
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
//	char* serviceName = getServiceNameFromPrplProtocolId(account);
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
	if (who &&* who)
		usernameFrom = who;
	else if (alias &&* alias)
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
	char* serviceName = getServiceNameFromPrplProtocolId(account);
	char* username = getMojoFriendlyUsername(account->username, serviceName);

	if (strcmp(username, usernameFrom) == 0) // TODO: should this compare use account->username?
	{
		/* We get notified even though we sent the message. Just ignore it */
		free(serviceName);
		free(username);
		return;
	}

	if (strcmp(serviceName, SERVICENAME_AIM) == 0 && (strcmp(usernameFrom, "aolsystemmsg") == 0 || strcmp(usernameFrom,
			"AOL System Msg") == 0))
	{
		/*
		 * ignore messages from aolsystemmsg telling us that we're logged in somewhere else
		 */
		free(serviceName);
		free(username);
		return;
	}

	char* usernameFromStripped = stripResourceFromGtalkUsername(usernameFrom, serviceName);
	
	//Special for Office Communicator. (Remove 'sip:' prefix)
	if (strcmp(serviceName, SERVICENAME_SIPE) == 0)
	{
		if (strstr(usernameFromStripped, "sip:") != NULL)
		{
			GString *SIPBuddyAlias = g_string_new(usernameFromStripped);
			g_string_erase(SIPBuddyAlias, 0, 4);
			usernameFromStripped = (char*)SIPBuddyAlias->str;
		}
	}

	// call the transport service incoming message handler
	s_imServiceHandler->incomingIM(serviceName, username, usernameFromStripped, message);

	free(serviceName);
	free(username);
	free(usernameFromStripped);
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
	char* serviceName = getServiceNameFromPrplProtocolId(account);
	char* username = getMojoFriendlyUsername(account->username, serviceName);
	char* usernameFromStripped = stripResourceFromGtalkUsername(remote_user, serviceName);

	// Save off the authorize/deny callbacks to use later
	AuthRequest *aa = g_new0(AuthRequest, 1);
	aa->auth_cb = authorize_cb;
	aa->deny_cb = deny_cb;
	aa->data = user_data;
	aa->remote_user = g_strdup(remote_user);
	aa->alias = g_strdup(alias);
	aa->account = account;

	char *authRequestKey = getAuthRequestKey(username, serviceName, usernameFromStripped);
	// if there is already an entry for this, we need to replace it since callback function pointers will change on login
	// old object gets deleted by our destroy functions specified in the hash table construction
	g_hash_table_replace(s_AuthorizeRequests, authRequestKey, aa);
	// log the table
	logAuthRequestTableValues();

	// call back into IMServiceHandler to create a receivedBuddyInvite imCommand.
	s_imServiceHandler->receivedBuddyInvite(serviceName, username, usernameFromStripped, message);

	free(serviceName);
	free(username);
	free(usernameFromStripped);
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
	char* accountKey = (char*)data;
	PurpleAccount* account = (PurpleAccount*)g_hash_table_lookup(s_pendingAccountData, accountKey);
	if (account == NULL)
	{
		/*
		 * If the account is not pending anymore (which means login either already failed or succeeded) 
		 * then we shouldn't have gotten to this point since we should have cancelled the timer
		 */
		MojLogError(IMServiceApp::s_log,
				_T("WARNING: we shouldn't have gotten to connectTimeoutCallback since login had already failed/succeeded. accountKey %s"), accountKey);
		free(accountKey);
		return FALSE;
	}

	/*
	 * abort logging in since our connect timeout has hit before login either failed or succeeded
	 */
	g_hash_table_remove(s_accountLoginTimers, accountKey);
	g_hash_table_remove(s_pendingAccountData, accountKey);
	g_hash_table_remove(s_ipAddressesBoundTo, accountKey);
	g_hash_table_remove(s_accountBuddyListTimers, accountKey);

	purple_account_disconnect(account);

	if (s_loginState)
	{
		char* serviceName = getServiceNameFromPrplProtocolId(account);
		char* username = getMojoFriendlyUsername(account->username, serviceName);

		s_loginState->loginResult(serviceName, username, LoginCallbackInterface::LOGIN_TIMEOUT, false, ERROR_NETWORK_ERROR, true);

		free(serviceName);
		free(username);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, _T("ERROR: connectTimeoutCallback called with s_loginState=NULL. accountKey %s"), accountKey);
	}

	free(accountKey);
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
	
	/* Set path to search for plugins. The core (libpurple) takes care of loading the
	* core-plugins, which includes the protocol-plugins. So it is not essential to add
	* any path here, but it might be desired, especially for ui-specific plugins. */
	purple_plugins_add_search_path(CUSTOM_PLUGIN_PATH);

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
LibpurpleAdapter::LoginResult LibpurpleAdapter::login(LoginParams* params, LoginCallbackInterface* loginState)
{	
	LoginResult result = LibpurpleAdapter::OK;

	PurpleAccount* account;
	char* prplProtocolId = NULL;
	char* transportFriendlyUserName = NULL;
	char* accountKey = NULL;
	bool accountIsAlreadyOnline = FALSE;
	bool accountIsAlreadyPending = FALSE;

	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	PurpleAccount* alreadyActiveAccount = NULL;

	if (!params || !params->serviceName || params->serviceName[0] == 0 || !params->username || params->username[0] == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login with empty username or serviceName"));
		result = LibpurpleAdapter::INVALID_CREDENTIALS;
		goto error;
	}

	if (!params->localIpAddress)
	{
		params->localIpAddress = "";
	}
	
	if (!params->connectionType)
	{
		params->connectionType = "";
	}

	MojLogInfo(IMServiceApp::s_log, _T("Parameters: accountId %s, servicename %s, connectionType %s"), params->accountId, params->serviceName, params->connectionType);

	if (s_libpurpleInitialized == FALSE)
	{
		initializeLibpurple();
	}

	/* libpurple variables */
	accountKey = getAccountKey(params->username, params->serviceName);

	// If this account id isn't yet stored, then keep track of it now.
	if (g_hash_table_lookup(s_AccountIdsData, accountKey) == NULL)
	{
		g_hash_table_insert(s_AccountIdsData, accountKey, strdup(params->accountId));
	}

	/*
	 * Let's check to see if we're already logged in to this account or that we're already in the process of logging in 
	 * to this account. This can happen when mojo goes down and comes back up.
	 */
	alreadyActiveAccount = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	if (alreadyActiveAccount != NULL)
	{
		accountIsAlreadyOnline = TRUE;
	}
	else
	{
		alreadyActiveAccount = (PurpleAccount*)g_hash_table_lookup(s_pendingAccountData, accountKey);
		if (alreadyActiveAccount != NULL)
		{
			accountIsAlreadyPending = TRUE;
		}
	}

	if (alreadyActiveAccount != NULL)
	{
		/*
		 * We're either already logged in to this account or we're already in the process of logging in to this account 
		 * (i.e. it's pending; waiting for server response)
		 */
		char* accountBoundToIpAddress = (char*)g_hash_table_lookup(s_ipAddressesBoundTo, accountKey);
		if (accountBoundToIpAddress != NULL && strcmp(params->localIpAddress, accountBoundToIpAddress) == 0)
		{
			/*
			 * We're using the right interface for this account
			 */
			if (accountIsAlreadyPending)
			{
				MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login: We were already in the process of logging in. accountKey %s"), accountKey);
				return LibpurpleAdapter::OK;
			}
			else if (accountIsAlreadyOnline)
			{
				MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login: We were already logged in to the requested account. accountKey %s"), accountKey);
				return LibpurpleAdapter::ALREADY_LOGGED_IN;
			}
		}
		else
		{
			/*
			 * We're not using the right interface. Close the current connection for this account and create a new one
			 */
			MojLogError(IMServiceApp::s_log,
					_T("LibpurpleAdapter::login: We have to logout and login again since the local IP address has changed. Logging out from account. accountKey %s"), accountKey);
			/*
			 * Once the current connection is closed we don't want to let mojo know that the account was disconnected.
			 * Since mojo went down and came back up it didn't know that the account was connected anyways.
			 * So let's take the account out of the account data hash and then disconnect it.
			 */
			if (g_hash_table_lookup(s_onlineAccountData, accountKey) != NULL)
			{
				MojLogInfo(IMServiceApp::s_log, _T("LibpurpleAdapter::login: removing account from onlineAccountData hash table. accountKey %s"), accountKey);
				g_hash_table_remove(s_onlineAccountData, accountKey);
			}
			if (g_hash_table_lookup(s_pendingAccountData, accountKey) != NULL)
			{
				g_hash_table_remove(s_pendingAccountData, accountKey);
			}
			purple_account_disconnect(alreadyActiveAccount);
		}
	}

	/*
	 * Let's go through our usual login process
	 */

	// TODO this currently ignores authentication token, but should check it as well when support for auth token is added
	if (params->password == NULL || params->password[0] == 0)
	{
		MojLogError(IMServiceApp::s_log, _T("Error: null or empty password trying to log in to servicename %s"), params->serviceName);
		result = LibpurpleAdapter::INVALID_CREDENTIALS;
		goto error;
	}
	else
	{
		/* save the local IP address that we need to use */
		// TODO - move this to #ifdef. If you are running imlibpurpletransport on desktop, but tethered to device, params->localIpAddress needs to be set to
		// NULL otherwise login will fail...
		if (params->localIpAddress != NULL && params->localIpAddress[0] != 0)
		{
			purple_prefs_remove("/purple/network/preferred_local_ip_address");
			purple_prefs_add_string("/purple/network/preferred_local_ip_address", params->localIpAddress);
		}
		else
		{
#ifdef DEVICE
			/*
			 * If we're on device you should not accept an empty ipAddress; it's mandatory to be provided
			 */
			MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login with missing localIpAddress"));
			result = FAILED;
			goto error;
#endif
		}

		/* save the local IP address that we need to use */
		if (params->connectionType != NULL && params->connectionType[0] != 0)
		{
			g_hash_table_insert(s_connectionTypeData, accountKey, strdup(params->connectionType));
		}

		prplProtocolId = getPrplProtocolIdFromServiceName(params->serviceName);
		/*
		 * If we've already logged in to this account before then re-use the old PurpleAccount struct
		 */
		transportFriendlyUserName = getPrplFriendlyUsername(params->serviceName, params->username);
		account = (PurpleAccount*)g_hash_table_lookup(s_offlineAccountData, accountKey);
		if (!account)
		{
			/* Create the account */
			account = purple_account_new(transportFriendlyUserName, prplProtocolId);
			if (!account)
			{
				MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::login failed to create new Purple account"));
				result = LibpurpleAdapter::FAILED;
				goto error;
			}
		}

		//Load account preferences
		char* templateId = getMojoFriendlyTemplateID((char*)params->serviceName);
		
		if (strcmp((char*)params->serviceName, SERVICENAME_JABBER) == 0)
		{
			//Set Account Alias to jabber
			purple_account_set_alias (account,"jabber");
		}
		if (strcmp((char*)params->serviceName,SERVICENAME_GTALK) == 0)
		{
			//Set Account Alias to gtalk
			purple_account_set_alias (account,"gtalk");
		}
		
		MojString m_templateId;
		m_templateId.assign(templateId);
		s_PrefsHandler->setaccountprefs(m_templateId,account);
		//Load account preferences
		MojLogInfo(IMServiceApp::s_log, _T("Logging in..."));

		purple_account_set_password(account, params->password);
	}

	if (result == LibpurpleAdapter::OK)
	{
		/* mark the account as pending */
		g_hash_table_insert(s_pendingAccountData, accountKey, account);

		if (params->localIpAddress != NULL && params->localIpAddress[0] != 0)
		{
			/* keep track of the local IP address that we bound to when logging in to this account */
			g_hash_table_insert(s_ipAddressesBoundTo, accountKey, strdup(params->localIpAddress));
		}

		/* It's necessary to enable the account first. */
		purple_account_set_enabled(account, UI_ID, TRUE);

		/* Now, to connect the account, create a status and activate it. */

		/*
		 * Create a timer for this account's login so it can fail the login after a timeout.
		 */
		char* UserName = getMojoFriendlyUsername(params->username, params->serviceName);
		char* templateId = getMojoFriendlyTemplateID((char*)params->serviceName);
		static const guint CONNECT_TIMEOUT_SECONDS = (guint)atoi(s_PrefsHandler->GetStringPreference("LoginTimeOut", templateId, UserName));
		guint timerHandle = purple_timeout_add_seconds(CONNECT_TIMEOUT_SECONDS, connectTimeoutCallback, accountKey);
		g_hash_table_insert(s_accountLoginTimers, accountKey, (gpointer)timerHandle);

		PurpleStatusPrimitive prim = getPurpleAvailabilityFromPalmAvailability(params->availability);
		PurpleSavedStatus* savedStatus = purple_savedstatus_new(NULL, prim);
		if (params->customMessage && params->customMessage[0])
		{
			purple_savedstatus_set_message(savedStatus, params->customMessage);
		}
		purple_savedstatus_activate_for_account(savedStatus, account);
	}

	error:

	if (prplProtocolId)
	{
		free(prplProtocolId);
	}
	if (transportFriendlyUserName)
	{
		free(transportFriendlyUserName);
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

	bool success = TRUE;

	MojLogInfo(IMServiceApp::s_log, _T("Parameters: servicename %s"), serviceName);

	char* accountKey = getAccountKey(username, serviceName);

	// Remove the accountId since a logout could be from the user removing the account
	g_hash_table_remove(s_AccountIdsData, accountKey);

	PurpleAccount* accountTologoutFrom = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	if (accountTologoutFrom == NULL)
	{
		accountTologoutFrom = (PurpleAccount*)g_hash_table_lookup(s_pendingAccountData, accountKey);
		if (accountTologoutFrom == NULL)
		{
			MojLogError(IMServiceApp::s_log, _T("Trying to logout from an account that is not logged in. username %s, service name %s, accountKey %s"), username, serviceName, accountKey);
			success = FALSE;
		}
	}

	if (accountTologoutFrom != NULL)
	{
		purple_account_disconnect(accountTologoutFrom);
	}

	if (accountKey)
	{
		free(accountKey);
	}
	return success;
}

bool LibpurpleAdapter::setMyAvailability(const char* serviceName, const char* username, int availability)
{
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName || !username)
	{
		MojLogError(IMServiceApp::s_log, _T("setMyAvailability: Invalid parameter. Please double check the passed parameters."));
		return FALSE;
	}

	MojLogInfo(IMServiceApp::s_log, _T("Parameters: serviceName %s, availability %i"), serviceName, availability);

	bool retVal = FALSE;
	char* accountKey = getAccountKey(username, serviceName);
	PurpleAccount* account = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	if (account == NULL)
	{
		//this should never happen based on MessagingService's logic
		MojLogError(IMServiceApp::s_log,
				_T("setMyAvailability was called on an account that wasn't logged in. serviceName: %s, availability: %i"),
				serviceName, availability);
		retVal = FALSE;
	}
	else
	{
		retVal = TRUE;
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
	}

	// delete the key since it was just for lookup
	free(accountKey);
	return retVal;
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

	bool retVal = FALSE;
	char* accountKey = getAccountKey(username, serviceName);
	PurpleAccount* account = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	if (account == NULL)
	{
		//this should never happen based on MessagingService's logic
		MojLogError(IMServiceApp::s_log,
				_T("setMyCustomMessage was called on an account that wasn't logged in. serviceName: %s"),
				serviceName);
		retVal = FALSE;
	}
	else
	{
		retVal = TRUE;
		// get the account's current status type
		PurpleStatusType* type = purple_status_get_type(purple_account_get_active_status(account));
		GList* attrs = NULL;
		attrs = g_list_append(attrs, (void*)"message");
		attrs = g_list_append(attrs, (char*)customMessage);
		purple_account_set_status_list(account, purple_status_type_get_id(type), TRUE, attrs);
	}

	// delete the key since it was just for lookup
	free(accountKey);
	return retVal;
}

/*
 * Block this user from sending us messages
 */
LibpurpleAdapter::SendResult LibpurpleAdapter::blockBuddy(const char* serviceName, const char* username, const char* buddyUsername, bool block)
{
	bool success = FALSE;
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName || !username || !buddyUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("blockBuddy: null parameter. username %s, service name %s, buddyUsername %s"), username, serviceName, buddyUsername);
		return INVALID_PARAMS;
	}

	char* accountKey = getAccountKey(username, serviceName);
	PurpleAccount* account = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	success = (account != NULL);
	if (success)
	{
		// strip off the "@aol.com" if needed
		char *transportFriendlyUserName = getPrplFriendlyUsername(serviceName, buddyUsername);
		if (block)
		{
			MojLogInfo(IMServiceApp::s_log, _T("blockBuddy: deny %s"), transportFriendlyUserName);
			purple_privacy_deny(account, transportFriendlyUserName, false, true);
		}
		else
		{
			MojLogInfo(IMServiceApp::s_log, _T("blockBuddy: allow %s"), transportFriendlyUserName);
			purple_privacy_allow(account, transportFriendlyUserName, false, true);
		}
		free(transportFriendlyUserName);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, "blockBuddy: Trying to send from an account that is not logged in. username %s, service name %s, accountKey %s", username, serviceName, accountKey);
	}

	if (accountKey)
	{
		free(accountKey);
	}

	if (success)
		return SENT;
	else return USER_NOT_LOGGED_IN;
}

/*
 * Remove a buddy from our account
 */
LibpurpleAdapter::SendResult LibpurpleAdapter::removeBuddy(const char* serviceName, const char* username, const char* buddyUsername)
{
	bool success = FALSE;
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName || !username || !buddyUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("removeBuddy: null parameter. username %s, service name %s, buddyUsername %s"), username, serviceName, buddyUsername);
		return INVALID_PARAMS;
	}

	char* accountKey = getAccountKey(username, serviceName);
	PurpleAccount* account = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	success = (account != NULL);
	if (success)
	{
		// strip off the "@aol.com" if needed
		char *transportFriendlyUserName = getPrplFriendlyUsername(serviceName, buddyUsername);
		MojLogInfo(IMServiceApp::s_log, _T("removeBuddy: %s"), transportFriendlyUserName);

		PurpleBuddy* buddy = purple_find_buddy(account, transportFriendlyUserName);
		if (NULL == buddy) {
			MojLogError(IMServiceApp::s_log, _T("could not find buddy %s in list - cannot remove"), transportFriendlyUserName);
			success = FALSE;
		}
		else {
			PurpleGroup* group = purple_buddy_get_group(buddy);
			// remove from server list
			purple_account_remove_buddy(account, buddy, group);

			// remove from buddy list - generates a "buddy-removed" signal
			purple_blist_remove_buddy(buddy);
		}

		free(transportFriendlyUserName);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, "removeBuddy: Trying to send from an account that is not logged in. username %s, service name %s, accountKey %s", username, serviceName, accountKey);
	}

	if (accountKey)
	{
		free(accountKey);
	}

	if (success)
		return SENT;
	else return USER_NOT_LOGGED_IN;
}

/*
 * Add a buddy to our buddy list. Some services (GTalk) will require the buddy to authorize us to add them
 */
LibpurpleAdapter::SendResult LibpurpleAdapter::addBuddy(const char* serviceName, const char* username, const char* buddyUsername, const char* groupName)
{
	bool success = FALSE;
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	if (!serviceName || !username || !buddyUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("addBuddy: null parameter. username %s, service name %s, buddyUsername %s"), username, serviceName, buddyUsername);
		return INVALID_PARAMS;
	}

	char* accountKey = getAccountKey(username, serviceName);
	PurpleAccount* account = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	success = (account != NULL);
	if (success)
	{
		// strip off the "@aol.com" if needed
		char *transportFriendlyUserName = getPrplFriendlyUsername(serviceName, buddyUsername);
		MojLogInfo(IMServiceApp::s_log, _T("addBuddy: %s"), transportFriendlyUserName);

		PurpleBuddy* buddy = purple_buddy_new(account, transportFriendlyUserName, /*alias*/ NULL);

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

		free(transportFriendlyUserName);
	}
	else
	{
		MojLogError(IMServiceApp::s_log, "addBuddy: Trying to send from an account that is not logged in. username %s, service name %s, accountKey %s", username, serviceName, accountKey);
	}

	if (accountKey)
	{
		free(accountKey);
	}

	if (success)
		return SENT;
	else return USER_NOT_LOGGED_IN;
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
	char *accountKey = getAccountKey(username, serviceName);
	PurpleAccount* onlineAccount = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);

	if (onlineAccount == NULL)
	{
		MojLogError(IMServiceApp::s_log, _T("authorizeBuddy: account not online. accountKey: %s"), accountKey);
		free (accountKey);
		return USER_NOT_LOGGED_IN;
	}
	// free key since it was only used for lookup
	free (accountKey);

	// create the key and find the auth_and_add object in the s_AuthorizeRequests table
	char *authRequestKey = getAuthRequestKey(username, serviceName, fromUsername);
	AuthRequest *aa = (AuthRequest *)g_hash_table_lookup(s_AuthorizeRequests, authRequestKey);
	if (NULL == aa)
	{
		MojLogError(IMServiceApp::s_log, "authorizeBuddy: cannot find auth request object - authRequestKey %s", authRequestKey);
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
	MojLogError(IMServiceApp::s_log, _T("declineBuddy: username: %s, serviceName: %s, buddyUsername: %s"), username, serviceName, fromUsername);
	if (!serviceName || !username || !fromUsername)
	{
		MojLogError(IMServiceApp::s_log, _T("declineBuddy: null parameter. cannot process command"));
		return INVALID_PARAMS;
	}

	// if we got here, we need to be online...
	char *accountKey = getAccountKey(username, serviceName);
	PurpleAccount* onlineAccount = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
	if (onlineAccount == NULL)
	{
		MojLogError(IMServiceApp::s_log, _T("declineBuddy: account not online. accountKey: %s"), accountKey);
		free (accountKey);
		return USER_NOT_LOGGED_IN;
	}
	// free key since it was only used for lookup
	free (accountKey);

	// create the key and find the auth_and_add object in the s_AuthorizeRequests table
	char *authRequestKey = getAuthRequestKey(username, serviceName, fromUsername);
	AuthRequest *aa = (AuthRequest *)g_hash_table_lookup(s_AuthorizeRequests, authRequestKey);
	if (NULL == aa)
	{
		MojLogError(IMServiceApp::s_log, "declineBuddy: cannot find auth request object - authRequestKey %s", authRequestKey);
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

LibpurpleAdapter::SendResult LibpurpleAdapter::sendMessage(const char* serviceName, const char* username, const char* usernameTo, const char* messageText)
{
	if (!serviceName || !username || !usernameTo || !messageText)
	{
		MojLogError(IMServiceApp::s_log, _T("sendMessage: Invalid parameter. Please double check the passed parameters."));
		return LibpurpleAdapter::INVALID_PARAMS;
	}

	LibpurpleAdapter::SendResult retVal = LibpurpleAdapter::SENT;
	MojLogInfo(IMServiceApp::s_log, _T("%s called."), __FUNCTION__);

	char* accountKey = getAccountKey(username, serviceName);
	PurpleAccount* accountToSendFrom = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);

	if (accountToSendFrom == NULL)
	{
		retVal = LibpurpleAdapter::USER_NOT_LOGGED_IN;
		MojLogError(IMServiceApp::s_log, _T("sendMessage: Trying to send from an account that is not logged in. username %s, service name %s, accountKey %s"), username, serviceName, accountKey);
	}
	else
	{
		// strip off the "@aol.com" if needed
		char *transportFriendlyUserName = getPrplFriendlyUsername(serviceName, usernameTo);
		PurpleConversation* purpleConversation = purple_conversation_new(PURPLE_CONV_TYPE_IM, accountToSendFrom, transportFriendlyUserName);
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
			retVal = LibpurpleAdapter::SEND_FAILED;
			MojLogError(IMServiceApp::s_log, _T("sendMessage: serv_send_im returned err %d"), err);
		}

		free(messageTextUnescaped);
		free(transportFriendlyUserName);
	}
	if (accountKey)
		free(accountKey);
	return retVal;
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

	PurpleAccount* account;
	GSList* accountToLogoutList = NULL;
	GList* iterator = NULL;

	GList* onlineAndPendingAccountKeys = g_hash_table_get_keys(s_ipAddressesBoundTo);
	for (iterator = onlineAndPendingAccountKeys; iterator != NULL; iterator = g_list_next(iterator))
	{
		char* accountKey = (char*)iterator->data;
		char* accountBoundToIpAddress = (char*)g_hash_table_lookup(s_ipAddressesBoundTo, accountKey);
		if (all == true || (accountBoundToIpAddress != NULL && strcmp(ipAddress, accountBoundToIpAddress) == 0))
		{
			bool accountWasLoggedIn = FALSE;

			account = (PurpleAccount*)g_hash_table_lookup(s_onlineAccountData, accountKey);
			if (account == NULL)
			{
				account = (PurpleAccount*)g_hash_table_lookup(s_pendingAccountData, accountKey);
				if (account == NULL)
				{
					MojLogInfo(IMServiceApp::s_log, _T("account was not found in the hash"));
					continue;
				}
				MojLogInfo(IMServiceApp::s_log, _T("Abandoning login"));
			}
			else
			{
				accountWasLoggedIn = TRUE;
				MojLogInfo(IMServiceApp::s_log, _T("Logging out"));
			}

			if (g_hash_table_lookup(s_onlineAccountData, accountKey) != NULL)
			{
				MojLogInfo(IMServiceApp::s_log, _T("deviceConnectionClosed: removing account from onlineAccountData hash table. accountKey %s"), accountKey);
				g_hash_table_remove(s_onlineAccountData, accountKey);
			}
			if (g_hash_table_lookup(s_pendingAccountData, accountKey) != NULL)
			{
				g_hash_table_remove(s_pendingAccountData, accountKey);
			}
			if (g_hash_table_lookup(s_offlineAccountData, accountKey) == NULL)
			{
				/*
				 * Keep the PurpleAccount struct to reuse in future logins
				 */
				g_hash_table_insert(s_offlineAccountData, accountKey, account);
			}
			
			purple_account_disconnect(account);

			accountToLogoutList = g_slist_append(accountToLogoutList, account);

			MojLogInfo(IMServiceApp::s_log, _T("deviceConnectionClosed: removing account from onlineAccountData hash table. accountKey %s"), accountKey);
			g_hash_table_remove(s_onlineAccountData, accountKey);
			// We can't remove this guy since we're iterating through its keys. We'll remove it after the break
			//g_hash_table_remove(ipAddressesBoundTo, accountKey);
		}
	}

	if (g_slist_length(accountToLogoutList) == 0)
	{
		MojLogInfo(IMServiceApp::s_log, _T("No accounts were connected on the requested ip address"));
	}
	else
	{
		GSList* accountIterator = NULL;
		for (accountIterator = accountToLogoutList; accountIterator != NULL; accountIterator = accountIterator->next)
		{
			account = (PurpleAccount*) accountIterator->data;
			char* serviceName = getServiceNameFromPrplProtocolId(account);
			char* username = getMojoFriendlyUsername(account->username, serviceName);
			char* accountKey = getAccountKey(username, serviceName);

			g_hash_table_remove(s_ipAddressesBoundTo, accountKey);

			free(serviceName);
			free(username);
			free(accountKey);
		}
	}

	return TRUE;
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

void LibpurpleAdapter::assignPrefsHandler(LibpurplePrefsHandler* PrefsHandler)
{
	s_PrefsHandler = PrefsHandler;
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

void LibpurpleAdapter::init()
{
	//TODO: replace the NULLs with real functions to prevent memory leaks
	/* g_hash_table_new_full() Creates a new GHashTable like g_hash_table_new() and allows one to specify functions to free the memory allocated for the key and value that get called when removing the entry from the GHashTable.
	 * Parameters:
			hash_func:	a function to create a hash value from a key.
			key_equal_func:	a function to check two keys for equality.
			key_destroy_func:	a function to free the memory allocated for the key used when removing the entry from the GHashTable or NULL if you don't want to supply such a function.
			value_destroy_func:	a function to free the memory allocated for the value used when removing the entry from the GHashTable or NULL if you don't want to supply such a function.
			Returns:	a new GHashTable.
	*/
	s_onlineAccountData = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
	s_pendingAccountData = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_accountLoginTimers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_ipAddressesBoundTo = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free);
	s_connectionTypeData = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free);
	s_AccountIdsData = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free);
	s_offlineAccountData = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_AuthorizeRequests = g_hash_table_new_full(g_str_hash, g_str_equal, free, deleteAuthRequest);
	s_accountBuddyListTimers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	
	initializeLibpurple();
}
