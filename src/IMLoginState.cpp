/*
 * IMLoginState.h
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
 * IMLoginState class handles changes in user login status
 */

#include "IMLoginState.h"
#include "db/MojDbQuery.h"
#include "PalmImCommon.h"
#include "IMDefines.h"
#include "IMServiceApp.h"
#include "ConnectionStateHandler.h"
#include "DisplayController.h"
#include "LibpurpleAdapter.h"

/*
 * IMLoginState
 */
IMLoginState::IMLoginState(MojService* service)
: m_loginStateRevision(0),
  m_signalHandler(NULL),
  m_service(service)
{
	// Register for login callbacks
	LibpurpleAdapter::assignIMLoginState(this);
	// Register for connection changed callbacks.
	ConnectionState::setLoginStateCallback(this);
}

IMLoginState::~IMLoginState()
{
	LibpurpleAdapter::assignIMLoginState(NULL);
	ConnectionState::setLoginStateCallback(NULL);
}

void IMLoginState::handlerDone(IMLoginStateHandlerInterface* handler)
{
	// this gets called from "handler's" destructor
	if (m_signalHandler != handler)
	{
		MojLogError(IMServiceApp::s_log, _T("handlerDone doesn't match. ours=%p, theirs=%p being destroyed"), m_signalHandler, handler);
	}
	else
	{
		m_signalHandler = NULL;
	}
}

void IMLoginState::loginForTesting(MojServiceMessage* serviceMsg, const MojObject payload)
{
	m_signalHandler = new IMLoginStateHandler(m_service, m_loginStateRevision, this);
	m_signalHandler->loginForTesting(serviceMsg, payload);
}

/*
 * LoginState DB watch was triggered
 */
MojErr IMLoginState::handleLoginStateChange(MojServiceMessage* serviceMsg, const MojObject payload)
{
	// we should not be orphaning an existing handler...
	if (NULL != m_signalHandler) {
		MojLogError(IMServiceApp::s_log, _T("handleLoginStateChange: called when signal handler already existed."));
	}

	m_signalHandler = new IMLoginStateHandler(m_service, m_loginStateRevision, this);
	return m_signalHandler->handleLoginStateChange(serviceMsg, payload);
}

/*
 * Connection Manager subscription watch was triggered
 */
MojErr IMLoginState::handleConnectionChanged(const MojObject& payload)
{
	//TODO: this should be its own ConnectionChangedHandler
	if (m_signalHandler == NULL)
	{
		MojLogWarning(IMServiceApp::s_log, _T("handleConnectionChanged needed to create a signal handler"));
		m_signalHandler = new IMLoginStateHandler(m_service, m_loginStateRevision, this);
	}
	return m_signalHandler->handleConnectionChanged(payload);
}

/*
 * Callback from libpurple when the account is signed on or off
 */
void IMLoginState::loginResult(const char* serviceName, const char* username, LoginResult type, bool loggedOut, const char* errCode, bool noRetry)
{
	if (m_signalHandler == NULL)
	{
		MojLogWarning(IMServiceApp::s_log, _T("loginResult needed to create a signal handler"));
		m_signalHandler = new IMLoginStateHandler(m_service, m_loginStateRevision, this);
	}
	m_signalHandler->loginResult(serviceName, username, type, loggedOut, errCode, noRetry);
}

/*
 * Callback from libpurple when buddy list is synced
 */
void IMLoginState::buddyListResult(const char* serviceName, const char* username, MojObject& buddyList, bool fullList)
{
	// fullList == false means it is just changes to an individual buddy
	if (fullList == false)
	{
		MojLogError(IMServiceApp::s_log, _T("buddyListResult: fullList is false. Doing nothing."));
		//TODO: Query com.palm.contact for this buddy to see if it needs to be updated (picture or name changed)
		//TODO: Merge (with query) com.palm.imbuddystatus to update this buddy's availability and custom message
	}
	else
	{
		if (m_signalHandler == NULL)
		{
			MojLogError(IMServiceApp::s_log, _T("buddyListResult called but m_signalHandler == NULL. Doing nothing."));
		}
		else
		{
			m_signalHandler->fullBuddyListResult(serviceName, username, buddyList);
		}
	}
}


bool IMLoginState::getLoginStateData(const MojString& key, LoginStateData& state)
{
	bool found = (m_loginState.find(key) != m_loginState.end());
	if (found)
	{
		state = m_loginState[key];
	}
	return found;
}

bool IMLoginState::getLoginStateData(const MojString& serviceName, const MojString& username, LoginStateData& state)
{
	std::map<MojString, LoginStateData>::iterator itr = m_loginState.begin();
	bool found = false;
	while (!found && itr != m_loginState.end())
	{
		state = itr->second;
		found = (state.getServiceName() == serviceName && state.getUsername() == username);
		if (!found)
		{
			itr++;
		}
	}

	return found;
}

void IMLoginState::putLoginStateData(const MojString& key, LoginStateData& newState)
{
	m_loginState[key] = newState;
}


/*
 * IMLoginStateHandler -- signal handler
 */
IMLoginStateHandler::IMLoginStateHandler(MojService* service, MojInt64 loginStateRevision, IMLoginState* loginStateController)
: m_activityAdoptSlot(this, &IMLoginStateHandler::activityAdoptResult),
  m_activityCompleteSlot(this, &IMLoginStateHandler::activityCompleteResult),
  m_setWatchSlot(this, &IMLoginStateHandler::setWatchResult),
  m_loginStateQuerySlot(this, &IMLoginStateHandler::loginStateQueryResult),
  m_getCredentialsSlot(this, &IMLoginStateHandler::getCredentialsResult),
  m_updateLoginStateSlot(this, &IMLoginStateHandler::updateLoginStateResult),
  m_ignoreUpdateLoginStateSlot(this, &IMLoginStateHandler::ignoreUpdateLoginStateResult),
  m_queryForContactsSlot(this, &IMLoginStateHandler::queryForContactsResult),
  m_queryForBuddyStatusSlot(this, &IMLoginStateHandler::queryForBuddyStatusResult),
  m_markBuddiesAsOfflineSlot(this, &IMLoginStateHandler::markBuddiesAsOfflineResult),
  m_moveMessagesToPendingSlot(this, &IMLoginStateHandler::moveMessagesToPendingResult),
  m_moveCommandsToPendingSlot(this, &IMLoginStateHandler::moveCommandsToPendingResult),
  m_service(service),
  m_dbClient(service, MojDbServiceDefs::ServiceName),
  m_tempdbClient(service, MojDbServiceDefs::TempServiceName),
  m_loginStateController(loginStateController),
  m_activityId(-1),
  m_workingLoginStateRev(loginStateRevision),
  m_buddyListConsolidator(NULL)
{
}


IMLoginStateHandler::~IMLoginStateHandler()
{
	// Tell IMLoginState that this handler is destroyed
	m_loginStateController->handlerDone(this);
}

