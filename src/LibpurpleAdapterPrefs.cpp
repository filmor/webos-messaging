/*
 * LibpurpleAdapterPrefs.cpp
 *
 */

#include "LibpurpleAdapterPrefs.h"
#include "LibpurpleAdapter.h"
#include "PalmImCommon.h"
#include "IMServiceApp.h"
#include "db/MojDbServiceClient.h"

/*
 * list of account preferences
 */
static GHashTable* s_AccountAliases = NULL;
static GHashTable* s_AccountAvatars = NULL;
static GHashTable* s_AccountBadCert = NULL;
static GHashTable* s_ServerName = NULL;
static GHashTable* s_ServerPort = NULL;
static GHashTable* s_ServerTLS = NULL;
static GHashTable* s_LoginTimeOut = NULL;
static GHashTable* s_BuddyListTimeOut = NULL;

static GHashTable* s_SametimeHideID = NULL;
static GHashTable* s_SIPEServerProxy = NULL;
static GHashTable* s_SIPEUserAgent = NULL;
static GHashTable* s_XFireVersion = NULL;
static GHashTable* s_SIPEServerLogin = NULL;
static GHashTable* s_JabberResource = NULL;

LibpurpleAdapterPrefs::LibpurpleAdapterPrefs(MojService* service)
: m_findCommandSlot(this, &LibpurpleAdapterPrefs::findCommandResult),
  m_service(service),
  m_dbClient(service)
{
	
}

LibpurpleAdapterPrefs::~LibpurpleAdapterPrefs() {
	
}

inline const char * const BoolToString(bool b)
{
	return b ? "true" : "false";
}

inline bool StringToBool(char* c)
{
	if(c == NULL)
	{
		return false;
	}
	if (strcmp(c, "false") == 0)
	{
		return false;
	}
	if (strcmp(c, "true") == 0)
	{
		return true;
	}
	return false;
}

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
	g_string_free(mojoFriendlyUsername, true);
	return mojoFriendlyUsernameToReturn;
}

static char* getMojoFriendlyServiceName (const char* templateId)
{
	if (strcmp(templateId, CAPABILITY_AIM) == 0) {
		return (char*)SERVICENAME_AIM;
	}
	else if (strcmp(templateId, CAPABILITY_FACEBOOK) == 0){
		return (char*)SERVICENAME_FACEBOOK;
	}
	else if (strcmp(templateId, CAPABILITY_GTALK) == 0){
		return (char*)SERVICENAME_GTALK;
	}
	else if (strcmp(templateId, CAPABILITY_GADU) == 0){
		return (char*)SERVICENAME_GADU;
	}
	else if (strcmp(templateId, CAPABILITY_GROUPWISE) == 0){
		return (char*)SERVICENAME_GROUPWISE;
	}
	else if (strcmp(templateId, CAPABILITY_ICQ) == 0){
		return (char*)SERVICENAME_ICQ;
	}
	else if (strcmp(templateId, CAPABILITY_JABBER) == 0){
		return (char*)SERVICENAME_JABBER;
	}
	else if (strcmp(templateId, CAPABILITY_LIVE) == 0){
		return (char*)SERVICENAME_LIVE;
	}
	else if (strcmp(templateId, CAPABILITY_WLM) == 0){
		return (char*)SERVICENAME_WLM;
	}
	else if (strcmp(templateId, CAPABILITY_MYSPACE) == 0){
		return (char*)SERVICENAME_MYSPACE;
	}
	else if (strcmp(templateId, CAPABILITY_QQ) == 0){
		return (char*)SERVICENAME_QQ;
	}
	else if (strcmp(templateId, CAPABILITY_SAMETIME) == 0){
		return (char*)SERVICENAME_SAMETIME;
	}
	else if (strcmp(templateId, CAPABILITY_SIPE) == 0){
		return (char*)SERVICENAME_SIPE;
	}
	else if (strcmp(templateId, CAPABILITY_XFIRE) == 0){
		return (char*)SERVICENAME_XFIRE;
	}
	else if (strcmp(templateId, CAPABILITY_YAHOO) == 0){
		return (char*)SERVICENAME_YAHOO;
	}
	
	return (char*)"";
}

