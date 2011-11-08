/*
 * PalmCommon.h
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
 */

#ifndef PALMIMCOMMON_H_
#define PALMIMCOMMON_H_


#define LOGIN_STATE_OFFLINE  			"offline"
#define LOGIN_STATE_LOGGING_ON  		"logging-on"
#define LOGIN_STATE_GETTING_BUDDIES  	"retrieving-buddies"
#define LOGIN_STATE_ONLINE   			"online"
#define LOGIN_STATE_LOGGING_OFF  		"logging-off"


#define SERVICENAME_AIM "type_aim"
#define SERVICENAME_GTALK "type_gtalk"
#define SERVICENAME_ICQ "type_icq"
#define SERVICENAME_YAHOO "type_yahoo"
#define SERVICENAME_SKYPE "type_skype"
#define SERVICENAME_WINOWSLIVE "type_msn"

// capability providor ids
#define CAPABILITY_GTALK "com.palm.google.talk"
#define CAPABILITY_AIM "com.palm.aol.aim"


typedef struct
{
	const char *accountId;
	const char *serviceName;
	const char *username;
	const char *password;
	int availability;
	const char *customMessage;
	const char *localIpAddress;
	const char *connectionType;
} LoginParams;


class PalmAvailability {
public:
	static const unsigned int ONLINE = 0;
	static const unsigned int MOBILE = 1;
	static const unsigned int IDLE = 2; // Also BUSY
	static const unsigned int INVISIBLE = 3;
	static const unsigned int OFFLINE = 4;
	static const unsigned int PENDING = 5;
	static const unsigned int NO_PRESENCE = 6;
};


// Error codes
#define ERROR_NO_ERROR              "AcctMgr_No_Error"
#define ERROR_ACCOUNT_EXISTS        "AcctMgr_Acct_Already_Exists"
#define ERROR_NO_SETUP_SERVICE      "AcctMgr_No_Setup_Service"
#define ERROR_BAD_USERNAME          "AcctMgr_Bad_Username"
#define ERROR_BAD_PASSWORD          "AcctMgr_Bad_Password"
#define ERROR_AUTHENTICATION_FAILED "AcctMgr_Bad_Authentication"
#define ERROR_NETWORK_ERROR         "AcctMgr_Network_Error"
#define ERROR_BAD_CERTIFICATE       "AcctMgr_Authentication_Bad_Certificate"
#define ERROR_CANNOT_CONNECT        "AcctMgr_Authentication_Connection_Failed"
#define ERROR_NAME_IN_USE           "AcctMgr_Name_In_Use"
#define ERROR_ACCOUNT_LOCKED        "AcctMgr_Account_Locked"
#define ERROR_SINGLE_ACCOUNT_ONLY   "AcctMgr_Single_Account_Only"
#define ERROR_GENERIC_ERROR         "AcctMgr_Generic_Error"
#define ERROR_CREDENTIALS_NOT_FOUND "AcctMgr_Credentials_Not_Found"

// account service errors
#define ACCOUNT_ERROR_UNKNOWN 	  "ERROR_UNKNOWN"
#define ACCOUNT_401_UNAUTHORIZED  "401_UNAUTHORIZED"
#define ACCOUNT_500_SERVER_ERROR  "500_SERVER_ERROR"
#define ACCOUNT_CREDENTIALS_NOT_FOUND  "CREDENTIALS_NOT_FOUND"

#endif /* PALMIMCOMMON_H_ */
