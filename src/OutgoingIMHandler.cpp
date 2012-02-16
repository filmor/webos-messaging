/*
 * OutgoingIMHandler.cpp
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

#include "OutgoingIMHandler.h"
#include "db/MojDbQuery.h"
#include "IMMessage.h"
#include "IMServiceApp.h"
#include "SendOneMessageHandler.h"

//static const char *sendIMCallback = "palm://com.palm.imlibpurple/sendIM";
//static const char *activityMgrIMWatchName = "IM Service pending messages watch";

OutgoingIMHandler::OutgoingIMHandler(MojService* service, MojInt64 activityId)
: m_service(service),
  m_activityAdoptSlot(this, &OutgoingIMHandler::activityAdoptResult),
  m_findMessageSlot(this, &OutgoingIMHandler::findMessageResult),
//  m_activityDBWatchSlot(this, &OutgoingIMHandler::activityDBWatchResult),
  m_activityCompleteSlot(this, &OutgoingIMHandler::activityCompleteResult),
  m_dbClient(service)
{
	// need to hang onto this so we can complete it
	m_activityId = activityId;
}

OutgoingIMHandler::~OutgoingIMHandler()
{

}

/*
 * Initialization for the outgoing message handler
 *
 * errors returned to caller
 */
MojErr OutgoingIMHandler::start()
{

	// Activity Manager internet requirement should handle only firing with a valid wan or wifi connection

	// adopt the activity
	// Get a request object from the service
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);
	if (err)
		MojLogError(IMServiceApp::s_log, _T("OutgoingIMHandler::start: create activity manager adopt id %llu request failed. error %d"), m_activityId, err);
	else {
		MojObject adoptParams;
		adoptParams.put(_T("activityId"), m_activityId);
		adoptParams.putBool(_T("subscribe"), true);
		MojLogInfo(IMServiceApp::s_log, _T("Adopting activity id: %llu."), m_activityId);
		err = req->send(m_activityAdoptSlot, "com.palm.activitymanager", "adopt", adoptParams, MojServiceRequest::Unlimited);
		if (err)
			MojLogError(IMServiceApp::s_log, _T("OutgoingIMHandler::start: send activity manager adopt id %llu request failed. error %d"), m_activityId, err);
	}

	return MojErrNone;
}

/*
 * Callback for the result of the activity manager adopt call
 * Start processing the outgoing messages
 *
 * We get here twice:
 * result: {"activityId":1,"event":"orphan","returnValue":true}
 * result: {"adopted":true,"returnValue":true}
 */
MojErr  OutgoingIMHandler::activityAdoptResult(MojObject& result, MojErr resultErr) {

	MojLogTrace(IMServiceApp::s_log);

	// log the parameters
	IMServiceHandler::logMojObjectJsonString(_T("activityAdoptResult result: %s"), result);

	bool adopted = false;
	result.get(_T("adopted"), adopted );

	if (resultErr)  {
		// adopt failed
		MojString error;
		MojErrToString(resultErr, error);
		MojLogError(IMServiceApp::s_log, _T("activity manager adopt FAILED. error %d - %s"), resultErr, error.data());
	}
	// we currently get 2 notifications from the adopt call. ignore the first one
	else if (adopted) {
		MojLogInfo(IMServiceApp::s_log, _T("Activity id %llu adopted."), m_activityId);
		// query the DB for all IM messages where
		//     folder == outbox
		//     status == pending
		// for each one, call transport to send

		//construct our where clause
		MojDbQuery query;
		MojErr err;
		err = createOutgoingMessageQuery(query, true);

		if (err) {
			MojString error;
			MojErrToString(err, error);
			MojLogError(IMServiceApp::s_log, _T("createOutgoingMessageQuery failed: error %d - %s"), err, error.data());
		}

		// call find
		//virtual MojErr find(Signal::SlotRef handler, const MojDbQuery& query,
		//					bool watch = false, bool returnCount = false) = 0;
		err = m_dbClient.find(this->m_findMessageSlot, query, /* watch */ false );
		if (err) {
			MojString error;
			MojErrToString(err, error);
			MojLogError(IMServiceApp::s_log, _T("dbClient.find() failed: error %d - %s"), err, error.data());
		}
	}

	return MojErrNone;

}


/*
 * Callback for results of DB find
 * If any outgoing messages were found, send to transport service
 *
 * see sendIMHelper() in TransportCallbackThread.java
 *
 * errors are logged
 */