//Preferences methods
MojErr LibpurpleAdapterPrefs::LoadAccountPreferences(const char* templateId, const char* UserName)
{
	MojString m_templateId;
	MojString m_UserName;
	m_templateId.assign(templateId);
	m_UserName.assign(UserName);

	// Create the query
	MojDbQuery query;
	MojErr err;
	
	//If the template is empty load everything
	if (!strcmp(m_templateId, "") == 0)
	{
		err = query.where("templateId", MojDbQuery::OpEq, m_templateId);
		MojErrCheck(err);

		err = query.where("UserName", MojDbQuery::OpEq, m_UserName);
		MojErrCheck(err);
	}

	// add our kind to the object
	err = query.from("org.webosinternals.messaging.prefs:1");
	MojErrCheck(err);
	
	// log the query
	MojObject queryObject;
	err = query.toObject(queryObject);
	MojErrCheck(err);
	
	MojString json;
	queryObject.toJson(json);
	MojLogError(IMServiceApp::s_log, _T("LoadAccountPreference Query: %s"), json.data());
	
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("LoadAccountPreference Query failed: error %d - %s"), err, error.data());
		return err;
	}

	// query DB
	err = m_dbClient.find(this->m_findCommandSlot, query, true);
	
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("LoadAccountPreference Query find failed: error %d - %s"), err, error.data());
		return err;
	}
	
	return MojErrNone;
}

/*
 * Callback for the prefs query
 */
