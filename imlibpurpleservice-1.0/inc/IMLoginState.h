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

#ifndef IMLOGINSTATE_H_
#define IMLOGINSTATE_H_

#include <memory>
#include <map>
#include "core/MojService.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbServiceClient.h"
#include "LibpurpleAdapter.h"
#include "BuddyListConsolidator.h"


class LoginStateData {
public:
	MojErr assignFromDbRecord(MojObject& record);
	const MojString& getKey() { return m_recordId; }
	const MojString& getAccountId() { return m_accountId; }
	const MojUInt64 getRevision() const { return m_revision; }
	MojString getUsername() const { return m_username; }
	MojString getServiceName() const { return m_serviceName; }
	const MojUInt32 getAvailability() const { return m_availability; }
	MojString getCustomMessage() const { return m_customMessage; }
	void setState(const MojString& state) { m_state = state; }
	bool needsToLogin(LoginStateData& old);
	bool needsToGetBuddies(LoginStateData& old);
	bool needsToLogoff(LoginStateData& old);
	bool hasAvailabilityChanged(LoginStateData& old);
	bool hasCustomMessageChanged(LoginStateData& old);
	bool operator==(const LoginStateData& rhs) const { return m_recordId == rhs.m_recordId; }
	void toString(MojString& string) const {
		string.assign("recordId="); string.append(m_recordId);
		string.append(", m_accountId="); string.append(m_accountId);
		string.appendFormat(", m_revision=%llu", m_revision);
		string.append(", m_username="); string.append(m_username);
		string.append(", m_serviceName="); string.append(m_serviceName);
		string.appendFormat(", m_availability=%i", m_availability);
		string.append(", m_state="); string.append(m_state);
		string.append(", m_customMessage=\""); string.append(m_customMessage); string.append("\"");
	}
private:
	MojString m_recordId;
	MojString m_accountId;
	MojUInt64 m_revision;
	MojString m_username;
	MojString m_serviceName;
	MojUInt32 m_availability;
	MojString m_state;
	MojString m_customMessage;
};


/*
 * IMLoginStateHandlerInterface - an interface used for functions call by IMLoginState
 */
class IMLoginStateHandlerInterface {
public:
	virtual void loginForTesting(MojServiceMessage* serviceMsg, const MojObject payload) = 0;
	// This is called by the db watch on com.palm.imloginstate
	virtual MojErr handleLoginStateChange(MojServiceMessage* serviceMsg, const MojObject payload) = 0;

	virtual MojErr handleConnectionChanged(const MojObject payload) = 0;
	virtual void loginResult(const char* serviceName, const char* username, LoginCallbackInterface::LoginResult type, bool loggedOut, const char* errCode, bool noRetry) = 0;
	virtual void fullBuddyListResult(const char* serviceName, const char* username, MojObject& buddyList) = 0;
};


/*
 * IMLoginState
 */
class IMLoginState : public LoginCallbackInterface {
public:
	IMLoginState(MojService* service);
	~IMLoginState();

	void loginForTesting(MojServiceMessage* serviceMsg, const MojObject payload);
	// This is called by the db watch on com.palm.imloginstate
	MojErr handleLoginStateChange(MojServiceMessage* serviceMsg, const MojObject payload);

	MojErr handleConnectionChanged(const MojObject& payload);

	virtual void loginResult(const char* serviceName, const char* username, LoginCallbackInterface::LoginResult type, bool loggedOut, const char* errCode, bool noRetry);
	virtual void buddyListResult(const char* serviceName, const char* username, MojObject& buddyList, bool fullList);

	void handlerDone(IMLoginStateHandlerInterface* handler);

	// return false if not found
	bool getLoginStateData(const MojString& key, LoginStateData& state);
	bool getLoginStateData(const MojString& serviceName, const MojString& username, LoginStateData& state);
	void putLoginStateData(const MojString& key, LoginStateData& state);

	void setLoginStateRevision(const MojInt64 revision) { m_loginStateRevision = revision; }

private:
	MojInt64 m_loginStateRevision;
	// For now just assume 1 active login transaction at a time. Maybe change this later for performance
	IMLoginStateHandlerInterface* m_signalHandler;
	MojService*	m_service;
	std::map<MojString, LoginStateData> m_loginState;
};


/*
 * IMLoginStateHandler - signal handler
 */
class IMLoginStateHandler : public MojSignalHandler, public IMLoginStateHandlerInterface {
public:
	IMLoginStateHandler(MojService* service, const MojInt64 loginStateRevision, IMLoginState* loginStateController);
	virtual ~IMLoginStateHandler();

	void loginForTesting(MojServiceMessage* serviceMsg, const MojObject payload);

	// This is called by the db watch on com.palm.imloginstate
	virtual MojErr handleLoginStateChange(MojServiceMessage* serviceMsg, const MojObject payload);

	virtual MojErr handleConnectionChanged(const MojObject payload);