void IMLoginStateHandler::loginForTesting(MojServiceMessage* serviceMsg, const MojObject payload)
{
	LoginParams loginParams;

	// your username and password go here!
	loginParams.username = "xxx@aol.com";
	loginParams.password = "xxx";
	loginParams.serviceName = "type_aim";

	loginParams.availability = PalmAvailability::ONLINE;
	loginParams.customMessage = "";
	loginParams.connectionType = "wan";
	loginParams.localIpAddress = NULL;
	loginParams.accountId = "";
	LibpurpleAdapter::login(&loginParams, m_loginStateController);
	serviceMsg->replySuccess();
}
/*
 * This is an entry point into the IMLoginStateHandler machine. This is triggered when the db changes.
 */
MojErr IMLoginStateHandler::handleLoginStateChange(MojServiceMessage* serviceMsg, const MojObject payload)
{
	MojLogTrace(IMServiceApp::s_log);

	m_activityId = -1;
	// get the $activity object
	MojObject activityObj;
	bool found = payload.get("$activity", activityObj);
	if (found)
	{
		found = activityObj.get("activityId", m_activityId);
	}

	if (!found)
	{
		MojLogError(IMServiceApp::s_log, _T("handleLoginStateChange parameter has no activityId"));
		//TODO No activity, so query the activity manager to see if we've got one. If not, create/start one.
		// For now, just do the query
		queryLoginState();
	}
	else
	{
		adoptActivity();
	}

	serviceMsg->replySuccess();
	return MojErrNone;
}

/*
 * This is an entry point into the IMLoginStateHandler machine. This is triggered by connection changes
 */
MojErr IMLoginStateHandler::handleConnectionChanged(const MojObject payload)
{
	MojLogTrace(IMServiceApp::s_log);
	MojErr err;
	// Things to check:
	// + no connection - ensure all logged out
	// + specific interface dropped - ensure all accounts on that interface are logged off
	// + wifi is available - log out any accounts on wan so they switch to wifi

	// If there's not internet connection, set all records to offline
	bool wanConnected = ConnectionState::wanConnected();
	bool wifiConnected = ConnectionState::wifiConnected();
	if (!ConnectionState::hasInternetConnection())
	{
		MojLogInfo(IMServiceApp::s_log, _T("handleConnectionChanged no connection. setting all accounts offline"));
		MojDbQuery query; // intentionally empty query since we want all records changed
		query.from(IM_LOGINSTATE_KIND);
		MojObject mergeProps;
		mergeProps.putString("state", LOGIN_STATE_OFFLINE);
		mergeProps.putString("ipAddress", "");
		err = m_dbClient.merge(m_ignoreUpdateLoginStateSlot, query, mergeProps);

		// Mark all buddies so they look offline to us
		MojString empty;
		empty.assign("");
		markBuddiesAsOffline(empty);

		// Also tell libpurple to disconnect
		LibpurpleAdapter::deviceConnectionClosed(true, NULL);
	}
	// Two reasons for disconnecting from WAN: it went down or WiFi came up.
	// WiFi connection is preferred over WAN
	else if (wanConnected == false || (wanConnected == true && wifiConnected == true))
	{
		//NOTE don't need to mark the buddies as offline since we're just switching to WiFi

		MojLogInfo(IMServiceApp::s_log, _T("handleConnectionChanged wan off, but wifi on. setting all wan accounts offline"));
		MojDbQuery query;
		query.where("ipAddress", MojDbQuery::OpEq, ConnectionState::wanIpAddress());
		query.from(IM_LOGINSTATE_KIND);
		MojObject mergeProps;
		mergeProps.putString("state", LOGIN_STATE_OFFLINE);
		mergeProps.putString("ipAddress", "");
		err = m_dbClient.merge(m_ignoreUpdateLoginStateSlot, query, mergeProps);

		// Also tell libpurple to disconnect
		LibpurpleAdapter::deviceConnectionClosed(false, ConnectionState::wanIpAddress());
	}
	else if (wifiConnected == false)
	{
		//NOTE don't need to mark the buddies as offline since we're just switching to WAN

		MojLogInfo(IMServiceApp::s_log, _T("handleConnectionChanged wifi off, setting them offline"));
		MojDbQuery query;
		query.where("ipAddress", MojDbQuery::OpEq, ConnectionState::wifiIpAddress());
		query.from(IM_LOGINSTATE_KIND);
		MojObject mergeProps;
		mergeProps.putString("state", LOGIN_STATE_OFFLINE);
		mergeProps.putString("ipAddress", "");
		err = m_dbClient.merge(m_ignoreUpdateLoginStateSlot, query, mergeProps);

		// Also tell libpurple to disconnect
		LibpurpleAdapter::deviceConnectionClosed(false, ConnectionState::wifiIpAddress());
	}

	return MojErrNone;
}

MojErr IMLoginStateHandler::activityAdoptResult(MojObject& payload, MojErr resultErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (resultErr != MojErrNone)
	{
		// adopt failed
		MojString error;
		MojErrToString(resultErr, error);
		MojLogError(IMServiceApp::s_log, _T("activity manager adopt FAILED. error %d - %s"), resultErr, error.data());
	}
	else
	{
		//IMServiceHandler::logMojObjectJsonString(_T("activityAdoptResult payload: %s"), payload);
		bool adopted = false;
		payload.get("adopted", adopted); //TODO: check for "orphan"?

		// we currently get 2 notifications from the adopt call. ignore the first one
		if (adopted)
		{
			MojLogInfo(IMServiceApp::s_log, _T("Activity adopted. Querying Login State"));
			queryLoginState();
		}
	}

	return MojErrNone;
}

MojErr IMLoginStateHandler::activityCompleteResult(MojObject& payload, MojErr resultErr)
{
	//TODO: log if there are errors
	m_activityAdoptSlot.cancel();
	//setWatch();
	return MojErrNone;
}

MojErr IMLoginStateHandler::setWatchResult(MojObject& payload, MojErr resultErr)
{
	//TODO: log if there are errors
	return MojErrNone;
}

MojErr IMLoginStateHandler::loginStateQueryResult(MojObject& payload, MojErr resultErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (resultErr != MojErrNone)
	{
		MojString error;
		MojErrToString(resultErr, error);
		MojLogError(IMServiceApp::s_log, _T("loginStateQuery failed. error %d - %s"), resultErr, error.data());
	}
	else
	{
		IMServiceHandler::logMojObjectJsonString(_T("loginStateQuery success: %s"), payload);
		// result is in the form {results:[{_id, _rev, username, serviceName, state, availability, customMessage}]}
		MojObject results;
		payload.get("results", results);

		processLoginStates(results);
	}

	return MojErrNone;
}


MojErr IMLoginStateHandler::updateLoginStateResult(MojObject& payload, MojErr resultErr)
{
	// DB state is updated so complete the activity and reset the watch so it
	// can fire again and start the next state transition
	completeAndResetWatch();
	return MojErrNone;
}

MojErr IMLoginStateHandler::ignoreUpdateLoginStateResult(MojObject& payload, MojErr resultErr)
{
	// This is for intermediate changes to login state (like "logging in") so the result
	// is not actionable.
	if (resultErr != MojErrNone)
	{
		MojString error;
		MojErrToString(resultErr, error);
		MojLogError(IMServiceApp::s_log, _T("ERROR Intermediate UpdateLoginState. error %d - %s"), resultErr, error.data());
	}
	return MojErrNone;
}