MojErr LibpurpleAdapterPrefs::findCommandResult(MojObject& result, MojErr findErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (findErr) {
		MojString error;
		MojErrToString(findErr, error);
		MojLogError(IMServiceApp::s_log, _T("findCommandResult failed: error %d - %s"), findErr, error.data());

	} else {
		// get the results
		MojString mojStringJson;
		result.toJson(mojStringJson);
		MojLogError(IMServiceApp::s_log, _T("findCommandResult result: %s"), mojStringJson.data());

		MojObject results;

		// get results array in "results"
		result.get(_T("results"), results);
		
		//Are there results?
		if (!results.empty())
		{
			bool BadCert = false;
			bool EnableAlias = false;
			bool EnableAvatar = false;
			bool ServerTLS = false;
			MojString Preferences;
			MojString ServerName;
			MojString ServerPort;
			MojString LoginTimeOut;
			MojString BuddyListTimeOut;
			MojString UserName;
			MojString templateId;
			MojObject m_ServerName;
			MojObject m_ServerPort;
			MojObject m_UserName;
			MojObject m_templateId;
			MojObject accountPreferences;
			
			bool SametimeHideID = false;
			bool SIPEServerProxy = false;
			MojString SIPEUserAgent;
			MojString XFireVersion;
			MojString SIPEServerLogin;
			MojString JabberResource;
			MojObject m_SIPEUserAgent;
			MojObject m_XFireVersion;
			MojObject m_SIPEServerLogin;
			MojObject m_JabberResource;

			MojObject::ConstArrayIterator PrefslItr = results.arrayBegin();
			while (PrefslItr != results.arrayEnd()) {
				accountPreferences = *PrefslItr;
				
				accountPreferences.toJson(Preferences);
				MojLogError(IMServiceApp::s_log, _T("findCommandResult 'Preferences' result: %s"), Preferences.data());

				//Get the results
				accountPreferences.get ("BadCert", BadCert);
				accountPreferences.get ("EnableAlias", EnableAlias);
				accountPreferences.get ("EnableAvatar", EnableAvatar);
				accountPreferences.getRequired ("ServerName", ServerName);
				accountPreferences.getRequired ("ServerPort", ServerPort);
				accountPreferences.getRequired ("LoginTimeOut", LoginTimeOut);
				accountPreferences.getRequired ("BuddyListTimeOut", BuddyListTimeOut);
				accountPreferences.get ("ServerTLS", ServerTLS);
				accountPreferences.getRequired ("UserName", UserName);
				accountPreferences.getRequired ("templateId", templateId);
				
				//Get non standard prefs
				if (0 == templateId.compare(CAPABILITY_SAMETIME))
				{
					accountPreferences.get ("SametimeHideID", SametimeHideID);
				}
				if (0 == templateId.compare(CAPABILITY_SIPE))
				{
					accountPreferences.get ("SIPEServerProxy", SIPEServerProxy);
					accountPreferences.getRequired ("SIPEUserAgent", SIPEUserAgent);
					accountPreferences.getRequired ("SIPEServerLogin", SIPEServerLogin);
				}
				if (0 == templateId.compare(CAPABILITY_JABBER))
				{
					accountPreferences.getRequired ("JabberResource", JabberResource);
				}
				if (0 == templateId.compare(CAPABILITY_XFIRE))
				{
					accountPreferences.getRequired ("XFireversion", XFireVersion);
				}
				
				//Set AccountID
				char *accountKey = NULL;
				const char* ServiceType = getMojoFriendlyServiceName (templateId.data());
				const char* mojUserName = getMojoFriendlyUsername ((const char*)UserName.data(),ServiceType);
				asprintf(&accountKey, "%s_%s", mojUserName, ServiceType);
				
				//Remove any entry in the hash tables
				if (g_hash_table_lookup(s_AccountAliases, accountKey) != NULL)
				{
					g_hash_table_remove(s_AccountAliases, accountKey);
				}
				if (g_hash_table_lookup(s_AccountAvatars, accountKey) != NULL)
				{
					g_hash_table_remove(s_AccountAvatars, accountKey);
				}
				if (g_hash_table_lookup(s_AccountBadCert, accountKey) != NULL)
				{
					g_hash_table_remove(s_AccountBadCert, accountKey);
				}
				if (g_hash_table_lookup(s_ServerName, accountKey) != NULL)
				{
					g_hash_table_remove(s_ServerName, accountKey);
				}
				if (g_hash_table_lookup(s_ServerPort, accountKey) != NULL)
				{
					g_hash_table_remove(s_ServerPort, accountKey);
				}
				if (g_hash_table_lookup(s_LoginTimeOut, accountKey) != NULL)
				{
					g_hash_table_remove(s_LoginTimeOut, accountKey);
				}
				if (g_hash_table_lookup(s_BuddyListTimeOut, accountKey) != NULL)
				{
					g_hash_table_remove(s_BuddyListTimeOut, accountKey);
				}
				if (g_hash_table_lookup(s_ServerTLS, accountKey) != NULL)
				{
					g_hash_table_remove(s_ServerTLS, accountKey);
				}
				if (g_hash_table_lookup(s_SametimeHideID, accountKey) != NULL)
				{
					g_hash_table_remove(s_SametimeHideID, accountKey);
				}
				if (g_hash_table_lookup(s_SIPEServerProxy, accountKey) != NULL)
				{
					g_hash_table_remove(s_SIPEServerProxy, accountKey);
				}
				if (g_hash_table_lookup(s_SIPEUserAgent, accountKey) != NULL)
				{
					g_hash_table_remove(s_SIPEUserAgent, accountKey);
				}
				if (g_hash_table_lookup(s_SIPEServerLogin, accountKey) != NULL)
				{
					g_hash_table_remove(s_SIPEServerLogin, accountKey);
				}
				if (g_hash_table_lookup(s_XFireVersion, accountKey) != NULL)
				{
					g_hash_table_remove(s_XFireVersion, accountKey);
				}
				if (g_hash_table_lookup(s_JabberResource, accountKey) != NULL)
				{
					g_hash_table_remove(s_JabberResource, accountKey);
				}
				
				//Show results
				//findCommandResult result: {"results":[{"BadCert":false,"EnableAlias":true,"EnableAvatar":true,"ServerName":"messenger.hotmail.com","ServerPort":"1863","UserName":"SOMEONE@HOTMAIL.COM","_id":"++HRyXbD3r+pt8M2","_kind":"org.webosinternals.messaging.prefs:1","_rev":41856,"_sync":true,"templateId":"ORG.WEBOSINTERNALS.MESSAGING.LIVE"}],"returnValue":true}
				if (BadCert)
				{
					MojLogError(IMServiceApp::s_log, _T("'Preference' BadCert: true"));
				}
				else
				{
					MojLogError(IMServiceApp::s_log, _T("'Preference' BadCert: false"));
				}
				if (EnableAlias)
				{
					MojLogError(IMServiceApp::s_log, _T("'Preference' EnableAlias: true"));
				}
				else
				{
					MojLogError(IMServiceApp::s_log, _T("'Preference' EnableAlias: false"));
				}
				if (EnableAvatar)
				{
					MojLogError(IMServiceApp::s_log, _T("'Preference' EnableAvatar: true"));
				}
				else
				{
					MojLogError(IMServiceApp::s_log, _T("'Preference' EnableAvatar: false"));
				}
				if (ServerTLS)
				{
					MojLogError(IMServiceApp::s_log, _T("'Preference' ServerTLS: true"));
				}
				else
				{
					MojLogError(IMServiceApp::s_log, _T("'Preference' ServerTLS: false"));
				}
				if (0 == templateId.compare(CAPABILITY_SAMETIME))
				{
					//Show results
					if (SametimeHideID)
					{
						MojLogError(IMServiceApp::s_log, _T("'Preference' SametimeHideID: true"));
					}
					else
					{
						MojLogError(IMServiceApp::s_log, _T("'Preference' SametimeHideID: false"));
					}
					//Add the results to the hash tables
					g_hash_table_insert(s_SametimeHideID, accountKey, strdup(BoolToString(SametimeHideID)));
				}
				if (0 == templateId.compare(CAPABILITY_SIPE))
				{
					//Show results
					if (SIPEServerProxy)
					{
						MojLogError(IMServiceApp::s_log, _T("'Preference' SIPEServerProxy: true"));
					}
					else
					{
						MojLogError(IMServiceApp::s_log, _T("'Preference' SIPEServerProxy: false"));
					}
					MojLogError(IMServiceApp::s_log, _T("'Preference' SIPEUserAgent: %s"), SIPEUserAgent.data());
					MojLogError(IMServiceApp::s_log, _T("'Preference' SIPEServerLogin: %s"), SIPEServerLogin.data());
					
					//Add the results to the hash tables
					g_hash_table_insert(s_SIPEServerLogin, accountKey, strdup(SIPEServerLogin.data()));
					g_hash_table_insert(s_SIPEUserAgent, accountKey, strdup(SIPEUserAgent.data()));
					g_hash_table_insert(s_SIPEServerProxy, accountKey, strdup(BoolToString(SIPEServerProxy)));
				}
				MojLogError(IMServiceApp::s_log, _T("'Preference' ServerName: %s"), ServerName.data());
				MojLogError(IMServiceApp::s_log, _T("'Preference' ServerPort: %s"), ServerPort.data());
				MojLogError(IMServiceApp::s_log, _T("'Preference' LoginTimeOut: %s"), LoginTimeOut.data());
				MojLogError(IMServiceApp::s_log, _T("'Preference' BuddyListTimeOut: %s"), BuddyListTimeOut.data());
				MojLogError(IMServiceApp::s_log, _T("'Preference' UserName: %s"), UserName.data());
				MojLogError(IMServiceApp::s_log, _T("'Preference' templateId: %s"), templateId.data());
				
				if (0 == templateId.compare(CAPABILITY_XFIRE))
				{
					//Show results
					MojLogError(IMServiceApp::s_log, _T("'Preference' XFireVersion: %s"), XFireVersion.data());
					//Add the results to the hash tables
					g_hash_table_insert(s_XFireVersion, accountKey, strdup(XFireVersion.data()));
				}
				if (0 == templateId.compare(CAPABILITY_JABBER))
				{
					//Show results
					MojLogError(IMServiceApp::s_log, _T("'Preference' JabberResource: %s"), JabberResource.data());
					//Add the results to the hash tables
					g_hash_table_insert(s_JabberResource, accountKey, strdup(JabberResource.data()));
				}
				
				//Add the results to the hash tables
				g_hash_table_insert(s_AccountAliases, accountKey, strdup(BoolToString(EnableAlias)));
				g_hash_table_insert(s_AccountAvatars, accountKey, strdup(BoolToString(EnableAvatar)));
				g_hash_table_insert(s_AccountBadCert, accountKey, strdup(BoolToString(BadCert)));
				g_hash_table_insert(s_ServerName, accountKey, strdup(ServerName.data()));
				g_hash_table_insert(s_ServerPort, accountKey, strdup(ServerPort.data()));
				g_hash_table_insert(s_ServerTLS, accountKey, strdup(BoolToString(ServerTLS)));
				g_hash_table_insert(s_LoginTimeOut, accountKey, strdup(LoginTimeOut.data()));
				g_hash_table_insert(s_BuddyListTimeOut, accountKey, strdup(BuddyListTimeOut.data()));
			
				//Loop
				PrefslItr++;
			}
		}	
	}

	return MojErrNone;
}