	/**
	 *  result callback used by the adapter
	 *  set noRetry=true if the account should stay offline instead of retrying the login
	 */
	virtual void loginResult(const char* serviceName, const char* username, LoginCallbackInterface::LoginResult type, bool loggedOut, const char* errCode, bool noRetry);

	/**
	 * if fullList==true, this is a complete list of buddies
	 */
	virtual void fullBuddyListResult(const char* serviceName, const char* username, MojObject& buddyList);
private:
	MojErr adoptActivity();
	MojErr completeAndResetWatch();
//	MojErr setWatch();
	MojErr queryLoginState();
	MojErr processLoginStates(MojObject& resultSet);
	MojErr getLoginStateQuery(MojDbQuery& query);
	MojErr handleBadCredentials(const MojString& serviceName, const MojString& username, const char* err);
	MojErr updateLoginStateNoResponse(const MojString& serviceName, const MojString& username, const char* state, const char* ipAddress);
	MojErr getBuddyLists(const MojString& serviceName, const MojString& username, const MojString& accountId);
	MojErr consolidateAllBuddyLists();
	MojErr markBuddiesAsOffline(const MojString& accountId);
	MojErr markBuddiesAsOffline(const MojString& serviceName, const MojString& username);
	void logLoginStates(const LoginStateData& cachedState, const LoginStateData& newState);
	MojErr moveWaitingMessagesToPending(const MojString& serviceName, const MojString& username);
	MojErr moveWaitingCommandsToPending();

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_activityAdoptSlot;
	MojErr activityAdoptResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_activityCompleteSlot;
	MojErr activityCompleteResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_setWatchSlot;
	MojErr setWatchResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_loginStateQuerySlot;
	MojErr loginStateQueryResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_getCredentialsSlot;
	MojErr getCredentialsResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_updateLoginStateSlot;
	MojErr updateLoginStateResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_ignoreUpdateLoginStateSlot;
	MojErr ignoreUpdateLoginStateResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_queryForContactsSlot;
	MojErr queryForContactsResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_queryForBuddyStatusSlot;
	MojErr queryForBuddyStatusResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_markBuddiesAsOfflineSlot;
	MojErr markBuddiesAsOfflineResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_moveMessagesToPendingSlot;
	MojErr moveMessagesToPendingResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginStateHandler> m_moveCommandsToPendingSlot;
	MojErr moveCommandsToPendingResult(MojObject& result, MojErr err);


	MojService*	m_service;
	MojDbServiceClient m_dbClient;
	MojDbServiceClient m_tempdbClient;

	IMLoginState* m_loginStateController;

	MojInt64 m_activityId;
	MojObject m_workingLoginStateRev;
	LoginStateData m_workingLoginState;
	BuddyListConsolidator* m_buddyListConsolidator;
};


/*
 * This class is used to add a small delay the logout failures caused when the
 * network connection is lost. This is desired because there is a race condition
 * where the connectionmanager may not have recognized the network loss. Since
 * the login failure is retryable, it will start an unnecessary (and futile)
 * attempt to login. This delay is enough to prevent that from happening.
 */
class IMLoginFailRetryHandler : public MojSignalHandler {
public:
	IMLoginFailRetryHandler(MojService* service);

	MojErr startTimerActivity(const MojString& serviceName, const MojDbQuery& query, const MojObject& mergeProps);

private:
	MojDbClient::Signal::Slot<IMLoginFailRetryHandler> m_activitySubscriptionSlot;
	MojErr activitySubscriptionResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginFailRetryHandler> m_dbmergeSlot;
	MojErr dbmergeResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginFailRetryHandler> m_activityCompleteSlot;
	MojErr activityCompleteResult(MojObject& result, MojErr err);

	MojService*	m_service;
	MojDbServiceClient m_dbClient;
	MojDbServiceClient m_tempdbClient;

	MojInt64 m_activityId;
	MojDbQuery m_query;
	MojObject m_mergeProps;
};

/*
 * Class used to update the syncState record in the case of a login authentication failure to accounts dashboard can show the UI
 */
class IMLoginSyncStateHandler : public MojSignalHandler {

public:
	IMLoginSyncStateHandler(MojService* service);
	void updateSyncStateRecord(const char* serviceName, MojString accountId, LoginCallbackInterface::LoginResult type, const char* errCode);


private:
	MojDbClient::Signal::Slot<IMLoginSyncStateHandler> m_removeSyncStateSlot;
	MojErr removeSyncStateResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<IMLoginSyncStateHandler> m_saveSyncStateSlot;
	MojErr saveSyncStateResult(MojObject& result, MojErr err);

	MojService*	m_service;
	MojDbServiceClient m_tempdbClient;

	MojString m_accountId;
	MojString m_capabilityId;
	MojString m_errorCode;

};


#endif /* IMLOGINSTATE_H_ */
