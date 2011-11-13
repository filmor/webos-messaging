/*
 * IMAccountValidatorHandler.cpp
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
 * IMAccountValidator uses libpurple.so to implement the checking of credentials when logging into an IM account
 */

#include "IMAccountValidatorHandler.h"
#include "IMAccountValidatorApp.h"
#include "savedstatuses.h"
#include "core/MojServiceRequest.h"

#define IMVersionString  "IMAccountValidator 6-8 1pm starting...."

MojString ServerName;
MojString ServerPort;
bool ServerTLS = false;
MojString XFireversion;
bool SIPEServerProxy = FALSE;
MojString SIPEUserAgent;
MojString SIPEServerLogin;
bool SametimehideID = FALSE;
MojString JabberResource;
bool BadCert = false;

const IMAccountValidatorHandler::Method IMAccountValidatorHandler::s_methods[] = {
	{_T("checkCredentials"), (Callback) &IMAccountValidatorHandler::validateAccount},
	{_T("logout"), (Callback) &IMAccountValidatorHandler::logout},
	{_T("setpreferences"), (Callback) &IMAccountValidatorHandler::setpreferences},
	{NULL, NULL} };


IMAccountValidatorHandler::IMAccountValidatorHandler(MojService* service)
: m_logoutSlot(this, &IMAccountValidatorHandler::logoutResult),
  m_service(service)
{
	MojLogTrace(IMAccountValidatorApp::s_log);
	m_account = NULL;
	m_serviceMsg = NULL;
	m_logoutServiceMsg = NULL;
}

IMAccountValidatorHandler::~IMAccountValidatorHandler()
{
	MojLogTrace(IMAccountValidatorApp::s_log);

}

/*
 * Set preferences for account we are logging in to
 *
 */
MojErr IMAccountValidatorHandler::setpreferences(MojServiceMessage* serviceMsg, const MojObject payload)
{
	//Load server and port
	MojString errorText;
	bool found = false;
	bool result = true;
	MojString templateId;

	//Get templateId
	MojErr err = payload.get(_T("templateId"), templateId, found);
	if (!found) {
		errorText.assign(_T("Missing templateId in payload."));
		result = false;
	}
	//Get server name
	err = payload.get(_T("ServerName"), ServerName, found);
	if (!found) {
		errorText.assign(_T("Missing ServerName in payload."));
		result = false;
	}
	//Get server port
	err = payload.get(_T("ServerPort"), ServerPort, found);
	//Accept Bad Cert?
	found = payload.get(_T("BadCert"), BadCert);
	if (!found) {
			BadCert = false;
	}
	//Get server TLS
	found = payload.get(_T("ServerTLS"), ServerTLS);
	if (!found) {
			ServerTLS = false;
	}
	//Get XFire version
	if (0 == templateId.compare(TEMPLATE_XFIRE))
	{
		err = payload.get(_T("XFireversion"), XFireversion, found);
		if (!found) {
			errorText.assign(_T("Missing XFireversion in payload."));
			result = false;
		}
	}
	//Get SIPE settings
	if (0 == templateId.compare(TEMPLATE_SIPE))
	{
		//Get SIPE UserAgent
		err = payload.get(_T("SIPEUserAgent"), SIPEUserAgent, found);
		if (!found) {
			SIPEUserAgent.assign("");
		}
		//Get SIPE Login
		err = payload.get(_T("SIPEServerLogin"), SIPEServerLogin, found);
		if (!found) {
			SIPEServerLogin.assign("");
		}
		//Get SIPE Server Proxy
		found = payload.get(_T("SIPEServerProxy"), SIPEServerProxy);
		if (!found) {
			SIPEServerProxy = false;
		}
	}
	//Get Sametime hideID pref
	if (0 == templateId.compare(TEMPLATE_SAMETIME))
	{
		found = payload.get(_T("SametimehideID"), SametimehideID);
		if (!found) {
			SametimehideID = false;
		}
	}
	//Get jabber resource
	if (0 == templateId.compare(TEMPLATE_JABBER))
	{
		err = payload.get(_T("JabberResource"), JabberResource, found);
		if (!found) {
			JabberResource.assign("");
		}
	}
	
	if (!result) {
		serviceMsg->replyError(err, errorText);
		return MojErrNone;
	}
	
	//Return success
	serviceMsg->replySuccess();
	return MojErrNone;
}

/*
 * Initialize the Service
 *
 */