MojErr LibpurpleAdapterPrefs::GetServerPreferences(const char* templateId, const char* UserName, char*& ServerName, char*& ServerPort, bool& ServerTLS, bool& BadCert)
{
	//Set AccountID
	char *accountKey = NULL;
	const char* ServiceType = getMojoFriendlyServiceName (templateId);
	const char* mojUserName = getMojoFriendlyUsername (UserName,ServiceType);
	asprintf(&accountKey, "%s_%s", mojUserName, ServiceType);
	
	//Get Server Name
	ServerName = (char*)g_hash_table_lookup(s_ServerName, accountKey);
	if (ServerName == NULL)
	{
		ServerName = (char*)"";
	}
	
	//Get Server Port
	ServerPort = (char*)g_hash_table_lookup(s_ServerPort, accountKey);
	if (ServerPort == NULL)
	{
		ServerPort = (char*)"0";
	}
	
	//Get Server TLS
	ServerTLS = StringToBool((char*)g_hash_table_lookup(s_ServerTLS, accountKey));
	
	//Get Server Bad Cert Setting
	BadCert = StringToBool((char*)g_hash_table_lookup(s_AccountBadCert, accountKey));
	
	return MojErrNone;
}

char* LibpurpleAdapterPrefs::GetStringPreference(const char* Preference, const char* templateId, const char* UserName)
{MojLogError(IMServiceApp::s_log, _T("In GetStringPrefs: %s"),Preference);
	// Set AccountID
	char *accountKey = NULL;
	const char* ServiceType = getMojoFriendlyServiceName (templateId);
	const char* mojUserName = getMojoFriendlyUsername (UserName,ServiceType);
	asprintf(&accountKey, "%s_%s", mojUserName, ServiceType);

	char* Setting = NULL;
	
	if (strcmp(Preference, "SIPEUserAgent") == 0)
	{
		//Get SIPEUserAgent Setting
		Setting = (char*)g_hash_table_lookup(s_SIPEUserAgent, accountKey);
	}
	if (strcmp(Preference, "XFireVersion") == 0)
	{
		//Get XFireVersion Setting
		Setting = (char*)g_hash_table_lookup(s_XFireVersion, accountKey);
		
		//If Xfire version is blank default to 132
		if (Setting == NULL)
		{
			Setting = (char*)"132";
		}
	}
	if (strcmp(Preference, "SIPEServerLogin") == 0)
	{
		//Get SIPEServerLogin Setting
		Setting = (char*)g_hash_table_lookup(s_SIPEServerLogin, accountKey);
	}
	if (strcmp(Preference, "JabberResource") == 0)
	{
		//Get JabberResource Setting
		Setting = (char*)g_hash_table_lookup(s_JabberResource, accountKey);
	}
	
	if (strcmp(Preference, "LoginTimeOut") == 0)
	{
		//Get Server Timeout
		Setting = (char*)g_hash_table_lookup(s_LoginTimeOut, accountKey);
		if (Setting == NULL)
		{
			Setting = (char*)"45";
		}
	}
	
	if (strcmp(Preference, "BuddyListTimeOut") == 0)
	{
		//Get BuddyList Timeout
		Setting = (char*)g_hash_table_lookup(s_BuddyListTimeOut, accountKey);
		if (Setting == NULL)
		{
			Setting = (char*)"10";
		}
	}
	
	if (Setting == NULL)
	{
		return (char*)"";
	}
	else
	{
		return Setting;
	}
}

