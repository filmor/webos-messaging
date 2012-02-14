/*
 * IMAccountValidatorHandler.h
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

#ifndef IMACCOUNTVALIDATORHANDLER_H_
#define IMACCOUNTVALIDATORHANDLER_H_

#include "core/MojService.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbServiceClient.h"
#include "connection.h"
#include "account.h"

// forward declaration to avoid circular #include refs
class IMAccountValidatorApp;

#define PURPLE_AIM "prpl-aim"
#define PURPLE_ICQ "prpl-icq"
#define TEMPLATE_AIM "com.palm.aol"
#define TEMPLATE_ICQ "com.palm.icq"
#define PURPLE_CALLBACK(func) ((PurpleCallback)func)
#define UI_ID        "accountValidator"

// Account service error return values
enum AccountErrorCode {
	UNKNOWN_ERROR = -3141601,             // geneneric fallback error.
	UNAUTHORIZED_401 = -3141602,          // Corresponds to HTTP 401: invalid credentials.
	TIMEOUT_408 = -3141603, 	          // Corresponds to HTTP 408: request timeout.
	SERVER_ERROR_500 = -3141604,	      // Corresponds to HTTP 500: general server error.
	SERVICE_UNAVAILABLE_503 = -3141605,   // Corresponds to HTTP 503: server is (temporarily) unavailable.
	PRECONDITION_FAILED_412 = -3141606,   // Corresponds to HTTP 412: precondition failed. The client request is not suitable for current configuration
	BAD_REQUEST_400 = -3141607,           // Corresponds to HTTP 400: Bad request
	HOST_NOT_FOUND = -3141608,            // host not found
	CONNECTION_TIMEOUT = -3141609,        // connection timeout
	CONNECTION_FAILED = -3141610,         // misc connection failed
	NO_CONNECTIVITY = -3141611,           // no connectivity
	SSL_CERT_EXPIRED = -3141612,          // SSL certificate expired
	SSL_CERT_UNTRUSTED = -3141613,        // SSL certificate untrusted
	SSL_CERT_INVALID = -3141614,          // SSL certificate invalid
	SSL_CERT_HOSTNAME_MISMATCH = -3141615,// SSL certificate hostname mismatch
	DUPLICATE_ACCOUNT =	-3141616 	      // Attempted, but refused to add a duplicate account.
};

class IMAccountValidatorHandler : public MojService::CategoryHandler
{
public:

	IMAccountValidatorHandler(MojService* service);
	virtual ~IMAccountValidatorHandler();
	MojErr init(IMAccountValidatorApp* const app);

	static MojChar* getPrplFriendlyUsername(const MojChar* serviceName, const MojChar* username);
	static MojErr privateLogMojObjectJsonString(const MojChar* format, const MojObject mojObject);
	static AccountErrorCode getAccountErrorForPurpleError(PurpleConnectionError purpleErr);

	// Libpurple callbacks
	static void accountLoggedIn(PurpleConnection* gc, gpointer accountValidator);
	static void accountSignedOff(PurpleConnection* gc, gpointer accountValidator);
	static void accountLoginFailed(PurpleConnection* gc, PurpleConnectionError error, const gchar* description,	gpointer accountValidator);

	PurpleAccount* getAccount() { return m_account; }

	void returnValidateSuccess();
	void returnValidateFailed(MojErr err, MojString errorText);
	void returnLogoutSuccess();

private:

	MojDbClient::Signal::Slot<IMAccountValidatorHandler> m_logoutSlot;
	MojErr logoutResult(MojObject& result, MojErr err);

	// Pointer to the service used for creating/sending requests.
	MojService*	m_service;

	// account we are validating
	PurpleAccount* m_account;

	// service message to reply back to
	MojRefCountedPtr <MojServiceMessage> m_serviceMsg;
	MojRefCountedPtr <MojServiceMessage> m_logoutServiceMsg;
	MojString m_password;
	MojString m_mojoUsername;

	MojErr validateAccount(MojServiceMessage* serviceMsg, const MojObject payload);
	MojErr logout(MojServiceMessage* serviceMsg, const MojObject payload);
	MojErr getMojoFriendlyUsername(const char* serviceName, const char* username);

	IMAccountValidatorApp* m_clientApp;

	static const Method s_methods[];
};

#endif /* IMACCOUNTVALIDATORHANDLER_H_ */