// Set the state & availability to offline and set errorCode
// dbmerge will then complete and reset the activity
MojErr IMLoginStateHandler::handleBadCredentials(const MojString& serviceName, const MojString& username, const char* err)
{
	MojDbQuery query;
	query.where("serviceName", MojDbQuery::OpEq, serviceName);
	query.where("username", MojDbQuery::OpEq, username);
	query.from(IM_LOGINSTATE_KIND);
	MojObject mergeProps;
	mergeProps.putInt("availability", PalmAvailability::OFFLINE);
	mergeProps.putString("state", LOGIN_STATE_OFFLINE);
	mergeProps.putString("ipAddress", "");
	if (err != NULL)
	{
		mergeProps.putString("errorCode", err);
	}
	m_dbClient.merge(m_updateLoginStateSlot, query, mergeProps);

	// update the syncState record for this account so account dashboard can display errors
	// first we need to find our account id
	LoginStateData state;
	MojString accountId, service, user;
	service.assign(serviceName);
	user.assign(username);
	bool found = m_loginStateController->getLoginStateData(service, user, state);
	if (found) {
		accountId = state.getAccountId();
		MojRefCountedPtr<IMLoginSyncStateHandler> syncStateHandler(new IMLoginSyncStateHandler(m_service));
		syncStateHandler->updateSyncStateRecord(serviceName, accountId, LoginCallbackInterface::LOGIN_FAILED, err);
	}
	else {
		MojLogError(IMServiceApp::s_log, _T("loginResult: could not find account Id in cached login states map. No syncState record created."));
		// can we do anything here??
	}

	return MojErrNone;
}

MojErr IMLoginStateHandler::getCredentialsResult(MojObject& payload, MojErr resultErr)
{
	MojLogError(IMServiceApp::s_log, _T("getCredentialsResult:  wanConnected:%d  wifiConnected:%d"),ConnectionState::wanConnected(),ConnectionState::wifiConnected());
	if (resultErr != MojErrNone)
	{
		// Failed to get credentials, so log the issue and set the state to offlnie
		MojString serviceName = m_workingLoginState.getServiceName();
		MojString username = m_workingLoginState.getUsername();

		MojString error;
		MojErrToString(resultErr, error);
		MojLogError(IMServiceApp::s_log, _T("Failed to get credentials on %s. error %d - %s"), serviceName.data(), resultErr, error.data());

		handleBadCredentials(serviceName, username, ERROR_BAD_PASSWORD);
	}
	// About to login so this is the time to make a final check for internet connection
	else if (!ConnectionState::hasInternetConnection())
	{
		MojLogInfo(IMServiceApp::s_log, _T("No internet connection available!"));
		// No internet so mark this activity complete and reset the watch
		// which will fire next time there's a stable connection
		completeAndResetWatch();
	}
	else
	{
		//TODO: get rid of m_workingLoginState: what in the credentials response can tie it back to its corresponding m_loginState?
		
		// Now get the login params and request login
		MojObject credentials;
		MojErr err = payload.getRequired("credentials", credentials);

		LoginParams loginParams;
		MojString password;
		err = credentials.getRequired("password", password);
		if (password.empty())
		{
			MojLogError(IMServiceApp::s_log, _T("Password is empty. I think this is not ok."));
		}

		MojString serviceName = m_workingLoginState.getServiceName();
		MojString username = m_workingLoginState.getUsername();

		MojString connectionType, localIpAddress;
		ConnectionState::getBestConnection(connectionType, localIpAddress);
		// We're about to log in, so set the state to "logging in"
		updateLoginStateNoResponse(serviceName, username, LOGIN_STATE_LOGGING_ON, localIpAddress.data());

		loginParams.password = password.data();
		loginParams.accountId = m_workingLoginState.getAccountId().data();
		loginParams.username = username.data();
		loginParams.serviceName = serviceName.data();
		loginParams.availability = m_workingLoginState.getAvailability();
		loginParams.customMessage = m_workingLoginState.getCustomMessage();
		loginParams.connectionType = connectionType.data();
		loginParams.localIpAddress = localIpAddress.data();

		// Login may be asynchronous with the result callback in loginResult().
		// Also deal with immediate results
		LibpurpleAdapter::LoginResult result;
		result = LibpurpleAdapter::login(&loginParams, m_loginStateController);
		if (result == LibpurpleAdapter::FAILED)
		{
			handleBadCredentials(serviceName, username, ERROR_GENERIC_ERROR);
		}
		if (result == LibpurpleAdapter::INVALID_CREDENTIALS)
		{
			handleBadCredentials(serviceName, username, ERROR_AUTHENTICATION_FAILED);
		}
		else if (result == LibpurpleAdapter::ALREADY_LOGGED_IN)
		{
			MojDbQuery query;
			query.where("serviceName", MojDbQuery::OpEq, serviceName);
			query.where("username", MojDbQuery::OpEq, username);
			query.from(IM_LOGINSTATE_KIND);
			MojObject mergeProps;
			mergeProps.putString("state", LOGIN_STATE_ONLINE);
			mergeProps.putString("ipAddress", localIpAddress);
			m_dbClient.merge(m_updateLoginStateSlot, query, mergeProps);

			// update any imcommands that are in the "waiting-for-connection" status
			moveWaitingCommandsToPending();
		}
	}
	return MojErrNone;
}


MojErr IMLoginStateHandler::queryForContactsResult(MojObject& payload, MojErr resultErr)
{
	if (m_buddyListConsolidator == NULL)
	{
		MojLogError(IMServiceApp::s_log, _T("m_buddyListConsolidator is null on contact response"));
		//TODO what to do here???
	}
	else
	{
		MojObject contactArray;
		payload.getRequired("results", contactArray);
		m_buddyListConsolidator->setContacts(contactArray);
		if (m_buddyListConsolidator->isAllDataSet())
		{
			consolidateAllBuddyLists();
		}
	}
	return MojErrNone;
}


MojErr IMLoginStateHandler::queryForBuddyStatusResult(MojObject& payload, MojErr resultErr)
{
	if (m_buddyListConsolidator == NULL)
	{
		MojLogError(IMServiceApp::s_log, _T("m_buddyListConsolidator is null on buddystatus response"));
		//TODO what to do here???
	}
	else
	{
		MojObject buddyArray;
		payload.getRequired("results", buddyArray);
		m_buddyListConsolidator->setBuddyStatus(buddyArray);
		if (m_buddyListConsolidator->isAllDataSet())
		{
			consolidateAllBuddyLists();
		}
	}
	return MojErrNone;
}


MojErr IMLoginStateHandler::markBuddiesAsOffline(const MojString& serviceName, const MojString& username)
{
	MojErr err = MojErrNotFound;
	LoginStateData state;
	bool found = m_loginStateController->getLoginStateData(serviceName, username, state);
	if (found)
	{
		err = markBuddiesAsOffline(state.getAccountId());
	}
	return err;
}