bool LibpurpleAdapterPrefs::GetBoolPreference(const char* Preference, const char* templateId, const char* UserName)
{
	//Set AccountID
	char *accountKey = NULL;
	const char* ServiceType = getMojoFriendlyServiceName (templateId);
	const char* mojUserName = getMojoFriendlyUsername (UserName,ServiceType);
	asprintf(&accountKey, "%s_%s", mojUserName, ServiceType);
	
	bool Setting = false;
	
	if (strcmp(Preference, "Avatar") == 0)
	{
		//Get Avatar Setting
		Setting = StringToBool((char*)g_hash_table_lookup(s_AccountAvatars, accountKey));
		return Setting;
	}
	if (strcmp(Preference, "Alias") == 0)
	{
		//Get Alias Setting
		Setting = StringToBool((char*)g_hash_table_lookup(s_AccountAliases, accountKey));
		return Setting;
	}
	if (strcmp(Preference, "SametimeHideID") == 0)
	{
		//Get SametimeHideID Setting
		Setting = StringToBool((char*)g_hash_table_lookup(s_SametimeHideID, accountKey));
		return Setting;
	}
	if (strcmp(Preference, "SIPEServerProxy") == 0)
	{
		//Get SIPEServerProxy Setting
		Setting = StringToBool((char*)g_hash_table_lookup(s_SIPEServerProxy, accountKey));
		return Setting;
	}
	return false;
}