MojErr IMAccountValidatorHandler::init(IMAccountValidatorApp* const app)
{
	static int handle = 0x1AD;

	MojLogTrace(IMAccountValidatorApp::s_log);
	MojLogInfo(IMAccountValidatorApp::s_log, IMVersionString);

	MojErr err = addMethods(s_methods);
	MojErrCheck(err);

	// set up signed-on, signed-off callback pointers - see LibpurpleAdapter::assignIMLoginState
	purple_signal_connect(purple_connections_get_handle(), "signed-on", &handle,
			PURPLE_CALLBACK(IMAccountValidatorHandler::accountLoggedIn), this);
	purple_signal_connect(purple_connections_get_handle(), "signed-off", &handle,
			PURPLE_CALLBACK(IMAccountValidatorHandler::accountSignedOff), this);
	purple_signal_connect(purple_connections_get_handle(), "connection-error", &handle,
			PURPLE_CALLBACK(IMAccountValidatorHandler::accountLoginFailed), this);

	// remember the app pointer so we can shutdown when we are done.
	m_clientApp = app;

	return MojErrNone;
}

void IMAccountValidatorHandler::setaccountprefs(MojString templateId, PurpleAccount* account)
{
	//Set account preferences
	purple_account_set_string(account, "server", ServerName);
	purple_account_set_string(account, "connect_server", ServerName);
	if (0 == templateId.compare(TEMPLATE_GADU))
	{
		//Set connect server
		purple_account_set_string(account, "gg_server", ServerName);
		purple_account_set_int(account, "server_port", atoi(ServerPort));
	}	
	purple_account_set_int(account, "port", atoi(ServerPort));
	purple_account_set_bool(account, "require_tls", ServerTLS);
	
	//Bad Cert Accept?
	purple_prefs_remove("/purple/acceptbadcert");
	if (BadCert)
	{
		MojLogError(IMAccountValidatorApp::s_log, "Accepting Bad Certificates");
		purple_prefs_add_string("/purple/acceptbadcert", "true");
	}
	if (ServerTLS)
	{
		purple_account_set_string(account, "transport", "tls");
	}
	else
	{
		purple_account_set_string(account, "transport", "auto");
	}
	if (0 == templateId.compare(TEMPLATE_QQ))
	{
		purple_account_set_string(account, "client_version", "qq2008");
		
		//Set server in servername:port format for QQ
		char *qqServer = NULL;
		qqServer = (char *)calloc(strlen(ServerName) + strlen(ServerPort) + 1, sizeof(char));
		strcat(qqServer, ServerName);
		strcat(qqServer, ":");
		strcat(qqServer, ServerPort);
		purple_account_set_string(account, "server", qqServer);
	}
	if (0 == templateId.compare(TEMPLATE_FACEBOOK))
	{
		//Don't load chat history
		purple_account_set_bool(account, "facebook_show_history", FALSE);
	}
	if (0 == templateId.compare(TEMPLATE_XFIRE))
	{
		purple_account_set_int(account, "version", atoi(XFireversion));
	}
		
	if (0 == templateId.compare(TEMPLATE_SIPE))
	{
		//Set ServerName
		if(strcmp(ServerName, "") != 0) 
		{
			char *SIPEFullServerName = NULL;
			SIPEFullServerName = (char *)calloc(strlen(ServerName) + strlen(ServerPort) + 1, sizeof(char));
			strcat(SIPEFullServerName, ServerName);
			strcat(SIPEFullServerName, ":");
			strcat(SIPEFullServerName, ServerPort);
			purple_account_set_string(account, "server", SIPEFullServerName);

			if (SIPEFullServerName)
			{
				free(SIPEFullServerName);
			}
		}

		//Proxy?
		if (!SIPEServerProxy)
		{
			//Disable Proxy
			PurpleProxyInfo *info = purple_proxy_info_new();
			purple_proxy_info_set_type(info, PURPLE_PROXY_NONE);
		}

		//User Agent
		if(strcmp(SIPEUserAgent, "") != 0) 
		{
			purple_account_set_string(account, "useragent", SIPEUserAgent);
		}
	}
	if (0 == templateId.compare(TEMPLATE_SAMETIME))
	{
		purple_account_set_bool(account, "fake_client_id", SametimehideID);
	}
	if (0 == templateId.compare(TEMPLATE_JABBER))
	{
		//Set Account Alias to gtalk
		purple_account_set_alias (account,"jabber");
	}
	if (0 == templateId.compare(TEMPLATE_GTALK))
	{
		//Set Account Alias to gtalk
		purple_account_set_alias (account,"gtalk");
	}
}