MojErr IMLoginStateHandler::markBuddiesAsOffline(const MojString& accountId)
{
	//MojLogInfo(IMServiceApp::s_log, _T("IMLoginStateHandler::markBuddiesAsOffline account=%s"), accountId.data());
	// All buddies for this account get marked as offline
	MojDbQuery query;
	if (accountId.length() > 0)
	{
		query.where("accountId", MojDbQuery::OpEq, accountId);
	}
	else
	{
		MojLogInfo(IMServiceApp::s_log, _T("markBuddiesAsOffline: no accountId - marking all buddies offline"));
	}

	query.from(IM_BUDDYSTATUS_KIND);
	MojObject mergeProps;
	mergeProps.putInt("availability", PalmAvailability::OFFLINE);
	mergeProps.putString("status", "");
	return m_tempdbClient.merge(m_markBuddiesAsOfflineSlot, query, mergeProps);
}


MojErr IMLoginStateHandler::markBuddiesAsOfflineResult(MojObject& payload, MojErr resultErr)
{
	// TODO: anything to do here???
	return MojErrNone;
}


MojErr IMLoginStateHandler::moveWaitingMessagesToPending(const MojString& serviceName, const MojString& username)
{
	//MojLogInfo(IMServiceApp::s_log, _T("IMLoginStateHandler::moveWaitingMessagesToPending"));
	MojDbQuery query;
	MojString folderName, statusName;
	folderName.assign(IMMessage::folderStrings[Outbox]);
	query.where(MOJDB_FOLDER, MojDbQuery::OpEq, folderName);
	statusName.assign(IMMessage::statusStrings[WaitingForConnection]);
	query.where(MOJDB_STATUS, MojDbQuery::OpEq, statusName);

	query.from(IM_IMMESSAGE_KIND);
	MojObject mergeProps;
	mergeProps.putString(MOJDB_STATUS, IMMessage::statusStrings[Pending]);
	return m_dbClient.merge(m_moveMessagesToPendingSlot, query, mergeProps);
}


MojErr IMLoginStateHandler::moveMessagesToPendingResult(MojObject& payload, MojErr resultErr)
{
	return MojErrNone;
}

/*
 * Find any imcommands that were left in the "waiting-for-connection" status and make them pending now that we are back online
 */
MojErr IMLoginStateHandler::moveWaitingCommandsToPending()
{
	MojLogInfo(IMServiceApp::s_log, _T("IMLoginStateHandler::moveWaitingCommandsToPending"));
	MojDbQuery query;
	MojString statusName;
	statusName.assign(IMMessage::statusStrings[WaitingForConnection]);
	query.where(MOJDB_STATUS, MojDbQuery::OpEq, statusName);

	query.from(IM_IMCOMMAND_KIND);
	MojObject mergeProps;
	mergeProps.putString(MOJDB_STATUS, IMMessage::statusStrings[Pending]);
	return m_dbClient.merge(m_moveCommandsToPendingSlot, query, mergeProps);
}


MojErr IMLoginStateHandler::moveCommandsToPendingResult(MojObject& payload, MojErr resultErr)
{
	return MojErrNone;
}


MojErr IMLoginStateHandler::queryLoginState()
{
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);
	if (err)
	{
		MojLogError(IMServiceApp::s_log, _T("IMLoginStateHandler: create activity manager adopt request failed"));
	}
	else
	{
		MojDbQuery query;
		getLoginStateQuery(query);
		err = m_dbClient.find(m_loginStateQuerySlot, query, false, false);
		if (err)
		{
			MojLogError(IMServiceApp::s_log, _T("handleLoginStateChange query failed"));
		}
	}

	return err;
}


void IMLoginStateHandler::logLoginStates(const LoginStateData& cachedState, const LoginStateData& newState)
{
	MojString stateString;
	cachedState.toString(stateString);
	//MojLogInfo(IMServiceApp::s_log, _T("cachedState: %s"), stateString.data());
	newState.toString(stateString);
	//MojLogInfo(IMServiceApp::s_log, _T("newState:    %s"), stateString.data());
}


// resultArray should be an array of db objects of the form
// {_id, _rev, username, serviceName, state, availability, customMessage}
MojErr IMLoginStateHandler::processLoginStates(MojObject& loginStateArray)
{
	MojErr err = MojErrNone;

	MojObject loginStateEntry;
	MojObject::ConstArrayIterator loginStateItr = loginStateArray.arrayBegin();
	// This shouldn't happen, but check if there's nothing to do.
	if (loginStateItr == loginStateArray.arrayEnd())
	{
		MojLogError(IMServiceApp::s_log, _T("processLoginStates had empty result set"));
		completeAndResetWatch();
	}
	else
	{
		// Just grab the first change and deal with it. If other records have changed, we'll
		// pick that up when the watch is reset. This works because the query is on _rev in
		// ascending order.
		loginStateEntry = *loginStateItr;
		LoginStateData newState;
		newState.assignFromDbRecord(loginStateEntry);

		// Update the revision number so this change is removed from the next query.
		m_loginStateController->setLoginStateRevision(newState.getRevision());
		//m_loginStateRevision = newState.getRevision();

		LoginStateData cachedState;
		bool found = m_loginStateController->getLoginStateData(newState.getKey(), cachedState);
		if (!found)
		{
			// Key not found, so assign the new state to the cached state and
			// let it proceed as normal for login (most likely) or logout
			cachedState = newState;
		}

		logLoginStates(cachedState, newState);
		m_workingLoginState = newState;
		if (newState.needsToLogin(cachedState))
		{
			// Get account info & password then start the login process.
			bool found = false;
			MojString accountId;
			err = loginStateEntry.get("accountId", accountId, found);
			if (err != MojErrNone || !found)
			{
				MojString error;
				MojErrToString(err, error);
				MojLogError(IMServiceApp::s_log, _T("ERROR accountId missing from imloginstate. error %d - %s"), err, error.data());
				// TODO: fail the record and ...
				completeAndResetWatch();
			}
			else
			{
				MojRefCountedPtr<MojServiceRequest> req;
				err = m_service->createRequest(req);
				MojObject params;
				err = params.put("accountId", accountId);
				err = params.putString("name", "common");
				err = req->send(m_getCredentialsSlot, "com.palm.service.accounts","readCredentials", params, 1);
			}
		}
		else if (newState.needsToLogoff(cachedState))
		{
			// Now log out. The result will come in loginResult() with type=SIGNED_OFF
			if (LibpurpleAdapter::logout(newState.getServiceName(), newState.getUsername(), m_loginStateController))
			{
				// Update state to show we're in the process of logging off
				updateLoginStateNoResponse(newState.getServiceName(), newState.getUsername(), LOGIN_STATE_LOGGING_OFF, NULL);
			}
			else
			{
				// Logout returns false if the account is already logged out.
				// TODO: update newState to offline to reflect the changed state
				updateLoginStateNoResponse(newState.getServiceName(), newState.getUsername(), LOGIN_STATE_OFFLINE, NULL);
				// Make the buddies for this account look offline to us
				markBuddiesAsOffline(newState.getAccountId());
				completeAndResetWatch();
			}
		}
		else if (newState.needsToGetBuddies(cachedState))
		{
			getBuddyLists(newState.getServiceName(), newState.getUsername(), newState.getAccountId());
		}
		else if (newState.hasAvailabilityChanged(cachedState) || newState.hasCustomMessageChanged(cachedState))
		{
			if (newState.hasAvailabilityChanged(cachedState))
			{
				LibpurpleAdapter::setMyAvailability(newState.getServiceName().data(), newState.getUsername().data(), newState.getAvailability());
			}

			if (newState.hasCustomMessageChanged(cachedState))
			{
				LibpurpleAdapter::setMyCustomMessage(newState.getServiceName().data(), newState.getUsername().data(), newState.getCustomMessage().data());
			}

			completeAndResetWatch();
		}
		else
		{
			// Nothing needed to be done so just reset the watch.
			completeAndResetWatch();
		}

		// Finally, add or update the cached entry to reflect whatever changed
		m_loginStateController->putLoginStateData(newState.getKey(), newState);
	}
	return err;
}


