/*
 * OutgoingIMCommandHandler.cpp
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

#include "OutgoingIMCommandHandler.h"
#include "db/MojDbQuery.h"
#include "IMServiceApp.h"
#include "IMDefines.h"
#include "SendOneCommandHandler.h"
#include "BuddyStatusHandler.h"

OutgoingIMCommandHandler::OutgoingIMCommandHandler(MojService* service, MojInt64 activityId)
: OutgoingIMHandler (service, activityId)
{

}

OutgoingIMCommandHandler::~OutgoingIMCommandHandler()
{

}

MojErr OutgoingIMCommandHandler::sendMessage(MojObject imMsg)
{
	// We need one signal handler per message so the right slot gets fired
	MojRefCountedPtr<SendOneCommandHandler> sendHandler(new SendOneCommandHandler(m_service, this));
	MojErr err = sendHandler->doSend(imMsg);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("doSend FAILED. error %d - %s"), err, error.data());
	}
	return MojErrNone;
}

/*
 * Create the query that we use for the DB watch
 * Looks for IM commands with handler = transport
 *
 * errors returned to caller
 */
MojErr OutgoingIMCommandHandler::createOutgoingMessageQuery(MojDbQuery& query, bool orderBy) {

	MojString IMType;
	MojErr err;

	MojString handler;
	handler.assign(_T("transport"));
	err = query.where(MOJDB_HANDLER, MojDbQuery::OpEq, handler);
	MojErrCheck(err);

	MojString statusName;
	statusName.assign(IMMessage::statusStrings[Pending]);
	err = query.where(MOJDB_STATUS, MojDbQuery::OpEq, statusName);
	MojErrCheck(err);

	//add our kind to the object
	err = query.from(IM_IMCOMMAND_KIND);
	MojErrCheck(err);

	// log the watch
	MojObject queryObject;
	err = query.toObject(queryObject);
	MojErrCheck(err);
	IMServiceHandler::logMojObjectJsonString(_T("OutgoingIMCommandHandler::createOutgoingMessageQuery: %s"), queryObject);

	return MojErrNone;

}

/*
 * get the activity manager send callback method name
 */
//const MojChar* OutgoingIMCommandHandler::getSendCallbackName()
//{
//	return sendCmdCallback;
//}
//
///*
// * get the activity manager watch name
// */
//const MojChar* OutgoingIMCommandHandler::getActivityMgrWatchName()
//{
//	return activityMgrCmdWatchName;
//}





