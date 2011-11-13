/*
 * DisplayController.h
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
 * DisplayController class handles device display events
 */


#ifndef IM_DISPLAYCONTROLLER_H_
#define IM_DISPLAYCONTROLLER_H_

#include "core/MojService.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbServiceClient.h"
#include "IMLoginState.h"

class DisplayController {
public:
	class DisplayControllerSubscription : public MojSignalHandler {
	public:
		DisplayControllerSubscription(MojService* service, DisplayController* controller);
		virtual ~DisplayControllerSubscription();

	private:
		MojDbClient::Signal::Slot<DisplayControllerSubscription> m_connMgrSubscriptionSlot;
		MojErr subscriptionResult(MojObject& result, MojErr err);
		MojService*	m_service;
		DisplayController* m_controller;
	};

	DisplayController(MojService* service);

private:
	void handleDisplayEvent(MojObject& result, MojErr err);
	void subscriptionDied();

	MojService*	m_service;
	DisplayControllerSubscription* m_handler;

	bool m_presenceUpdatesDisabled;
};


#endif /* IM_DISPLAYCONTROLLER_H_ */
