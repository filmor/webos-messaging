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

#include "Util.hpp"

#define IMVersionString  "IMAccountValidator 6-8 1pm starting...."

const IMAccountValidatorHandler::Method IMAccountValidatorHandler::s_methods[] = {
	{_T("checkCredentials"), (Callback) &IMAccountValidatorHandler::validateAccount},
    {_T("getOptions"), (Callback) &IMAccountValidatorHandler::getOptions},
    {_T("getUIEvents"), (Callback) &IMAccountValidatorHandler::getUIEvents},
    {_T("answerUIEvent"), (Callback) &IMAccountValidatorHandler::answerUIEvent},
	{_T("logout"), (Callback) &IMAccountValidatorHandler::logout},
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
    if (thread_)
    {
        thread_->interrupt();
        delete thread_;
    }
	MojLogTrace(IMAccountValidatorApp::s_log);
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

MojErr IMAccountValidatorHandler::getOptions(MojServiceMessage* serviceMsg, const MojObject payload)
{
    try
    {
        MojString prpl = Util::get(payload, "prpl");
 
        MojObject options = Util::getProtocolOptions(prpl);
        MojObject reply;
        reply.put("options", options);

        serviceMsg->replySuccess(reply);
    }
    catch (Util::MojoException const& exc)
    {
        MojString error;
        error.assign(exc.what().c_str(), exc.what().size());
        serviceMsg->replyError(MojErrInvalidArg, error);
        return MojErrInvalidArg;
    }

    return MojErrNone;
}

static void thread_func(MojServiceMessage* serviceMsg, bool const& running)
{
    MojObject elem;

    // TODO: Check if serviceMsg belongs to an open request

    while (running /* && serviceMsg->open()*/)
    {
        Purple::popEvent(elem);
        serviceMsg->reply(elem);
    }
}

MojErr IMAccountValidatorHandler::getUIEvents(MojServiceMessage* serviceMsg, const MojObject payload)
{
    // Subscription stuff
    if (running_)
    {
        serviceMsg->replyError(MojErrInvalidArg, "Already getting UI events");
    }
    else
    {
        // Stop the thread in destructor
        running_ = true;
        if (thread_)
        {
            thread_->interrupt();
            delete thread_;
        }

        thread_ = new boost::thread(thread_func, serviceMsg, running_);
    }

    return MojErrNone;
}

MojErr IMAccountValidatorHandler::answerUIEvent(MojServiceMessage* serviceMsg, const MojObject payload)
{
    unsigned id;
    MojErr err = payload.getRequired("id", id);
    if (err != MojErrNone)
    {
        serviceMsg->replyError(MojErrInvalidArg);
        return MojErrInvalidArg;
    }

    unsigned answer;
    err = payload.getRequired("answer", answer);
    if (err != MojErrNone)
    {
        serviceMsg->replyError(MojErrInvalidArg);
        return MojErrInvalidArg;
    }

    err = Purple::answerRequest(id, answer);
    if (err != MojErrNone)
    {
        serviceMsg->replyError(MojErrInvalidArg);
        return MojErrInvalidArg;
    }

    serviceMsg->replySuccess();
    return MojErrNone;
}

MojErr IMAccountValidatorHandler::validateAccount(MojServiceMessage* serviceMsg, const MojObject payload)
{
	// remember the message so we can reply back in the callback
	m_serviceMsg = serviceMsg;

	PurpleSavedStatus *status;

	// log the parameters
	privateLogMojObjectJsonString(_T("validateAccount payload: %s"), payload);

	// get the username, password, and config from the input payload
	MojString username;
    MojObject config;

    try
    {
        username = Util::get(payload, "username");
        m_password = Util::get(payload, "password");
        MojString templateId = Util::get(payload, "templateId");

        payload.get("config", config);

        m_account = Util::createPurpleAccount(username, config);
    }
    catch(Util::MojoException const& exc)
    {
        MojString error;
        error.assign(exc.what().c_str());
        returnValidateFailed(MojErrInvalidArg, error);
        m_clientApp->shutdown();
        return MojErrNone;
    }

	// Input is OK - validate the account
	MojLogInfo(IMAccountValidatorApp::s_log, "validateAccount: Logging in...username: %s, password: ****removed****", username.data());

    purple_account_set_password(m_account, m_password.data());
    /* It's necessary to enable the account first. */
    purple_account_set_enabled(m_account, UI_ID, TRUE);

   /* Now, to activate the account with status=invisible so the user doesn't briefly show as online. */
    status = purple_savedstatus_new(NULL, PURPLE_STATUS_INVISIBLE);
    purple_savedstatus_activate(status);

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
	// luna-send -n 1 palm://com.palm.imaccountvalidator/logout '{}'

	// log the send
	MojLogInfo(IMAccountValidatorApp::s_log, "Issuing logout request to imaccountvalidator");

	// Get a request object from the service
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);

	// create empty parameter object
	MojObject params;

	if (!err)
		err = req->send(m_logoutSlot, "org.webosinternals.purple.validator", "logout", params);
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