MojErr OutgoingIMHandler::findMessageResult(MojObject& result, MojErr findErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (findErr) {

		MojString error;
		MojErrToString(findErr, error);
		MojLogError(IMServiceApp::s_log, _T("find outgoing messages failed: error %d - %s"), findErr, error.data());

	} else {

		// get the results
		// debug log - OK to show message body
		MojString mojStringJson;
		result.toJson(mojStringJson);
		MojLogDebug(IMServiceApp::s_log, _T("IMSend find result: %s"), mojStringJson.data());

		MojObject results;

		// get results array in "results"
		result.get(_T("results"), results);

		if (!results.empty()) {

			// found messages to send. create a queue to hold them
			MojObject msg;
			MojObject::ConstArrayIterator itr = results.arrayBegin();
			while (itr != results.arrayEnd()) {
				msg = *itr;
				// this adds to the end of the vector
				m_messagesToSend.push(msg);
				itr++;
			}

			// pop the first message and send it
			MojObject imMsg;
			imMsg = m_messagesToSend.front();
			sendMessage(imMsg);

		} else {

			// nothing to send - this means we are done processing and we need to set up a watch for the next message
			// We really should never get here unless extra activity watches are firing...
			MojLogInfo(IMServiceApp::s_log, _T("No more outgoing messages to send"));
			messageFinished();
		}
	}
	return MojErrNone;
}

/*
 * Send the message to the correct send handler
 */
MojErr OutgoingIMHandler::sendMessage(MojObject imMsg)
{
	// We need one signal handler per message so the right slot gets fired
	MojRefCountedPtr<SendOneMessageHandler> sendHandler(new SendOneMessageHandler(m_service, this));
	MojErr err = sendHandler->doSend(imMsg);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("doSend FAILED. error %d - %s"), err, error.data());
		// not much we can do here
		messageFinished();
	}
	return MojErrNone;
}


/*
 * Create the query that we use for the DB watch
 * Looks for pending IM messages in the outbox folder
 *
 * errors returned to caller
 */
MojErr OutgoingIMHandler::createOutgoingMessageQuery(MojDbQuery& query, bool orderBy) {

	MojString IMType;
	MojErr err;

	MojString folderName;
	folderName.assign(IMMessage::folderStrings[Outbox]);
	err = query.where(MOJDB_FOLDER, MojDbQuery::OpEq, folderName);
	MojErrCheck(err);

	MojString statusName;
	statusName.assign(IMMessage::statusStrings[Pending]);
	err = query.where(MOJDB_STATUS, MojDbQuery::OpEq, statusName);
	MojErrCheck(err);

	//add our kind to the object
	err = query.from(PALM_DB_IMMESSAGE_KIND);
	MojErrCheck(err);

	if (orderBy) {
		// need to send in order - order by timestamp
		err = query.order(MOJDB_DEVICE_TIMESTAMP);
		MojErrCheck(err);
	}

	// log the watch
	MojObject queryObject;
	err = query.toObject(queryObject);
	MojErrCheck(err);
	IMServiceHandler::logMojObjectJsonString(_T("OutgoingIMHandler::createOutgoingMessageQuery: %s"), queryObject);

	return MojErrNone;

}

/*
 * We are finished handing the current message
 *
 * log any errors
 */
void OutgoingIMHandler::messageFinished() {

	// if we finished processing, remove the top message
	if (!m_messagesToSend.empty()) {
		m_messagesToSend.erase(0);
	}

	// create a new handler to send the next message
	if (!m_messagesToSend.empty()) {

		// pop the first message
		MojObject imMsg;
		imMsg = m_messagesToSend.front();
		sendMessage(imMsg);
	}
	else {
		// Tell the activity manager we are done
		if (m_activityId >= 0) {
			completeActivityManagerActivity();
		}

		// cancel any subscriptions...
	}

	return;
}

/*
 * Call complete on the activity manager for our activity
 */
MojErr OutgoingIMHandler::completeActivityManagerActivity() {

	// Get a request object from the service
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);
	if (err)
		MojLogError(IMServiceApp::s_log, _T("completeActivityManagerActivity: createRequest failed"));

	// create the service call
	MojObject params;
	params.put(_T("activityId"), m_activityId);
	// re-arm the DB watch
	params.put(_T("restart"), true);

	MojLogInfo(IMServiceApp::s_log, _T("completing activity id: %llu."), m_activityId);
	err = req->send(m_activityCompleteSlot, "com.palm.activitymanager", "complete", params, 1);
	if (err)
		MojLogError(IMServiceApp::s_log, _T("completeActivityManagerActivity: send request failed"));

	return MojErrNone;
}

/*
 * Callback for the result of the activity manager adopt call
 */
MojErr OutgoingIMHandler::activityCompleteResult(MojObject& result, MojErr completeErr) {

	MojLogTrace(IMServiceApp::s_log);
	// log the parameters
	IMServiceHandler::logMojObjectJsonString(_T("activityCompleteResult result: %s"), result);

	bool completed = false;
	result.get(_T("returnValue"), completed );

	if (completeErr || !completed) {
		// complete failed
		MojString error;
		bool found = false;
		result.get(_T("errorText"), error, found );
		if (!found)
			MojErrToString(completeErr, error);
		MojLogError(IMServiceApp::s_log, _T("activity manager complete FAILED. error %d - %s"), completeErr, error.data());
	} else {
		MojLogInfo(IMServiceApp::s_log, _T("Activity completed."));
		// cancel the prior subscription so the restarted watch can fire
		if (m_activityId > 0)
			m_activityAdoptSlot.cancel();

	}

	return MojErrNone;
}






