/*
 * IMServiceHandler.cpp
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
 * IMServiceHandler class is the top level signal handler for the application
 */


#include "IMServiceHandler.h"
#include "IncomingIMHandler.h"
#include "IMMessage.h"
#include "OutgoingIMCommandHandler.h"
#include "OnEnabledHandler.h"
#include "IMServiceApp.h"
#include "BuddyStatusHandler.h"
#include "IMLoginState.h"
#include "LibpurpleAdapterPrefs.h"
#include "LibpurpleAdapter.h"

#define IMVersionString  "IMLibpurpleService 7-13 12:30pm starting...."

const IMServiceHandler::Method IMServiceHandler::s_methods[] = {
	{_T("loginForTesting"), (Callback) &IMServiceHandler::loginForTesting},
	{_T("onEnabled"), (Callback) &IMServiceHandler::onEnabled},
	{_T("loginStateChanged"), (Callback) &IMServiceHandler::handleLoginStateChange},
	{_T("sendIM"), (Callback) &IMServiceHandler::IMSend}, // callback for activity manager
	{_T("sendCommand"), (Callback) &IMServiceHandler::IMSendCmd}, // callback for activity manager
	{_T("loadpreferences"), (Callback) &IMServiceHandler::loadpreferences},
	{NULL, NULL} };

IMServiceHandler::IMServiceHandler(MojService* service)
: m_service(service),
  m_dbClient(service),
  m_connectionState(service)
{
	MojLogTrace(IMServiceApp::s_log);
	m_loginState = NULL;
}

IMServiceHandler::~IMServiceHandler()
{
	MojLogTrace(IMServiceApp::s_log);
	delete m_loginState;
}


/*
 * Initialize the Service
 *
 */
MojErr IMServiceHandler::init()
{
	MojLogTrace(IMServiceApp::s_log);
	MojLogInfo(IMServiceApp::s_log, IMVersionString);

	MojErr err = addMethods(s_methods);
	MojErrCheck(err);

	// set up the incoming message callback pointer
	LibpurpleAdapter::assignIMServiceHandler(this);
	
	//Load Preferences
	IMServiceHandler::LoadAccountPreferences ("", "");

	return MojErrNone;
}


MojErr IMServiceHandler::onEnabled(MojServiceMessage* serviceMsg, const MojObject payload)
{
	MojRefCountedPtr<OnEnabledHandler> handler(new OnEnabledHandler(m_service));
	MojErr err = handler->start(payload);
	if (err == MojErrNone) {
		serviceMsg->replySuccess();
	} else {
		MojString error;
		MojString msg;
		MojErrToString(err, error);
		msg.format(_T("OnEnabledHandler.start() failed: error %d - %s"), err, error.data());
		MojLogError(IMServiceApp::s_log, _T("%s"), msg.data());

		serviceMsg->replyError(err);
	}
	return MojErrNone;
}

/**
 * Send IM handler.
 * callback Used by Activity Manager - gets fired from a DB watch
 *
 * parms from activity manager:
 *     {"$activity":{"activityId":1,"trigger":{"fired":true,"returnValue":true}}}
 */
MojErr IMServiceHandler::IMSend(MojServiceMessage* serviceMsg, const MojObject parms)
{
	MojLogInfo(IMServiceApp::s_log, _T("send IM request received."));

	// log the parameters
	logMojObjectJsonString(_T("IMSend parameters: %s"), parms);

	// if we were called by the activity manager we will get an activity id here to adopt
	// get the $activity object
	MojObject activityObj;
	MojInt64 activityId = 0;
	bool found = parms.get(_T("$activity"), activityObj);
	if (found)
		found = activityObj.get(_T("activityId"), activityId);
	if (!found) {
		MojString msg;
		msg.format(_T("IMSend failed: parameter has no activityId"));
		MojLogError(IMServiceApp::s_log, "%s", msg.data());
		// send error back to caller
		serviceMsg->replyError(MojErrInvalidArg, msg);
		return MojErrInvalidArg;
	}

	// check for returnValue = false in trigger parameter
	MojObject trigger;
	bool retVal = false;
	found = activityObj.get(_T("trigger"), trigger);
	if (found)
		found = trigger.get(_T("returnValue"), retVal);
	if (!found || !retVal) {
		MojString msg;
		msg.format(_T("IMSend failed: trigger does not have returnValue: true"));
		MojLogError(IMServiceApp::s_log, "%s", msg.data());
		// send error back to caller
		serviceMsg->replyError(MojErrInvalidArg, msg);
		return MojErrInvalidArg;
	}
	// create the outgoing message handler
	// This object is ref counted and will be deleted after the slot is invoked
	// Therefore we don't need to hold on to it or worry about deleting it
	MojRefCountedPtr<OutgoingIMHandler> handler(new OutgoingIMHandler(m_service, activityId));

	// query the DB for outgoing messages and send them
	MojErr err = handler->start();

	// Reply to caller
	if (err) {
		MojString error;
		MojString msg;
		MojErrToString(err, error);
		msg.format(_T("OutgoingIMHandler.start() failed: error %d - %s"), err, error.data());
		MojLogError(IMServiceApp::s_log, _T("%s"), msg.data());

		// send error back to caller
		serviceMsg->replyError(err, msg);
		return MojErrInternal;
	}


	// Reply to caller
	MojObject reply;
	reply.putString(_T("response"), _T("IMService processing outbox."));
	serviceMsg->replySuccess(reply);

	return MojErrNone;
}