/*
 * Service method call to validate the given account
 * INPUT:
 * {
    "username": <String>,
    "password": <String>,
    "templateId": <template id> (optional, if known),
    "config": <dictionary of Opaque Configuration Objects> (optional)
    }

    OUTPUT:
    {
    "returnValue": <boolean>,
    "errorText": <String> (only if returnValue=false), (localization of this msg TBD)
    "templateId": <template id> (only if changing),
    "username": <String> (only if changing),
    "credentials": {
        <name>: <Opaque Credentials Object>,
        ...
        },
    "config": {<name>: <Amended Opaque Config Object>, ...} (only if changing)
    }
 *
 * see LibpurpleAdapter::login
 *
 */

MojErr IMAccountValidatorHandler::validateAccount(MojServiceMessage* serviceMsg, const MojObject payload)
{

	// remember the message so we can reply back in the callback
	m_serviceMsg = serviceMsg;

	bool result = true;
	PurpleSavedStatus *status;
	char* transportFriendlyUserName = NULL;

	// log the parameters
	privateLogMojObjectJsonString(_T("validateAccount payload: %s"), payload);

	// get the username and password from the input payload
	MojString username;
	MojString templateId;
	bool found = false;
	MojString errorText;
	char *prplProtocolId = NULL;

	MojErr err = payload.get(_T("username"), username, found);
	if (!found) {
		errorText.assign(_T("Missing username in payload."));
		result = false;
	}
	else {
		err = payload.get(_T("password"), m_password, found);
		if (!found) {
			errorText.assign(_T("Missing password in payload."));
			result = false;
		}
		else {
		    err = payload.get(_T("templateId"), templateId, found);
			if (!found) {
				errorText.assign(_T("Missing templateId in payload."));
				result = false;
			}
			else {
				// figure out which service we are validating from templateID
				if (0 == templateId.compare(TEMPLATE_AIM))
					prplProtocolId = strdup(PURPLE_AIM);
				else if (0 == templateId.compare(TEMPLATE_FACEBOOK))
					prplProtocolId = strdup(PURPLE_FACEBOOK);
				else if (0 == templateId.compare(TEMPLATE_GTALK))
					prplProtocolId = strdup(PURPLE_GTALK);
				else if (0 == templateId.compare(TEMPLATE_GADU))
					prplProtocolId = strdup(PURPLE_GADU);
				else if (0 == templateId.compare(TEMPLATE_GROUPWISE))
					prplProtocolId = strdup(PURPLE_GROUPWISE);
				else if (0 == templateId.compare(TEMPLATE_ICQ))
					prplProtocolId = strdup(PURPLE_ICQ);
				else if (0 == templateId.compare(TEMPLATE_JABBER))
					prplProtocolId = strdup(PURPLE_JABBER);
				else if (0 == templateId.compare(TEMPLATE_LIVE))
					prplProtocolId = strdup(PURPLE_LIVE);
				else if (0 == templateId.compare(TEMPLATE_WLM))
					prplProtocolId = strdup(PURPLE_WLM);
				else if (0 == templateId.compare(TEMPLATE_MYSPACE))
					prplProtocolId = strdup(PURPLE_MYSPACE);
				else if (0 == templateId.compare(TEMPLATE_QQ))
					prplProtocolId = strdup(PURPLE_QQ);
				else if (0 == templateId.compare(TEMPLATE_SAMETIME))
					prplProtocolId = strdup(PURPLE_SAMETIME);
				else if (0 == templateId.compare(TEMPLATE_SIPE))
					prplProtocolId = strdup(PURPLE_SIPE);
				else if (0 == templateId.compare(TEMPLATE_XFIRE))
					prplProtocolId = strdup(PURPLE_XFIRE);
				else if (0 == templateId.compare(TEMPLATE_YAHOO))
					prplProtocolId = strdup(PURPLE_YAHOO);
				else {
					errorText.assign(_T("Invalid templateId in payload."));
					result = false;
				}
			}
		}
	}
	
	MojLogError(IMAccountValidatorApp::s_log, "ID's %s:%s", prplProtocolId, templateId.data());

	if (!result) {
		returnValidateFailed(MojErrInvalidArg, errorText);
		// need to exit gracefully
		m_clientApp->shutdown();
		return MojErrNone;
	}

	// Input is OK - validate the account
	MojLogInfo(IMAccountValidatorApp::s_log, "validateAccount: Logging in...username: %s, password: ****removed****, templateId: %s", username.data(), templateId.data());

	// strip off the "@aol.com"
	transportFriendlyUserName = getPrplFriendlyUsername(prplProtocolId, username.data());

	// save the username with the "@aol.com" to return later
	err = getMojoFriendlyUsername(prplProtocolId, username.data());

	/* Create the account */
	m_account = purple_account_new(transportFriendlyUserName, prplProtocolId);
	if (!m_account) {
		result = false;
	}
	else {
		//Set account preferences
		setaccountprefs(templateId, m_account);

		purple_account_set_password(m_account, m_password.data());
		/* It's necessary to enable the account first. */
		purple_account_set_enabled(m_account, UI_ID, TRUE);

	   /* Now, to activate the account with status=invisible so the user doesn't briefly show as online. */
	   status = purple_savedstatus_new(NULL, PURPLE_STATUS_INVISIBLE);
	   purple_savedstatus_activate(status);
	}
	
	free(transportFriendlyUserName);
	free(prplProtocolId);

	return MojErrNone;
}