// This fires off 3 asynchronous requests with the responses being stored in m_buddyListConsolidator
// So whichever of the 3 returns last will continue the buddy list processing
MojErr IMLoginStateHandler::getBuddyLists(const MojString& serviceName, const MojString& username, const MojString& accountId)
{
	m_buddyListConsolidator = new BuddyListConsolidator(m_service, accountId);

	// Get a full list of buddies. The result is asynchronously returned via the buddyListResult() callback
	bool ok = LibpurpleAdapter::getFullBuddyList(serviceName, username);
	if (ok)
	{
		//TODO move these two slots into BuddyListConsolidator??
		MojErr err;

		// Get contacts that are buddies of this user
		MojDbQuery contactsQuery;
		contactsQuery.from(IM_CONTACT_KIND);
		contactsQuery.where("accountId", MojDbQuery::OpEq, accountId);
		err = m_dbClient.find(m_queryForContactsSlot, contactsQuery, false, false);
		if (err)
		    MojLogError(IMServiceApp::s_log, _T("getBuddyLists: query for contacts returned err: %d"),err);

		// Get buddies of this user
		MojDbQuery buddiesQuery;
		buddiesQuery.from(IM_BUDDYSTATUS_KIND);
		buddiesQuery.where("accountId", MojDbQuery::OpEq, accountId);
		err = m_tempdbClient.find(m_queryForBuddyStatusSlot, buddiesQuery, false, false);
		if (err)
			MojLogError(IMServiceApp::s_log, _T("getBuddyLists: query for buddies returned err: %d"),err);
	}
	else
	{
		//TODO: set the state to offline?? Perhaps loginstate needs a retry count.
		MojLogError(IMServiceApp::s_log, _T("getBuddyLists: getFullBuddyList return false. This is not good"));

		delete m_buddyListConsolidator;
		m_buddyListConsolidator = NULL;


		// Since it failed, we need to reset the watch ourself.
		completeAndResetWatch();
	}

	return MojErrNone;
}

MojErr IMLoginStateHandler::consolidateAllBuddyLists()
{
	if (m_buddyListConsolidator == NULL)
	{
		MojLogError(IMServiceApp::s_log, _T("m_buddyListConsolidator is null in consolidateBuddyLists(). This shouldn't be possible"));
		//TODO what to do here???
	}
	else
	{
		m_buddyListConsolidator->consolidateContacts();
		m_buddyListConsolidator->consolidateBuddyStatus();
		// The consolidator cleans itself up. Good kitty.
		m_buddyListConsolidator = NULL;
	}

	// The user is online at this point despite any errors in buddy consolidation

	// update any imcommands that are in the "waiting-for-connection" status
	moveWaitingCommandsToPending();

	MojString serviceName = m_workingLoginState.getServiceName();
	MojString username = m_workingLoginState.getUsername();
	MojDbQuery query;
	query.where("serviceName", MojDbQuery::OpEq, serviceName);
	query.where("username", MojDbQuery::OpEq, username);
	query.from(IM_LOGINSTATE_KIND);
	MojObject mergeProps;
	mergeProps.putString("state", LOGIN_STATE_ONLINE);
	return m_dbClient.merge(m_updateLoginStateSlot, query, mergeProps);
}

MojErr IMLoginStateHandler::adoptActivity()
{
	MojLogInfo(IMServiceApp::s_log, _T("Adopting activityId: %llu."), m_activityId);
	MojErr err = MojErrNone;
	if (m_activityId > 0)
	{
		MojRefCountedPtr<MojServiceRequest> req;
		MojErr err = m_service->createRequest(req);
		if (err)
		{
			MojLogError(IMServiceApp::s_log, _T("IMLoginStateHandler: create activity manager adopt request failed"));
		}
		else
		{
			MojObject adoptParams;
			adoptParams.put(_T("activityId"), m_activityId);
			adoptParams.putBool(_T("subscribe"), true);



			MojLogInfo(IMServiceApp::s_log, _T("handleLoginStateChange adopting activity id: %llu."), m_activityId);
			err = req->send(m_activityAdoptSlot, "com.palm.activitymanager", "adopt", adoptParams, MojServiceRequest::Unlimited);
			if (err)
			{
				MojLogError(IMServiceApp::s_log, _T("IMLoginStateHandler::adoptActivity send activity manager adopt request failed"));
			}
		}
	}

	return err;
}


MojErr IMLoginStateHandler::completeAndResetWatch()
{
	//MojLogInfo(IMServiceApp::s_log, _T("Completing activityId: %llu."), m_activityId);
	MojErr err = MojErrNone;

	// If there is an active activity, mark it as completed with the "restart" flag
	if (m_activityId > 0)
	{
		MojRefCountedPtr<MojServiceRequest> completeReq;
		MojErr err = m_service->createRequest(completeReq);
		MojErrCheck(err);
		MojObject completeParams;
		completeParams.put(_T("activityId"), m_activityId);
		completeParams.put(_T("restart"), true); // Important, we need the activity to restart

		// Build out the actvity's trigger
		const char* triggerJson =
				"{\"key\":\"fired\","
				"\"method\":\"palm://com.palm.db/watch\","
				"}";
		MojObject trigger;
		trigger.fromJson(triggerJson);
		MojDbQuery dbQuery;
		getLoginStateQuery(dbQuery);
		MojObject queryDetails;
		dbQuery.toObject(queryDetails);
		MojObject query;
		query.put("query", queryDetails);
		trigger.put("params", query);

		completeParams.put("trigger", trigger);

		err = completeReq->send(m_activityCompleteSlot, "com.palm.activitymanager", "complete", completeParams, 1);
		MojErrCheck(err);

		m_activityId = -1; // clear this out so we don't keep using it.
	}
	else
	{
		MojLogError(IMServiceApp::s_log, _T("completeAndResetWatch - no activity id to complete"));
//		setWatch();
	}

	return err;
}

/*
 * NOT USED
 */
