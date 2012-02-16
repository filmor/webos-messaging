/*
 * SendOneMessageHandler.h
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
 * SendOneMessageHandler class handles sending a single outgoing immessage
 */

#ifndef SENDONEMESSAGEHANDLER_H_
#define SENDONEMESSAGEHANDLER_H_

#include "core/MojService.h"
#include "db/MojDbServiceClient.h"
#include "db/MojDbServiceClient.h"
#include "OutgoingIMHandler.h"
#include "LibpurpleAdapter.h"

// send IM error codes
#define ERROR_SEND_TO_NON_BUDDY       "SendIMErr_non_buddy"
#define ERROR_SEND_USER_NOT_LOGGED_IN "SendIMErr_user_not_logged_in"
#define ERROR_SEND_GENERIC_ERROR      "SendIMErr_generic_error"

class SendOneMessageHandler : public MojSignalHandler
{
public:

	SendOneMessageHandler(MojService* service, OutgoingIMHandler* const imHandler);
	~SendOneMessageHandler();

	// send one im message
	MojErr doSend(const MojObject imMsg);

private:

	// slot used for the DB save callback
	MojDbClient::Signal::Slot<SendOneMessageHandler> m_imSaveMessageSlot;
	MojErr imSaveMessageResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<SendOneMessageHandler> m_findBuddySlot;
	MojErr findBuddyResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<SendOneMessageHandler> m_listAccountSlot;
	MojErr listAccountResult(MojObject& result, MojErr err);

	// slot used to find accountId from loginState
	MojDbClient::Signal::Slot<SendOneMessageHandler> m_findAccountIdSlot;
	MojErr findAccountIdResult(MojObject& result, MojErr err);

	MojErr sendToTransport();
	const MojChar* transportErrorToCategory(LibpurpleAdapter::SendResult transportError);
	MojErr failMessage(const MojChar *errorString);
	MojErr readAccountIdFromResults(MojObject& result);

	// Pointer to the service used for creating/sending requests.
	MojService*	m_service;

	// Database client used to make any requests (put, watch, find, etc.) to Mojo DB
	MojDbServiceClient m_dbClient;
	MojDbServiceClient m_tempdbClient;

	// DB id for the current outgoing message - need to save it for the send result handler
	MojString m_currentMsgdbId;
	
	// message properties
	MojString m_messageText;
	MojString m_username;
	MojString m_usernameTo;
	MojString m_serviceName;
	MojString m_accountId;

	// parent handler
	MojRefCountedPtr <OutgoingIMHandler> m_outgoingIMHandler;
	
};
#endif /* SENDONEMESSAGEHANDLER_H_ */
