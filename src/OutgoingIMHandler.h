/*
 * OutgoingIMHandler.h
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
 * OutgoingIMHandler class handles sending pending immessage and imcommand objects
 */

#ifndef OUTGOINGIMHANDLER_H_
#define OUTGOINGIMHANDLER_H_

#include "core/MojService.h"
#include "db/MojDbServiceClient.h"

class OutgoingIMHandler : public MojSignalHandler
{
public:

	OutgoingIMHandler(MojService* service, MojInt64 activityId);
	~OutgoingIMHandler();

	// initiate the send process
	MojErr start();

	// callback when send is finished
	void messageFinished();

protected:

    // Pointer to the service used for creating/sending requests.
	MojService* m_service;

private:
	virtual MojErr createOutgoingMessageQuery(MojDbQuery& query, bool orderBy);
	virtual MojErr sendMessage(MojObject imMsg);

	// Slot used for the response to the send IM activity manager adopt operation
	MojDbClient::Signal::Slot<OutgoingIMHandler> m_activityAdoptSlot;
	MojErr activityAdoptResult(MojObject& result, MojErr err);

	// callback for the DB find
	MojDbClient::Signal::Slot<OutgoingIMHandler> m_findMessageSlot;
	MojErr findMessageResult(MojObject& result, MojErr err);

	// Slot used for the response to the activity manager adopt operation
	MojDbClient::Signal::Slot<OutgoingIMHandler> m_activityCompleteSlot;
	MojErr activityCompleteResult(MojObject& result, MojErr err);

	MojErr completeActivityManagerActivity();

	// Database client used to make any requests (put, watch, find, etc.) to Mojo DB
	MojDbServiceClient m_dbClient;

	MojVector<MojObject> m_messagesToSend;
	MojInt64 m_activityId;

};

#endif /* OUTGOINGIMHANDLER_H_ */
