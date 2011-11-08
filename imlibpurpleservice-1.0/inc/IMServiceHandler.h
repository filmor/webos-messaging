/*
 * IMServiceHandler.h
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


#ifndef IMSERVICEHANDLER_H_
#define IMSERVICEHANDLER_H_

#include "core/MojService.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbServiceClient.h"
#include "IMLoginState.h"
#include "ConnectionStateHandler.h"
#include "IMServiceApp.h"
#include "DisplayController.h"

class IMServiceHandler : public MojService::CategoryHandler, public IMServiceCallbackInterface, public IMServiceApp::Listener
{
public:

	static const int SERVICE_CRASHED_ERROR_CODE = -1;

	IMServiceHandler(MojService* service);
	virtual ~IMServiceHandler();
	MojErr init();

	// libpurple callback
	virtual bool incomingIM(const char* serviceName, const char* username, const char* usernameFrom, const char* message);
	virtual bool updateBuddyStatus(const char* accountId, const char* serviceName, const char* username, int availability,
			const char* customMessage, const char* groupName, const char* buddyAvatarLoc);
	virtual bool receivedBuddyInvite(const char* serviceName, const char* username, const char* usernameFrom, const char* message);
	virtual bool buddyInviteDeclined(const char* serviceName, const char* username, const char* usernameFrom);

	static MojErr logMojObjectJsonString(const MojChar* format, const MojObject mojObject);
	// strip message body to protect private data
	static MojErr privatelogIMMessage(const MojChar* format, MojObject mojObject, const MojChar* messageTextKey);

	// called by each signal handler when they start and stop processing so we know when to shut down
	void ProcessStarting();
	void ProcessDone();

private:
	static const MojInt64 SHUTDOWN_DELAY_SECONDS = 30; // number of seconds to wait before shutting down
	static const Method s_methods[];
	static const char* const COM_PALM_MESSAGING_KIND;

	// Pointer to the service used for creating/sending requests.
	MojService*	m_service;

	// Database client used to make any requests (put, watch, find, etc.) to Mojo DB
	MojDbServiceClient m_dbClient;

	IMLoginState* m_loginState;
	ConnectionState m_connectionState;
	DisplayController* m_displayController;

	// count of active processes (signal handlers)
	MojInt64 m_activeProcesses;

	// shutdown timer active
	MojInt64 m_shutdownCallbackId;

	MojErr onEnabled(MojServiceMessage* serviceMsg, const MojObject payload);

	MojErr handleLoginStateChange(MojServiceMessage* msg, const MojObject payload);
	MojErr loginForTesting(MojServiceMessage* msg, const MojObject payload);

	// Kicks off the send process. Queries DB for outgoing messages and sends them.
	MojErr IMSend(MojServiceMessage* msg, const MojObject payload);

	// Kicks off the send command process. Queries DB for outgoing commands and sends them.
	MojErr IMSendCmd(MojServiceMessage* msg, const MojObject payload);

	// can we shut down?
	bool OkToShutdown();

	// GLib Main Event Loop callback to shutdown the process.
	static gboolean ShutdownCallback(void* data);

};

#endif /* IMSERVICEHANDLER_H_ */
