/*
 * ConnectionStateHandler.h
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
 * ConnectionStateHandler class handles changes between WAN and WiFi device connection
 */

#ifndef CONNECTIONSTATEHANDLER_H_
#define CONNECTIONSTATEHANDLER_H_

#include "core/MojService.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbServiceClient.h"
#include "IMLoginState.h"

class ConnectionState {
public:
	class ConnectionStateHandler : public MojSignalHandler {
	public:
		ConnectionStateHandler(MojService* service, ConnectionState* connState);
		virtual ~ConnectionStateHandler();
		MojErr connectionManagerResult(MojObject& result, MojErr err);
		bool hasConnData() { return m_receivedResponse; }

	private:
		MojDbClient::Signal::Slot<ConnectionStateHandler> m_connMgrSubscriptionSlot;

		MojService*	m_service;
		ConnectionState* m_connState;
		bool m_receivedResponse;
	};

	class ConnectionChangedScheduler : public MojSignalHandler {
	public:
		ConnectionChangedScheduler(MojService* service);
		MojErr scheduleActivity();

	private:
		MojDbClient::Signal::Slot<ConnectionChangedScheduler> m_scheduleActivitySlot;
		MojErr scheduleActivityResult(MojObject& result, MojErr err);
		MojDbClient::Signal::Slot<ConnectionChangedScheduler> m_activityCompleteSlot;
		MojErr activityCompleteResult(MojObject& result, MojErr err);

		MojService*	m_service;
	};

	ConnectionState(MojService* service);

	void initConnectionStatesFromActivity(const MojObject& activity);

	static void setLoginStateCallback(IMLoginState* loginState) { m_loginState = loginState; }

	static void getBestConnection(MojString& connectionType, MojString& ipAddress);

	static bool hasInternetConnection();
	static bool wanConnected() { return ConnectionState::m_connState->m_wanConnected == true; }
	static bool wifiConnected() { return ConnectionState::m_connState->m_wifiConnected == true; }
	static MojString& wanIpAddress() { return ConnectionState::m_connState->m_wanIpAddress; }
	static MojString& wifiIpAddress() { return ConnectionState::m_connState->m_wifiIpAddress; }

private:
	void connectHandlerDied();
	bool m_inetConnected;
	bool m_wanConnected;
	MojString m_wanIpAddress;
	bool m_wifiConnected;
	MojString m_wifiIpAddress;

	MojService*	m_service;
	ConnectionStateHandler* m_handler;

	static ConnectionState* m_connState;
	static IMLoginState* m_loginState;
};


#endif /* CONNECTIONSTATEHANDLER_H_ */
