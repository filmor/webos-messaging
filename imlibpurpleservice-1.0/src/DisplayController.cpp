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

#include "DisplayController.h"
#include "IMServiceApp.h"
#include "LibpurpleAdapter.h"

DisplayController::DisplayController(MojService* service)
: m_service(service),
  m_presenceUpdatesDisabled(false)
{
	assert(m_handler == NULL);
	m_handler = new DisplayController::DisplayControllerSubscription(service, this);
}

void DisplayController::subscriptionDied()
{
	m_handler = NULL;
	//TODO: resubscribe
}


void DisplayController::handleDisplayEvent(MojObject& result, MojErr err)
{
	bool returnValue = false;
	result.get("returnValue", returnValue);
	if (err != MojErrNone || returnValue != true)
	{
		// Something is wrong so assume the screen/backlight is on and enable presence.
		if (m_presenceUpdatesDisabled != false)
		{
			m_presenceUpdatesDisabled = false;
			LibpurpleAdapter::queuePresenceUpdates(false);
		}
    	//TODO: try subscribing again? maybe not since subscriptionDied() does that
	}
	else
	{
        bool disableUpdates = m_presenceUpdatesDisabled;
		bool found;
		MojString displayState;
        result.get("state", displayState, found);
		if (displayState == "on")
		{
			disableUpdates = false;
		}
		else if (displayState == "off")
		{
			disableUpdates = true;
		}
		else
		{
	        result.get("event", displayState, found);
			if (displayState == "displayOn")
			{
				disableUpdates = false;
			}
			else if (displayState == "displayOff")
			{
				disableUpdates = true;
			}
		}

		if (disableUpdates != m_presenceUpdatesDisabled)
		{
			m_presenceUpdatesDisabled = disableUpdates;
			LibpurpleAdapter::queuePresenceUpdates(disableUpdates);
		}
    }
}


DisplayController::DisplayControllerSubscription::DisplayControllerSubscription(MojService* service, DisplayController* controller)
: m_connMgrSubscriptionSlot(this, &DisplayController::DisplayControllerSubscription::subscriptionResult),
  m_service(service),
  m_controller(controller)
{
	MojLogInfo(IMServiceApp::s_log, "DisplayControllerSubscription constructor");

	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);
	if (err)
	{
		MojLogError(IMServiceApp::s_log, _T("DisplayControllerSubscription: create request failed"));
	}
	else
	{
		MojObject params;
		err = params.put("subscribe", true);
		err = req->send(m_connMgrSubscriptionSlot, "palm://com.palm.display","control/status", params, MojServiceRequest::Unlimited);
		if (err)
		{
			MojLogError(IMServiceApp::s_log, _T("DisplayControllerSubscription send request failed"));
		}
	}
}


DisplayController::DisplayControllerSubscription::~DisplayControllerSubscription()
{
	MojLogInfo(IMServiceApp::s_log, "DisplayControllerSubscription destructor");
	m_controller->subscriptionDied();
}


MojErr DisplayController::DisplayControllerSubscription::subscriptionResult(MojObject& result, MojErr err)
{
	m_controller->handleDisplayEvent(result, err);
	return MojErrNone;
}


