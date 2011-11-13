/*
 * OnEnabledHandler.h
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
 * OnEnabledHandler class handles enabling and disabling the IM account
 */

#ifndef _ONENABLEDHANDLER_H_
#define _ONENABLEDHANDLER_H_

#include "core/MojService.h"
#include "db/MojDbServiceClient.h"
#include "LibpurpleAdapter.h"

class OnEnabledHandler : public MojSignalHandler
{
public:

	OnEnabledHandler(MojService* service);
	~OnEnabledHandler();

	MojErr start(const MojObject& payload);
	static void assignIMServiceHandler(IMServiceCallbackInterface* incomingIMHandler);

private:
	MojDbClient::Signal::Slot<OnEnabledHandler> m_getAccountInfoSlot;
	MojErr getAccountInfoResult(MojObject& payload, MojErr err);

	MojDbClient::Signal::Slot<OnEnabledHandler> m_addImLoginStateSlot;
	MojErr addImLoginStateResult(MojObject& payload, MojErr err);

	MojDbClient::Signal::Slot<OnEnabledHandler> m_deleteImLoginStateSlot;
	MojErr deleteImLoginStateResult(MojObject& payload, MojErr err);

	MojDbClient::Signal::Slot<OnEnabledHandler> m_deleteImMessagesSlot;
	MojErr deleteImMessagesResult(MojObject& payload, MojErr err);

	MojDbClient::Signal::Slot<OnEnabledHandler> m_deleteImCommandsSlot;
	MojErr deleteImCommandsResult(MojObject& payload, MojErr err);

	MojDbClient::Signal::Slot<OnEnabledHandler> m_deleteContactsSlot;
	MojErr deleteContactsResult(MojObject& payload, MojErr err);

	MojDbClient::Signal::Slot<OnEnabledHandler> m_deleteImBuddyStatusSlot;
	MojErr deleteImBuddyStatusResult(MojObject& payload, MojErr err);

	MojErr accountEnabled(const MojString& accountId, const MojString& serviceName, const MojString& username);
	MojErr accountDisabled(const MojString& accountId, const MojString& serviceName, const MojString& username);
	MojErr getMessagingCapabilityObject(const MojObject& capabilityProviders, MojObject& messagingObj);
	MojErr getDefaultServiceName(const MojObject& accountResult, MojString& serviceName);
	void getServiceNameFromCapabilityId(MojString& serviceName);

	MojService* m_service;
	MojDbServiceClient m_dbClient;
	MojDbServiceClient m_tempdbClient;
	bool m_enable;
	MojString m_capabilityProviderId;
};

#endif /* _ONENABLEDHANDLER_H_ */