/*
 * Log out of the account we just logged into
 *
 * doing this in the libpurple callback doesn't work.
 */
MojErr IMAccountValidatorHandler::logout(MojServiceMessage* serviceMsg, const MojObject payload)
{
	// remember the message so we can reply back in the callback
	m_logoutServiceMsg = serviceMsg;
	purple_account_disconnect(m_account);
	return MojErrNone;
}

/*
 * This method handles special cases where the username passed by the mojo side does not satisfy a particular prpl's requirement
 * (e.g. for logging into AIM, the mojo service uses "palmpre@aol.com", yet the aim prpl expects "palmpre"; same scenario with yahoo)
 * Free the returned string when you're done with it
 *
 * Copied from LibPurpleAdapter.cpp getPrplFriendlyUsername()
 */
MojChar* IMAccountValidatorHandler::getPrplFriendlyUsername(const MojChar* serviceName, const MojChar* username)
{
	if (!username || !serviceName)
	{
		return strdup("");
	}
	char* transportFriendlyUsername = strdup(username);
	if (strcmp(serviceName, PURPLE_AIM) == 0)
	{
		// Need to strip off @aol.com, but not @aol.com.mx
		const char* extension = strstr(username, "@aol.com");
		if (extension != NULL && strstr(extension, "aol.com.") == NULL)
		{
			strtok(transportFriendlyUsername, "@");
		}
	}
	//Special Case for Office Communicator when DOMAIN\USER is set. Account name is USERNAME,DOMAIN\USER
	if (strcmp(serviceName, PURPLE_SIPE) == 0 && strcmp(SIPEServerLogin, "") != 0)
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
	//Special Case for jabber when resource is set. Account name is USERNAME/RESOURCE
	if (strcmp(serviceName, PURPLE_JABBER) == 0 && strcmp(JabberResource, "") != 0)
	{
		if (strstr(username, "/") != NULL)
		{
			//A "/" exists in the sign in name already
			//transportFriendlyUsername = malloc(strlen(username) + 1);
			transportFriendlyUsername = strcpy(transportFriendlyUsername, username);

			return transportFriendlyUsername;
		}
		else
		{
			//transportFriendlyUsername = malloc(strlen(username) + 1);
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
	return transportFriendlyUsername;
}

/*
 * The messaging service expects the username to be in the username@domain.com format, whereas the AIM prpl uses the username only
 */
MojErr IMAccountValidatorHandler::getMojoFriendlyUsername(const char* serviceName, const char* username)
{
	MojLogInfo(IMAccountValidatorApp::s_log, "getMojoFriendlyUsername: serviceName %s, username %s", serviceName, username);
	if (!username || !serviceName)
	{
		return MojErrInvalidArg;
	}
	GString *newusername = g_string_new(username);
	
	if (strcmp(serviceName, PURPLE_AIM) == 0 && strchr(username, '@') == NULL)
	{
		m_mojoUsername.assign(username);
		m_mojoUsername.append("@aol.com");
		MojLogInfo(IMAccountValidatorApp::s_log, "getMojoFriendlyUsername: new username %s", m_mojoUsername.data());
	}
	else if (strcmp(serviceName, PURPLE_YAHOO) == 0 && strchr(username, '@') == NULL)
	{
		g_string_append(newusername, "@yahoo.com");
		
		m_mojoUsername.assign(newusername->str);
		MojLogInfo(IMAccountValidatorApp::s_log, "getMojoFriendlyUsername: new username %s", m_mojoUsername.data());
	}
	else if (strcmp(serviceName, PURPLE_GTALK) == 0)
	{
		char* resource = (char*)memchr(username, '/', strlen(username));
		if (resource != NULL)
		{
			int charsToKeep = resource - username;
			g_string_erase(newusername, charsToKeep, -1);
			
			m_mojoUsername.assign(newusername->str);
			MojLogInfo(IMAccountValidatorApp::s_log, "getMojoFriendlyUsername: new username %s", m_mojoUsername.data());
		}
	}
	else if (strcmp(serviceName, PURPLE_JABBER) == 0)
	{
		if (strcmp(JabberResource, "") != 0)
		{
			//If jabber resource is blank remove /
			char *resource = (char*)memchr(username, '/', strlen(username));
			if (resource != NULL)
			{
				int charsToKeep = resource - username;
				g_string_erase(newusername, charsToKeep, -1);
				
				m_mojoUsername.assign(newusername->str);
				MojLogInfo(IMAccountValidatorApp::s_log, "getMojoFriendlyUsername: new username %s", m_mojoUsername.data());
			}
		}
	}
	else if (strcmp(serviceName, PURPLE_SIPE) == 0)
	{
		char *resource = (char*)memchr(username, ',', strlen(username));
		if (resource != NULL)
		{
			int charsToKeep = resource - username;
			g_string_erase(newusername, charsToKeep, -1);
			
			m_mojoUsername.assign(newusername->str);
			MojLogInfo(IMAccountValidatorApp::s_log, "getMojoFriendlyUsername: new username %s", m_mojoUsername.data());
		}
	}

	return MojErrNone;
}


/*
 * return json like so
 *  returnValue: true,
 *  credentials: {
 *    common: {
 *      password: password
 *    }
 *  }
 *
 */
void IMAccountValidatorHandler::returnValidateSuccess()
{
	// First reply to account services with the credentials
	MojObject reply, common, password;
	password.put("password", m_password);
	common.put("common", password);
	reply.put("credentials", common);

	// add normalized username if it changed (ie. if we need to add "@aol.com")
	if (!m_mojoUsername.empty()) {
		reply.put("username", m_mojoUsername);
	}

	m_serviceMsg->replySuccess(reply);

	// Done with password so clear it out
	m_password.clear();

	// clean up - need to send a logout message to ourselves
	// luna-send -n 1 palm://org.webosinternals.imaccountvalidator/logout '{}'

	// log the send
	MojLogInfo(IMAccountValidatorApp::s_log, "Issuing logout request to imaccountvalidator");

	// Get a request object from the service
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);

	// create empty parameter object
	MojObject params;

	if (!err)
		err = req->send(m_logoutSlot, "org.webosinternals.imaccountvalidator", "logout", params);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMAccountValidatorApp::s_log, _T("imaccountvalidator sending request failed. error %d - %s"), err, error.data());
	}

}