//MojErr IMLoginStateHandler::setWatch()
//{
//	MojLogError(IMServiceApp::s_log, _T("setWatch 1"));
//	MojLogInfo(IMServiceApp::s_log, _T("Starting new loginstate watch"));
//	MojErr err = MojErrNone;
//
//	// Reset the activity watch
//	MojRefCountedPtr<MojServiceRequest> watchReq;
//	err = m_service->createRequest(watchReq);
//	MojErrCheck(err);
//	MojLogDebug(IMServiceApp::s_log, _T("com.palm.activitymanager/create"));
//
//	MojLogError(IMServiceApp::s_log, _T("setWatch 2"));
//	// A lot of the activity object is static data, so fill that in using hardcoded strings, then add the "trigger"
//	// Properties within the "activity" object
//
//	// Build out the actvity's trigger
//	const char* triggerJson =
//			"{\"key\":\"fired\","
//			"\"method\":\"palm://com.palm.db/watch\","
//			"}";
//	MojObject trigger;
//	trigger.fromJson(triggerJson);
//	MojDbQuery dbQuery;
//	getLoginStateQuery(dbQuery);
//	MojObject queryDetails;
//	dbQuery.toObject(queryDetails);
//	MojObject query;
//	query.put("query", queryDetails);
//	trigger.put("params", query);
//
//	// Build out the activity
//	const char* activityJson =
//			"{\"name\":\"Libpurple loginstate\","
//			"\"description\":\"Watch for changes to the imloginstate\","
//			"\"type\": {\"foreground\": true},"
//			"\"callback\":{\"method\":\"palm://com.palm.imlibpurple/loginStateChanged\"}"
//			"}";
//	MojObject activity;
//	activity.fromJson(activityJson);
//	activity.put("trigger", trigger);
//
//	// requirements for internet:true
//	MojObject requirements;
//	err = requirements.put("internet", true);
//	MojErrCheck(err);
//	err = activity.put("requirements", requirements);
//	MojErrCheck(err);
//
//	MojObject activityCreateParams;
//	activityCreateParams.putBool("start", true);
//	activityCreateParams.put("activity", activity);
//	//TODO should this be Unlimited or just one-off???
//	err = watchReq->send(m_setWatchSlot, "com.palm.activitymanager", "create", activityCreateParams, MojServiceRequest::Unlimited);
//	MojErrCheck(err);
//
//	return err;
//}


// This is a callback from the LibpurpleAdapter for notification of login events (success, failed, disconnected)
void IMLoginStateHandler::loginResult(const char* serviceName, const char* username, LoginCallbackInterface::LoginResult type, bool loggedOut, const char* errCode, bool noRetry)
{
	MojErr err = MojErrNone;
	MojLogInfo(IMServiceApp::s_log, _T("loginResult: user %s, service %s, result=%d, code=%s, loggedOut=%d, noRetry=%d."), username, serviceName, type, errCode, loggedOut, noRetry);

	MojString serviceNameMoj, usernameMoj, errorCodeMoj;
	if (errCode == NULL)
	{
		// If no error code was specified, assume no error
		errorCodeMoj.assign(ERROR_NO_ERROR);
	}
	else
	{
		errorCodeMoj.assign(errCode);
	}

	// Want to merge the new state and errorCode values for the given username and serviceName
	MojDbQuery query;
	serviceNameMoj.assign(serviceName);
	usernameMoj.assign(username);
	query.where("serviceName", MojDbQuery::OpEq, serviceNameMoj);
	query.where("username", MojDbQuery::OpEq, usernameMoj);
	query.from(IM_LOGINSTATE_KIND);

	MojObject mergeProps;
	if (noRetry)
	{
		if (type == LoginCallbackInterface::LOGIN_SUCCESS)
		{
			MojLogWarning(IMServiceApp::s_log, _T("loginResult WARNING: ignoring noRetry==true because type==login_success"));
		}
		else
		{
			mergeProps.putInt("availability", PalmAvailability::OFFLINE);
		}
	}

	// Special case to handle getting logged off because of lost network connection.
	// There's a race condition where the login fails before the network manager
	// admits the connection is lost so the following prevents it from immediately
	// logging off which then triggers the db watch, causing a login to start and
	// quickly get stopped.
	if (type == LoginCallbackInterface::LOGIN_FAILED && noRetry == false)
	{
		IMLoginFailRetryHandler* handler = new IMLoginFailRetryHandler(m_service);
		mergeProps.putString("state", LOGIN_STATE_OFFLINE);
		mergeProps.put("errorCode", errorCodeMoj);
		handler->startTimerActivity(serviceNameMoj, query, mergeProps);
	}
	else
	{
		switch (type)
		{
		case LoginCallbackInterface::LOGIN_SUCCESS:
			mergeProps.putString("state", LOGIN_STATE_GETTING_BUDDIES);
			mergeProps.put("errorCode", errorCodeMoj);
			// Since the login succeeded, move any waiting messages back to pending
			moveWaitingMessagesToPending(serviceNameMoj, usernameMoj);
			break;
		case LoginCallbackInterface::LOGIN_SIGNED_OFF:
		case LoginCallbackInterface::LOGIN_FAILED:
		case LoginCallbackInterface::LOGIN_TIMEOUT:
			mergeProps.putString("state", LOGIN_STATE_OFFLINE);
			mergeProps.put("errorCode", errorCodeMoj);
			// Make the buddies for this account look offline to us
			markBuddiesAsOffline(serviceNameMoj, usernameMoj);
			break;
		}
		err = m_dbClient.merge(m_updateLoginStateSlot, query, mergeProps);
	}

	// update the syncState record for this account so account dashboard can display errors
	// first we need to find our account id
	LoginStateData state;
	MojString accountId, service, user;
	service.assign(serviceName);
	user.assign(username);
	bool found = m_loginStateController->getLoginStateData(service, user, state);
	if (found) {
		accountId = state.getAccountId();
		MojRefCountedPtr<IMLoginSyncStateHandler> syncStateHandler(new IMLoginSyncStateHandler(m_service));
		syncStateHandler->updateSyncStateRecord(serviceName, accountId, type, errorCodeMoj);
	}
	else {
		MojLogError(IMServiceApp::s_log, _T("loginResult: could not find account Id in cached login states map. No syncState record created."));
		// can we do anything here??
	}

}


// This is a callback from the LibpurpleAdapter for notification of buddyList changes
void IMLoginStateHandler::fullBuddyListResult(const char* serviceName, const char* username, MojObject& buddyList)
{

	//TODO: Update com.palm.imbuddystatus (maybe just delete all and add all new)
	if (m_buddyListConsolidator == NULL)
	{
		MojLogError(IMServiceApp::s_log, _T("m_buddyListConsolidator is null on fullBuddyListResult"));
		//TODO what to do here???
	}
	else
	{
		MojLogInfo(IMServiceApp::s_log, _T("fullBuddyListResult: setting new buddy list"));
		m_buddyListConsolidator->setNewBuddyList(buddyList);
		if (m_buddyListConsolidator->isAllDataSet())
		{
			consolidateAllBuddyLists();
		}
	}
}


MojErr IMLoginStateHandler::getLoginStateQuery(MojDbQuery& query)
{
	query.from(IM_LOGINSTATE_KIND);
	query.where("_rev", MojDbQuery::OpGreaterThan, m_workingLoginStateRev);
	MojObject queryObj;
	query.toObject(queryObj);
	IMServiceHandler::logMojObjectJsonString(_T("getLoginStateQuery: %s"), queryObj);
	return MojErrNone;
}


MojErr IMLoginStateHandler::updateLoginStateNoResponse(const MojString& serviceName, const MojString& username, const char* state, const char* ipAddress)
{
	MojDbQuery query;
	query.where("serviceName", MojDbQuery::OpEq, serviceName);
	query.where("username", MojDbQuery::OpEq, username);
	query.from(IM_LOGINSTATE_KIND);
	MojObject mergeProps;
	mergeProps.putString("state", state);
	if (ipAddress != NULL)
	{
		mergeProps.putString("ipAddress", ipAddress);
	}
	return m_dbClient.merge(m_ignoreUpdateLoginStateSlot, query, mergeProps);
}