/*
 * Given the prpl-specific protocol_id, it will return mojo-friendly serviceName (e.g. given "prpl-aim", it will return "type_aim")
 * Free the returned string when you're done with it 
 */
static char* getServiceNameFromPrplProtocolId(char* prplProtocolId)
{
	if (!prplProtocolId)
	{
		MojLogError(IMServiceApp::s_log, _T("getServiceNameFromPrplProtocolId called with empty protocolId"));
		return strdup("type_default");
	}
	MojLogError(IMServiceApp::s_log, _T("getServiceNameFromPrplProtocolId, prplProtocolId = %s"), prplProtocolId);
	char* stringChopper = prplProtocolId;
	stringChopper += strlen("prpl-");
	GString* serviceName = g_string_new(stringChopper);

	if (strcmp(serviceName->str, "jabber") == 0)
	{
		// Special case for gtalk where the mojo serviceName is "type_gtalk" and the prpl protocol_id is "prpl-jabber"
		g_string_free(serviceName, true);
		serviceName = g_string_new("gtalk");
	}
	if (strcmp(serviceName->str, "msn") == 0)
	{
		// Special case for live where the mojo serviceName is "type_live" and the prpl protocol_id is "prpl-msn"
		g_string_free(serviceName, true);
		serviceName = g_string_new("live");
	}
	if (strcmp(serviceName->str, "msn-pecan") == 0)
	{
		// Special case for wlm where the mojo serviceName is "type_wlm" and the prpl protocol_id is "prpl-msn-pecan"
		g_string_free(serviceName, true);
		serviceName = g_string_new("wlm");
	}
	if (strcmp(serviceName->str, "bigbrownchunx-facebookim") == 0)
	{
		// Special case for facebook where the mojo serviceName is "type_facebook" and the prpl protocol_id is "prpl-bigbrownchunx-facebookim"
		g_string_free(serviceName, true);
		serviceName = g_string_new("facebook");
	}
	if (strcmp(serviceName->str, "meanwhile") == 0)
	{
		// Special case for sametime where the mojo serviceName is "type_sametime" and the prpl protocol_id is "prpl-meanwhile"
		g_string_free(serviceName, true);
		serviceName = g_string_new("sametime");
	}
	if (strcmp(serviceName->str, "novell") == 0)
	{
		// Special case for groupwise where the mojo serviceName is "type_groupwise" and the prpl protocol_id is "prpl-novell"
		g_string_free(serviceName, true);
		serviceName = g_string_new("groupwise");
	}
	if (strcmp(serviceName->str, "gg") == 0)
	{
		// Special case for gadu where the mojo serviceName is "type_gadu" and the prpl protocol_id is "prpl-gg"
		g_string_free(serviceName, true);
		serviceName = g_string_new("gadu");
	}
	MojLogError(IMServiceApp::s_log, _T("getServiceNameFromPrplProtocolId, serviceName = %s"),serviceName->str);
	char* serviceNameToReturn = NULL;
	// asprintf allocates appropriate-sized buffer
	asprintf(&serviceNameToReturn, "type_%s", serviceName->str);
	g_string_free(serviceName, true);
	return serviceNameToReturn;
}