/*
 * Return form for AIM:
 * {"errorCode":2,"errorText":"Incorrect password.","returnValue":false}
 */
void IMAccountValidatorHandler::returnValidateFailed(MojErr err, MojString errorText)
{
	MojLogError(IMAccountValidatorApp::s_log, "returnValidateFailed error: %d - %s", err, errorText.data());
	m_serviceMsg->replyError(err, errorText);

	// in the login fail case, libPurple goes back and disconnects the account so we can't delete it here.

}

void IMAccountValidatorHandler::returnLogoutSuccess()
{
	if (m_logoutServiceMsg.get() != NULL) {
		m_logoutServiceMsg->replySuccess();
		m_logoutServiceMsg = NULL; // Null this to prevent multiple responses in the login failed case
	}
	else {
		// need to exit gracefully
		m_clientApp->shutdown();
	}

}

/*
 * bus Callback for the logout after a successful login (from returnLogoutSuccess)
 */
MojErr IMAccountValidatorHandler::logoutResult(MojObject& result, MojErr err)
{
	MojLogTrace(IMAccountValidatorApp::s_log);

	if (err) {

		MojString error;
		MojErrToString(err, error);
		MojLogError(IMAccountValidatorApp::s_log, _T("imaccountvalidator logout failed. error %d - %s"), err, error.data());
	} else {
		MojLogInfo(IMAccountValidatorApp::s_log, _T("imaccountvalidator logout succeeded."));
	}

	// exit gracefully in success case
	m_clientApp->shutdown();
	return MojErrNone;
}

