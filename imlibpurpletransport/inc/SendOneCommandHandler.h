/*
 * SendOneCommandHandler.h
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
 * SendOneCommandHandler class handles sending a single outgoing imcommand to the libpurple transport
 */

#ifndef SENDONECOMMANDHANDLER_H_
#define SENDONECOMMANDHANDLER_H_

#include "core/MojService.h"
#include "db/MojDbServiceClient.h"
#include "db/MojDbServiceClient.h"
#include "OutgoingIMCommandHandler.h"
#include "LibpurpleAdapter.h"

// libpurple transport property names - block, remove buddy
#define XPORT_BLOCK		        	_T("block")
#define XPORT_GROUP	        	    _T("group")
#define XPORT_ACCEPT	        	_T("accept")

class SendOneCommandHandler : public MojSignalHandler
{
public:

	SendOneCommandHandler(MojService* service, OutgoingIMCommandHandler* const imHandler);
	~SendOneCommandHandler();

	// send one im Command
	MojErr doSend(const MojObject imMsg);

private:

	// slot used for the DB delete commandcallback
	MojDbClient::Signal::Slot<SendOneCommandHandler> m_imDeleteCommandSlot;
	MojErr imDeleteCommandResult(MojObject& result, MojErr err);

	// slot used for the DB save callback
	MojDbClient::Signal::Slot<SendOneCommandHandler> m_imSaveCommandSlot;
	MojErr imSaveCommandResult(MojObject& result, MojErr err);

	// slot used for the DB remove buddy callback
	MojDbClient::Signal::Slot<SendOneCommandHandler> m_removeBuddySlot;
	MojErr removeBuddyResult(MojObject& result, MojErr err);

	// slot used for the DB remove contact callback
	MojDbClient::Signal::Slot<SendOneCommandHandler> m_removeContactSlot;
	MojErr removeContactResult(MojObject& result, MojErr err);

	// slot used for the DB add buddy callback
	MojDbClient::Signal::Slot<SendOneCommandHandler> m_addBuddySlot;
	MojErr addBuddyResult(MojObject& result, MojErr err);

	// slot used for the DB add contact callback
	MojDbClient::Signal::Slot<SendOneCommandHandler> m_addContactSlot;
	MojErr addContactResult(MojObject& result, MojErr err);

	// slot used for the DB unblock buddy callback
	MojDbClient::Signal::Slot<SendOneCommandHandler> m_findAccountIdForAddSlot;
	MojErr findAccountIdForAddResult(MojObject& result, MojErr err);

	// slot used for the DB block buddy callback
	MojDbClient::Signal::Slot<SendOneCommandHandler> m_findAccountIdForRemoveSlot;
	MojErr findAccountIdForRemoveResult(MojObject& result, MojErr err);

	LibpurpleAdapter::SendResult blockBuddy(const MojObject imCmd);
	LibpurpleAdapter::SendResult removeBuddy(const MojObject imCmd);
	LibpurpleAdapter::SendResult inviteBuddy(const MojObject imCmd);
	LibpurpleAdapter::SendResult receivedBuddyInvite(const MojObject imCmd);
	MojErr removeBuddyFromDb();
	MojErr addBuddyToDb();

	MojErr createContactQuery(MojDbQuery& query);
	MojErr readAccountIdFromResults(MojObject& result);

	// Pointer to the service used for creating/sending requests.
	MojService*	m_service;

	// Database client used to make any requests (put, watch, find, etc.) to Mojo DB
	MojDbServiceClient m_dbClient;
	MojDbServiceClient m_tempdbClient;

	// DB id for the current outgoing Command - need to save it for the send result handler
	MojString m_currentCmdDbId;

	// current user and service name
	MojString m_username;
	MojString m_serviceName;
	MojString m_accountId;
	MojString m_buddyName;

	// parent handler
	MojRefCountedPtr <OutgoingIMCommandHandler> m_outgoingIMHandler;

};

#endif /* SENDONECOMMANDHANDLER_H_ */