void LibpurpleAdapterPrefs::setaccountprefs(MojString templateId, PurpleAccount* account)
{
	//Load the preferences
	char* ServerName;
	char* ServerPort;
	bool ServerTLS = false;
	bool BadCert = false;
	char* SIPEUserAgent;
	char* XFireversion;
	bool SIPEServerProxy = false;
	bool SametimehideID = false;
	
	//Load account preferences
	char* serviceName = getServiceNameFromPrplProtocolId(account->protocol_id);
	char* username = getMojoFriendlyUsername(account->username, serviceName);
	GetServerPreferences((const char*)templateId.data(), (const char*)username, ServerName, ServerPort, ServerTLS, BadCert);
	
	//If no server blank
	if (strcmp(ServerName, "") == 0)
	{
		ServerName = (char*)"";
	}
	//If no port quit
	if (strcmp(ServerPort, "") == 0)
	{
		return;
	}
	
	//Bad Cert Accept?
	purple_prefs_remove("/purple/acceptbadcert");
	if (BadCert)
	{
		MojLogError(IMServiceApp::s_log, "Accepting Bad Certificates");
		purple_prefs_add_string("/purple/acceptbadcert", "true");
	}
	
	//Set account preferences
	purple_account_set_string(account, "server", ServerName);
	purple_account_set_string(account, "connect_server", ServerName);
	if (0 == templateId.compare(CAPABILITY_GADU))
	{
		//Set connect server
		purple_account_set_string(account, "gg_server", ServerName);
		purple_account_set_int(account, "server_port", atoi(ServerPort));
	}
	purple_account_set_int(account, "port", atoi(ServerPort));
	purple_account_set_bool(account, "require_tls", ServerTLS);
	
	if (ServerTLS)
	{
		purple_account_set_string(account, "transport", "tls");
	}
	else
	{
		purple_account_set_string(account, "transport", "auto");
	}
	if (0 == templateId.compare(CAPABILITY_QQ))
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
	if (0 == templateId.compare(CAPABILITY_FACEBOOK))
	{
		//Don't load chat history
		purple_account_set_bool(account, "facebook_show_history", false);
	}
	if (0 == templateId.compare(CAPABILITY_XFIRE))
	{
		XFireversion = GetStringPreference("XFireVersion", templateId, username);
		purple_account_set_int(account, "version", atoi(XFireversion));
	}
		
	if (0 == templateId.compare(CAPABILITY_SIPE))
	{
		SIPEUserAgent = GetStringPreference("SIPEUserAgent", templateId, username);
		SIPEServerProxy = GetBoolPreference("SIPEServerProxy", templateId, username);
		
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
	if (0 == templateId.compare(CAPABILITY_SAMETIME))
	{
		SametimehideID = GetBoolPreference("SametimeHideID", templateId, username);
		purple_account_set_bool(account, "fake_client_id", SametimehideID);
	}
}

void LibpurpleAdapterPrefs::init()
{
	//Initialise hash tables
	s_AccountAliases = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_AccountAvatars = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_AccountBadCert = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_ServerName = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_ServerPort = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_ServerTLS = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_LoginTimeOut = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_BuddyListTimeOut = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	
	s_SametimeHideID = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_SIPEServerProxy = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_SIPEUserAgent = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_XFireVersion = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_SIPEServerLogin = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	s_JabberResource = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	
	LibpurpleAdapter::assignPrefsHandler(this);
}