/**
 * Send IM Command handler.
 * callback Used by Activity Manager - gets fired from a DB watch
 *
 * parms from activity manager:
 *     {"$activity":{"activityId":1,"trigger":{"fired":true,"returnValue":true}}}
 */
MojErr IMServiceHandler::IMSendCmd(MojServiceMessage* serviceMsg, const MojObject parms)
{
	MojLogInfo(IMServiceApp::s_log, _T("IMSendCmd request received."));

	// log the parameters
	logMojObjectJsonString(_T("IMSendCmd parameters: %s"), parms);

	// if we were called by the activity manager we will get an activity id here to adopt
	// get the $activity object
	MojObject activityObj;
	MojInt64 activityId = 0;
	bool found = parms.get(_T("$activity"), activityObj);
	if (found)
		found = activityObj.get(_T("activityId"), activityId);
	if (!found) {
		MojString msg;
		msg.format(_T("IMSendCmd failed: parameter has no activityId"));
		MojLogError(IMServiceApp::s_log, "%s", msg.data());
		// send error back to caller
		serviceMsg->replyError(MojErrInvalidArg, msg);
		return MojErrInvalidArg;
	}

	// check for returnValue = false in trigger parameter
	MojObject trigger;
	bool retVal = false;
	found = activityObj.get(_T("trigger"), trigger);
	if (found)
		found = trigger.get(_T("returnValue"), retVal);
	if (!found || !retVal) {
		MojString msg;
		msg.format(_T("IMSendCmd failed: trigger does not have returnValue: true"));
		MojLogError(IMServiceApp::s_log, "%s", msg.data());
		// send error back to caller
		serviceMsg->replyError(MojErrInvalidArg, msg);
		return MojErrInvalidArg;
	}

	// create the outgoing command handler
	// This object is ref counted and will be deleted after the slot is invoked
	// Therefore we don't need to hold on to it or worry about deleting it
	MojRefCountedPtr<OutgoingIMCommandHandler> handler(new OutgoingIMCommandHandler(m_service, activityId));

	// query the DB for outgoing commands and send them
	MojErr err = handler->start();

	if (err) {
		MojString error;
		MojString msg;
		MojErrToString(err, error);
		msg.format(_T("OutgoingIMCommandHandler.start() failed: error %d - %s"), err, error.data());
		MojLogError(IMServiceApp::s_log, _T("%s"), msg.data());
		// send error back to caller
		serviceMsg->replyError(err, msg);
		return MojErrInternal;
	}


	// Reply to caller
	MojObject reply;
	reply.putString(_T("response"), _T("IMService processing command list"));
	serviceMsg->replySuccess(reply);

	return MojErrNone;
}


/*
 * New incoming IM message
 */
bool IMServiceHandler::incomingIM(const char* serviceName, const char* username, const char* usernameFrom, const char* message)
{

	MojLogTrace(IMServiceApp::s_log);

	// log the parameters
	// don't log the message text
	MojLogInfo (IMServiceApp::s_log, _T("incomingIM - IM received. serviceName: %s username: %s usernameFrom: %s"), serviceName, username, usernameFrom);

	// no error - process the IM
	MojRefCountedPtr<IMMessage> imMessage(new IMMessage);

	// set the message fields based on the incoming parameters
	MojErr err = imMessage->initFromCallback(serviceName, username, usernameFrom, message);

	if (!err) {
		// handle the message
		MojRefCountedPtr<IncomingIMHandler> incomingIMHandler(new IncomingIMHandler(m_service));
		err = incomingIMHandler->saveNewIMMessage(imMessage);
	}
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("incomingIM failed: %d - %s"), err, error.data());
		MojErrCheck(err);
		return false;
	}

	return true;
}

/*
 * Change in buddy status
 */
bool IMServiceHandler::updateBuddyStatus(const char* accountId, const char* serviceName, const char* username, int availability,
		const char* customMessage, const char* groupName, const char* buddyAvatarLoc)
{

	MojLogTrace(IMServiceApp::s_log);

	// log the parameters
	MojLogInfo (IMServiceApp::s_log, _T("updateBuddyStatus - accountId: %s, serviceName: %s, username: %s, availability: %i, customMessage: %s, groupName: %s, buddyAvatarLoc: %s"),
			accountId, serviceName, username, availability, customMessage, groupName, buddyAvatarLoc);

	// handle the message
	MojRefCountedPtr<BuddyStatusHandler> buddyStatusHandler(new BuddyStatusHandler(m_service));
	MojErr err = buddyStatusHandler->updateBuddyStatus(accountId, serviceName, username, availability, customMessage, groupName, buddyAvatarLoc);

	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("updateBuddyStatus failed: %d - %s"), err, error.data());
		return false;
	}

	return true;
}

/*
 * Receive a request for authorization (accept/deny) from another user to add us to their buddy list
 */