/*
 * IMLoginState::LoginStateData
 */
MojErr LoginStateData::assignFromDbRecord(MojObject& record)
{
	MojErr err = MojErrNone;
	bool found = false;

	err = record.get("_id", m_recordId, found);
	if (err != MojErrNone || found == false)
	{
		MojLogError(IMServiceApp::s_log, _T("LoginStateData::getFromDbRecord missing _id."));
		return err;
	}

	err = record.get("_rev", m_revision, found);
	if (err != MojErrNone || found == false)
	{
		MojLogError(IMServiceApp::s_log, _T("LoginStateData::getFromDbRecord missing _rev. err=%u"), err);
		return err;
	}

	record.get("username", m_username, found);
	record.get("accountId", m_accountId, found);
	record.get("serviceName", m_serviceName, found);
	err = record.get("state", m_state, found);	
	if (err != MojErrNone || found == false)
	{
		MojLogError(IMServiceApp::s_log, _T("LoginStateData::getFromDbRecord missing state, assuming offline."));
		m_state.assign(LOGIN_STATE_OFFLINE);
	}

	// This may legitimately be missing
	record.get("customMessage", m_customMessage, found);

	err = record.get("availability", m_availability, found);
	if (err != MojErrNone || found == false)
	{
		MojLogError(IMServiceApp::s_log, _T("LoginStateData::getFromDbRecord missing availability, assuming offline."));
		m_availability = PalmAvailability::OFFLINE;
	}
	return err;
}

bool LoginStateData::needsToLogin(LoginStateData& oldState)
{
	return (m_state == LOGIN_STATE_OFFLINE && m_availability != PalmAvailability::OFFLINE && m_availability != PalmAvailability::NO_PRESENCE);
}

bool LoginStateData::needsToGetBuddies(LoginStateData& oldState)
{
	return (m_state == LOGIN_STATE_GETTING_BUDDIES && m_availability != PalmAvailability::OFFLINE && m_availability != PalmAvailability::NO_PRESENCE && (oldState.m_state == LOGIN_STATE_OFFLINE || oldState.m_state == LOGIN_STATE_LOGGING_ON));
}

bool LoginStateData::needsToLogoff(LoginStateData& oldState)
{
	return (m_state != LOGIN_STATE_OFFLINE && m_availability == PalmAvailability::OFFLINE);
}

bool LoginStateData::hasAvailabilityChanged(LoginStateData& oldState)
{
	return (m_availability != oldState.m_availability);
}

bool LoginStateData::hasCustomMessageChanged(LoginStateData& newState)
{
	return (m_customMessage != newState.m_customMessage);
}


/*
 * IMLoginFailRetryHandler -- signal handler
 */
IMLoginFailRetryHandler::IMLoginFailRetryHandler(MojService* service)
: m_activitySubscriptionSlot(this, &IMLoginFailRetryHandler::activitySubscriptionResult),
  m_dbmergeSlot(this, &IMLoginFailRetryHandler::dbmergeResult),
  m_activityCompleteSlot(this, &IMLoginFailRetryHandler::activityCompleteResult),
  m_service(service),
  m_dbClient(service, MojDbServiceDefs::ServiceName),
  m_tempdbClient(service, MojDbServiceDefs::TempServiceName),
  m_activityId(-1)
{
}

MojErr IMLoginFailRetryHandler::startTimerActivity(const MojString& serviceName, const MojDbQuery& query, const MojObject& mergeProps)
{
	//MojLogError(IMServiceApp::s_log, _T("**********IMLoginFailRetryHandler::startTimerActivity"));
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);
	if (err != MojErrNone)
	{
		MojLogError(IMServiceApp::s_log, _T("IMLoginFailRetryHandler createRequest failed. error %d"), err);
	}
	else
	{
		m_query = query;
		m_mergeProps = mergeProps;
		// This is what the activity looks like
		// {
		//  "name":"Set IMLoginState Offline " + serviceName,
		//  "description":"Scheduled request to mark the service as offline",
		//  "type":{"foreground":true},
		//  "schedule":{
		//      "start":<current date/time> + 2s,
		//  }
		// }
		// activity
		MojObject activity;
		MojString activityName;
		activityName.assign(_T("Set IMLoginState Offline "));
		activityName.append(serviceName);
		activity.putString("name", activityName);
		activity.putString("description", _T("Scheduled request to mark the service as offline"));

		// activity.type
		MojObject activityType;
		activityType.putBool("foreground", true);
		activity.put("type", activityType);

		// activity.schedule
		time_t targetDate;
		time(&targetDate);
		targetDate += 2; // schedule for 2 seconds in the future
		tm* ptm = gmtime(&targetDate);
		char scheduleTime[50];
		sprintf(scheduleTime, "%d-%02d-%02d %02d:%02d:%02dZ", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		MojLogDebug(IMServiceApp::s_log, _T("IMLoginFailRetryHandler: com.palm.activitymanager/create date=%s"), scheduleTime);
		MojObject scheduleObj;
		scheduleObj.putString("start", scheduleTime);
		activity.put("schedule", scheduleObj);

		MojObject params;
		params.putBool("start", true);
		params.putBool("subscribe", true);
		params.put("activity", activity);

		err = req->send(m_activitySubscriptionSlot, "com.palm.activitymanager", "create", params, MojServiceRequest::Unlimited);
	}

	return err;
}

// Expected responses
// {"activityId":78,"returnValue":true}
// {"activityId":78,"event":"start","returnValue":true}  <-- this occurs when the timeout fires
// Error in case there is already an activity for the given serviceName
// {"errorCode":17,"errorText":"Activity with that name already exists","returnValue":false}
MojErr IMLoginFailRetryHandler::activitySubscriptionResult(MojObject& result, MojErr err)
{
	MojLogDebug(IMServiceApp::s_log, _T("IMLoginFailRetryHandler::activitySubscriptionResult begin err=%d"), err);

	MojString event;
	if (err == MojErrNone && result.getRequired(_T("event"), event) == MojErrNone)
	{
		if (event == "start")
		{
			bool found = result.get("activityId", m_activityId);
			if (!found)
				MojLogError(IMServiceApp::s_log, _T("activitySubscriptionResult: parameter has no activityId"));
			err = m_dbClient.merge(m_dbmergeSlot, m_query, m_mergeProps);
			//TODO also markBuddiesAsOffline? Shouldn't need to since lost connection should do that later
		}
	}
	return err;
}

MojErr IMLoginFailRetryHandler::dbmergeResult(MojObject& result, MojErr err)
{
	if (m_activityId > 0)
	{
		MojRefCountedPtr<MojServiceRequest> req;
		err = m_service->createRequest(req);
		MojObject completeParams;
		completeParams.put(_T("activityId"), m_activityId);
		err = req->send(m_activityCompleteSlot, "com.palm.activitymanager", "complete", completeParams, 1);
	}
	return err;
}

MojErr IMLoginFailRetryHandler::activityCompleteResult(MojObject& result, MojErr err)
{
	m_activitySubscriptionSlot.cancel();
	return err;
}


/*
 * IMLoginSyncStateHandler -- signal handler to update syncState
 */
