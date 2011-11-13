/*
 * OutgoingIMCommandHandler.h
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
 * OutgoingIMCommandHandler class handles processing pending imcommand objects
 */

#ifndef OUTGOINGIMCOMMANDHANDLER_H_
#define OUTGOINGIMCOMMANDHANDLER_H_



#include "OutgoingIMHandler.h"

class OutgoingIMCommandHandler : public OutgoingIMHandler
{
public:

	OutgoingIMCommandHandler(MojService* service, MojInt64 activityId);
	~OutgoingIMCommandHandler();

private:

	virtual MojErr createOutgoingMessageQuery(MojDbQuery& query, bool orderBy);
	virtual MojErr sendMessage(MojObject imMsg);

};

#endif /* OUTGOINGIMCOMMANDHANDLER_H_ */