bool IMServiceHandler::receivedBuddyInvite(const char* serviceName, const char* username, const char* usernameFrom, const char* customMessage)
{
	// log the parameters
	MojLogInfo (IMServiceApp::s_log, _T("receivedBuddyInvite - serviceName: %s username: %s usernameFrom: %s message: %s"), serviceName, username, usernameFrom, customMessage);

	// Create an imcommand for the application to prompt user for acceptance
	MojRefCountedPtr<BuddyStatusHandler> buddyStatusHandler(new BuddyStatusHandler(m_service));
	MojErr err = buddyStatusHandler->receivedBuddyInvite(serviceName, username, usernameFrom, customMessage);

	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("receivedBuddyInvite failed: %d - %s"), err, error.data());
		return false;
	}

	return true;
}

/*
 * Receive a decline messages from a remote user we invited to be our buddy
 *
 * 07/13/2010 - Note: This doesn't get used currently - Libpurple doesn't seem to notify us in this case - at least I can't find it!!
 */
bool IMServiceHandler::buddyInviteDeclined(const char* serviceName, const char* username, const char* usernameFrom)
{
	// log the parameters
	MojLogInfo (IMServiceApp::s_log, _T("buddyInviteDeclined - serviceName: %s username: %s usernameFrom: %s"), serviceName, username, usernameFrom);

	// need to delete buddy and contact from DB
	return true;
}


MojErr IMServiceHandler::loginForTesting(MojServiceMessage* serviceMsg, const MojObject payload)
{
	if (m_loginState == NULL) {
		m_loginState = new IMLoginState(m_service);
	}

	m_loginState->loginForTesting(serviceMsg, payload);
	return MojErrNone;
}

/*
 * watch on the loginstate table was triggered
 */
MojErr IMServiceHandler::handleLoginStateChange(MojServiceMessage* serviceMsg, const MojObject payload)
{
	// Since the activity has the requirement "internet:true", the activity includes ConnectionManager
	// details. Use this information if our connectionState isn't yet initialized.
	m_connectionState.initConnectionStatesFromActivity(payload);

	if (m_loginState == NULL) {
		m_loginState = new IMLoginState(m_service);
	}

	return m_loginState->handleLoginStateChange(serviceMsg, payload);
}

/**
 * Log the json for a MojObject - useful for debugging
 */
MojErr IMServiceHandler::logMojObjectJsonString(const MojChar* format, const MojObject mojObject) {

	if (IMServiceApp::s_log.level() <= MojLogger::LevelInfo) {
		MojString mojStringJson;
		mojObject.toJson(mojStringJson);
		MojLogInfo(IMServiceApp::s_log, format, mojStringJson.data());
	}

	return MojErrNone;
}

/**
 * Log the json for an incoming IM payload so we can troubleshoot errors in the field in
 * non debug builds.
 *
 * removes "private" data - ie. message body
 */
MojErr IMServiceHandler::privatelogIMMessage(const MojChar* format, MojObject IMObject, const MojChar* messageTextKey) {

	MojString mojStringJson;
	MojString msgText;

	// replace the message body
	// not much we can do about errors here...

	//TODO - put this back when we can control log levels centrally
	bool found = false;
	MojErr err = IMObject.del(messageTextKey, found);
	MojErrCheck(err);
	if (found) {
		msgText.assign(_T("***IM Body Removed***"));
		IMObject.put(messageTextKey, msgText);
	}

	// log it (non debug)
	IMObject.toJson(mojStringJson);
	MojLogNotice(IMServiceApp::s_log, format, mojStringJson.data());

	return MojErrNone;
}

/*
 * Load preferences for account we are logging in to
 *
 */
MojErr IMServiceHandler::loadpreferences(MojServiceMessage* serviceMsg, const MojObject payload)
{
	//Sleep for 3 secondas to ensure DB is updated
	sleep(3);
	
	//Load server and port
	MojString errorText;
	bool found = false;
	bool result = true;
	MojString templateId;
	MojString UserName;

	//Get templateId
	MojErr err = payload.get(_T("templateId"), templateId, found);
	if (!found) {
		errorText.assign(_T("Missing templateId in payload."));
		result = false;
	}
	//Get server name
	err = payload.get(_T("UserName"), UserName, found);
	if (!found) {
		errorText.assign(_T("Missing UserName in payload."));
		result = false;
	}
	
	if (!result) {
		serviceMsg->replyError(err, errorText);
		return MojErrNone;
	}
	
	//Load the prefs
	LoadAccountPreferences (templateId, UserName);
	
	//Return success
	serviceMsg->replySuccess();
	return MojErrNone;
}

MojErr IMServiceHandler::LoadAccountPreferences(const char* templateId, const char* UserName)
{
	//Assign Prefs
	MojRefCountedPtr<LibpurpleAdapterPrefs> imPrefsHandler(new LibpurpleAdapterPrefs(m_service));
	
	//Init Prefs
	imPrefsHandler->init();
	
	//Load Account Preferences
	MojErr err = imPrefsHandler->LoadAccountPreferences(templateId, UserName);
	
	return err;
}