IMLoginSyncStateHandler::IMLoginSyncStateHandler(MojService* service)
: m_removeSyncStateSlot(this, &IMLoginSyncStateHandler::removeSyncStateResult),
  m_saveSyncStateSlot(this, &IMLoginSyncStateHandler::saveSyncStateResult),
  m_service(service),
  m_tempdbClient(service, MojDbServiceDefs::TempServiceName)
{
}

/*
 * Update the syncState record in temp DB
 *     delete any existing syncState record. Covers the case when login succeeds
 *     if the login failed due to bad password, need to add a syncState record with the correct account error code
 */
void IMLoginSyncStateHandler::updateSyncStateRecord(const char* serviceName, MojString accountId, LoginCallbackInterface::LoginResult type, const char* errCode)
{
	// syncState record:
		/*
		{
		"_kind": "com.palm.account.syncstate:1",
		"accountId": "++HBI8gkY38GlaVk",   LoginStateData.getAccountId()
		"busAddress": "com.palm.imlibpurple"
		"capabilityProvider": "com.palm.google.talk" or "com.palm.aol.aim"
		"errorCode": "401_UNAUTHORIZED",
		"errorText": "Incorrect Password",
		"syncState": "ERROR"
		 }
		 */


	// get the capabilityProvidor id from the service
	// TODO - should read this out of template ID...
	if (strcmp(serviceName, SERVICENAME_AIM) == 0) {
		m_capabilityId.assign(CAPABILITY_AIM);
	}
	else if (strcmp(serviceName, SERVICENAME_FACEBOOK) == 0){
		m_capabilityId.assign(CAPABILITY_FACEBOOK);
	}
	else if (strcmp(serviceName, SERVICENAME_GTALK) == 0){
		m_capabilityId.assign(CAPABILITY_GTALK);
	}
	else if (strcmp(serviceName, SERVICENAME_GADU) == 0){
		m_capabilityId.assign(CAPABILITY_GADU);
	}
	else if (strcmp(serviceName, SERVICENAME_GROUPWISE) == 0){
		m_capabilityId.assign(CAPABILITY_GROUPWISE);
	}
	else if (strcmp(serviceName, SERVICENAME_ICQ) == 0){
		m_capabilityId.assign(CAPABILITY_ICQ);
	}
	else if (strcmp(serviceName, SERVICENAME_JABBER) == 0){
		m_capabilityId.assign(CAPABILITY_JABBER);
	}
	else if (strcmp(serviceName, SERVICENAME_LIVE) == 0){
		m_capabilityId.assign(CAPABILITY_LIVE);
	}
	else if (strcmp(serviceName, SERVICENAME_WLM) == 0){
		m_capabilityId.assign(CAPABILITY_WLM);
	}
	else if (strcmp(serviceName, SERVICENAME_MYSPACE) == 0){
		m_capabilityId.assign(CAPABILITY_MYSPACE);
	}
	else if (strcmp(serviceName, SERVICENAME_QQ) == 0){
		m_capabilityId.assign(CAPABILITY_QQ);
	}
	else if (strcmp(serviceName, SERVICENAME_SAMETIME) == 0){
		m_capabilityId.assign(CAPABILITY_SAMETIME);
	}
	else if (strcmp(serviceName, SERVICENAME_SIPE) == 0){
		m_capabilityId.assign(CAPABILITY_SIPE);
	}
	else if (strcmp(serviceName, SERVICENAME_XFIRE) == 0){
		m_capabilityId.assign(CAPABILITY_XFIRE);
	}
	else if (strcmp(serviceName, SERVICENAME_YAHOO) == 0){
		m_capabilityId.assign(CAPABILITY_YAHOO);
	}
	else {
		MojLogError(IMServiceApp::s_log, _T("updateSyncStateRecord: unknown serviceName %s. No syncState record created."), serviceName);
		// can we do anything here??
		return;
	}

	// save the input parameters
	m_accountId = accountId;

	// get the error code for the new record if we need it
	if (LoginCallbackInterface::LOGIN_SUCCESS != type) {

		// for now, we only show dashboard for authentication errors
		if ((strcmp(errCode, ERROR_AUTHENTICATION_FAILED) == 0) ||
			(strcmp(errCode, ERROR_BAD_USERNAME) == 0) ||
		    (strcmp(errCode, ERROR_BAD_PASSWORD) == 0))	{
			m_errorCode.assign(ACCOUNT_401_UNAUTHORIZED);
		}
	}

	// delete any existing record for this account
	// construct our where clause - find by username, accountId and servicename
	MojDbQuery query;
	query.where("capabilityProvider", MojDbQuery::OpEq, m_capabilityId);
	query.where("accountId", MojDbQuery::OpEq, m_accountId);
	query.from(IM_SYNCSTATE_KIND);

	// delete the old record. If none exists, that is OK - DB just returns a count of 0 in result
	MojErr err = m_tempdbClient.del(m_removeSyncStateSlot, query);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("updateSyncStateRecord: dbClient.del() failed: error %d - %s"), err, error.data());
	}
}

/*
 * Remove the old the syncState record result
 */
MojErr IMLoginSyncStateHandler::removeSyncStateResult(MojObject& result, MojErr resultErr)
{
	if (resultErr) {
		MojString error;
		MojErrToString(resultErr, error);
		MojLogError(IMServiceApp::s_log, _T("removeSyncStateResult: DB del failed. error %d - %s"), resultErr, error.data());
	}
	else {
		IMServiceHandler::logMojObjectJsonString(_T("removeSyncStateResult: syncState successfully removed: %s"), result);
	}

	// no error means we are done
	if (m_errorCode.empty())
		return MojErrNone;


	// otherwise add the new record
	MojObject props;
	props.putString(_T("errorCode"), m_errorCode);
	props.putString(_T("_kind"), IM_SYNCSTATE_KIND);
	props.putString(_T("accountId"), m_accountId);
	props.putString(_T("capabilityProvider"), m_capabilityId);

	// bus address
	MojString busAddr;
	busAddr.assign(_T("org.webosinternals.imlibpurple"));
	props.putString(_T("busAddress"), busAddr);

	// sync state - error
	MojString syncState;
	syncState.assign(_T("ERROR"));
	props.putString(_T("syncState"), syncState);

	// log it
	MojString json;
	props.toJson(json);
	MojLogInfo(IMServiceApp::s_log, _T("removeSyncStateResult: saving syncState to db: %s"), json.data());

	MojErr err = m_tempdbClient.put(m_saveSyncStateSlot, props);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("removeSyncStateResult: dbClient.put() failed: error %d - %s"), err, error.data());
	}

	return MojErrNone;
}

/*
 * Save the new syncState record result
 * Just log errors.
 */
MojErr IMLoginSyncStateHandler::saveSyncStateResult(MojObject& result, MojErr resultErr)
{
	if (resultErr) {
		MojString error;
		MojErrToString(resultErr, error);
		MojLogError(IMServiceApp::s_log, _T("saveSyncStateResult: DB put failed. error %d - %s"), resultErr, error.data());
	}
	else {
		IMServiceHandler::logMojObjectJsonString(_T("saveSyncStateResult: syncState successfully saved: %s"), result);
	}

	return MojErrNone;
}