/**
 * Log the json for a MojObject - useful for debugging. Need to remove "password".
 */
MojErr IMAccountValidatorHandler::privateLogMojObjectJsonString(const MojChar* format, const MojObject mojObject) {

	MojString mojStringJson;
	MojString msgText;

	// copy the object
	MojObject logObject = mojObject;
	// replace the password
	bool found = false;
	logObject.del(_T("password"), found);
	if (found) {
		msgText.assign(_T("***removed***"));
		logObject.put(_T("password"), msgText);
	}

	// log it (non debug)
	logObject.toJson(mojStringJson);
	MojLogInfo(IMAccountValidatorApp::s_log, format, mojStringJson.data());

	return MojErrNone;
}

/*
 * LibPurple callback
 */
void IMAccountValidatorHandler::accountLoggedIn(PurpleConnection* gc, gpointer accountValidator)
{
	MojLogInfo(IMAccountValidatorApp::s_log, "Account signed in. connection state: %d", gc->state);

	IMAccountValidatorHandler *validator = (IMAccountValidatorHandler*) accountValidator;
	validator->returnValidateSuccess();

}

/*
 * LibPurple callback
 *
 * Note - we also get herein the case of the login failed since Lippurple still signs off the account in that case.
 */
void IMAccountValidatorHandler::accountSignedOff(PurpleConnection* gc, gpointer accountValidator)
{
	MojLogInfo(IMAccountValidatorApp::s_log, "Account signed off. connection state: %d", gc->state);

	// return true to validateAccount caller
	IMAccountValidatorHandler *validator = (IMAccountValidatorHandler*) accountValidator;
	validator->returnLogoutSuccess();
}

/*
 * LibPurple callback
 */
void IMAccountValidatorHandler::accountLoginFailed(PurpleConnection* gc, PurpleConnectionError error, const gchar* description,	gpointer accountValidator)
{
	MojLogError(IMAccountValidatorApp::s_log, "accountLoginFailed is called with error: %d. Text: %s", error, description);

	// return result

	MojString errorText;
	errorText.assign(description);

	MojErr mojErr = (MojErr) getAccountErrorForPurpleError(error);
	// note libpurple error: "Could not connect to authentication server" is error code 0!

	// send error back to caller
	IMAccountValidatorHandler *validator = (IMAccountValidatorHandler*) accountValidator;
	validator->returnValidateFailed(mojErr, errorText);
}

/*
 * Translate the purple error into one the account service will recognize
 */
AccountErrorCode IMAccountValidatorHandler::getAccountErrorForPurpleError(PurpleConnectionError purpleErr)
{
	AccountErrorCode accountErr = UNKNOWN_ERROR;

	// Could not connect to authentication server
	if (PURPLE_CONNECTION_ERROR_NETWORK_ERROR == purpleErr)
		accountErr = CONNECTION_FAILED;
	// invalid credentials
	else if (PURPLE_CONNECTION_ERROR_INVALID_USERNAME == purpleErr)
		accountErr = UNAUTHORIZED_401;
	else if (PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED == purpleErr)
		accountErr = UNAUTHORIZED_401;
	// ssl errors
	else if (PURPLE_CONNECTION_ERROR_CERT_NOT_PROVIDED == purpleErr)
		accountErr = SSL_CERT_INVALID;  // is this right?
	else if (PURPLE_CONNECTION_ERROR_CERT_UNTRUSTED == purpleErr)
		accountErr = SSL_CERT_EXPIRED;
	else if (PURPLE_CONNECTION_ERROR_CERT_EXPIRED == purpleErr)
		accountErr = SSL_CERT_EXPIRED;
	else if (PURPLE_CONNECTION_ERROR_CERT_HOSTNAME_MISMATCH == purpleErr)
		accountErr = SSL_CERT_HOSTNAME_MISMATCH;

	MojLogInfo(IMAccountValidatorApp::s_log, "getAccountErrorForPurpleError: purple error %d, account error %d", purpleErr, accountErr);

	return accountErr;
}

