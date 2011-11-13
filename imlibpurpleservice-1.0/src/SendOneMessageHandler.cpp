/*
 * SendOneMessageHandler.cpp
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

#include "SendOneMessageHandler.h"
#include "db/MojDbQuery.h"
#include "IMMessage.h"
#include "IMServiceApp.h"
#include "IMDefines.h"
#include "BuddyStatusHandler.h"
#include "IMServiceHandler.h"

SendOneMessageHandler::SendOneMessageHandler(MojService* service, OutgoingIMHandler* const imHandler)
: m_imSaveMessageSlot(this, &SendOneMessageHandler::imSaveMessageResult),
  m_findBuddySlot(this, &SendOneMessageHandler::findBuddyResult),
  m_listAccountSlot(this, &SendOneMessageHandler::listAccountResult),
  m_findAccountIdSlot(this, &SendOneMessageHandler::findAccountIdResult),
  m_service(service),
  m_dbClient(service, MojDbServiceDefs::ServiceName),
  m_tempdbClient(service, MojDbServiceDefs::TempServiceName)
{
	// remember the handler so we can let them know when we are done
	m_outgoingIMHandler = imHandler;

}

SendOneMessageHandler::~SendOneMessageHandler()
{

}

/*
 * Send one IMMessage that was in the outbox. See java code in TransportCallbackThread.sendImHelper()
 * Change the message status from pending to either successful or failed in the DB when done
 * Set timestamp on the message and save it.
 *
 * errors returned up to OutgoingIMHandler
 *
 *  send payload format:
 *  im.libpurple.palm	//sendMessage	string=“{"serviceName":"gmail","usernameTo":"palm@gmail.com","messageText":"Test send from phone","username":"xxx@gmail.com"}”
 *  luna://im.libpurple.palm/sendMessage '{"serviceName":"aol","usernameTo":"palm","messageText":"test send IM","username":"xxx@aol.com"}'
 *
 * @param imMsg - imMessage that was read out of the DB with status pending and folder set to outbox
 */
MojErr SendOneMessageHandler::doSend(const MojObject imMsg) {

	MojErr err = MojErrNone;

	// body
	// text should be saved unescaped in the DB
	bool found = false;
	err = imMsg.get(MOJDB_MSG_TEXT, m_messageText, found);
	MojErrCheck(err);

	// userNameTo - use "to" address
	MojObject addrArray; // array
	found = imMsg.get(MOJDB_TO, addrArray);

	// now read the address out of the object
	MojObject toAddrObject;
	if (found) {
		found = addrArray.at(0, toAddrObject);
		err = toAddrObject.get(MOJDB_ADDRESS, m_usernameTo, found);
		MojErrCheck(err);
	}

	// user = "from" address
	MojObject fromAddrObject;
	found = imMsg.get(MOJDB_FROM, fromAddrObject);
	if (found) {
		err = fromAddrObject.get(MOJDB_ADDRESS, m_username, found);
		MojErrCheck(err);
	}

	// serviceName
	err = imMsg.get(MOJDB_SERVICENAME, m_serviceName, found);
	MojErrCheck(err);

	// get the id so we can update the status in  the DB after sending
	err = imMsg.getRequired("_id", m_currentMsgdbId);
	MojErrCheck(err);

	// need to search buddy list to see if this user is on it.
	// first we have to figure out the accountId - query ImLoginState

	// construct our where clause - find by username and servicename
	MojDbQuery query;
	query.where("serviceName", MojDbQuery::OpEq, m_serviceName);
	query.where("username", MojDbQuery::OpEq, m_username);
	query.from(IM_LOGINSTATE_KIND);

	// call del
	// virtual MojErr del(Signal::SlotRef handler, const MojDbQuery& query,
	//					   MojUInt32 flags = MojDb::FlagNone);
	err = m_dbClient.find(this->m_findAccountIdSlot, query);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("doSend: dbClient.find() failed: error %d - %s"), err, error.data());
		// tell the outgoing Command handler we are done
		failMessage(ERROR_SEND_GENERIC_ERROR);
	}

	return MojErrNone;
}

/*
 * Parse the account id from the results object for the accountId query
 * saved in m_accountId member variable
 *
 */
MojErr SendOneMessageHandler::readAccountIdFromResults(MojObject& result)
{
	// log the results
	MojString mojStringJson;
	result.toJson(mojStringJson);
	MojLogDebug(IMServiceApp::s_log, _T("readAccountIdFromResults result: %s"), mojStringJson.data());

	// "results" in result
	MojObject results;
	result.get(_T("results"), results);

	// check to see if array is empty - there should really always be 1 item here
	if (!results.empty()){

		// get the db id of the buddy
		MojObject loginState;
		MojObject::ConstArrayIterator itr = results.arrayBegin();
		bool foundOne = false;
		while (itr != results.arrayEnd()) {
			if (foundOne) {
				MojLogError(IMServiceApp::s_log,
						_T("readAccountIdFromResults: found more than one ImLoginState with same username/serviceName - using the first one"));
				break;
			}
			loginState = *itr;
			foundOne = true;
			itr++;
		}

		// create the DB object
		MojErr err = loginState.getRequired("accountId", m_accountId);
		if (err) {
			MojLogError(IMServiceApp::s_log, _T("readAccountIdFromResults: missing accountId in loginState entry"));
		}
		MojLogInfo(IMServiceApp::s_log, _T("readAccountIdFromResults - accountId: %s. "), m_accountId.data());
	}
	else {
		MojLogError(IMServiceApp::s_log, _T("readAccountIdFromResults: no matching loginState record found for %s %s"), m_username.data(), m_serviceName.data());
	}

	return MojErrNone;
}


/*
 * Result of query for loginState entry with given username and serviceName when removing a buddy
 *
 * Remove buddy from buddyStatus and contact DB
 */
MojErr SendOneMessageHandler::findAccountIdResult(MojObject& result, MojErr findErr)
{

	if (findErr) {

		MojString error;
		MojErrToString(findErr, error);
		MojLogError(IMServiceApp::s_log, _T("findAccountIdResult failed: error %d - %s"), findErr, error.data());
		// not much we can do here...
		failMessage(ERROR_SEND_GENERIC_ERROR);

	} else {

		// parse out the accountId
		readAccountIdFromResults(result);

		if (!m_accountId.empty()){

			// construct our where clause - find by username, accountId and servicename
			MojDbQuery query;
			query.where("username", MojDbQuery::OpEq, m_usernameTo);
			query.where("accountId", MojDbQuery::OpEq, m_accountId);
			query.from(IM_BUDDYSTATUS_KIND);
			MojObject queryObject;
			query.toObject(queryObject);
			IMServiceHandler::logMojObjectJsonString(_T("findAccountIdResult - buddyStatus query: %s"), queryObject);

			// call find
			//virtual MojErr find(Signal::SlotRef handler, const MojDbQuery& query,
			//					bool watch = false, bool returnCount = false) = 0;
			MojErr err = m_tempdbClient.find(this->m_findBuddySlot, query, /* watch */ false );
			if (err) {
				MojString error;
				MojErrToString(err, error);
				MojLogError(IMServiceApp::s_log, _T("findAccountIdResult dbClient.find() failed: error %d - %s"), err, error.data());
				failMessage(ERROR_SEND_GENERIC_ERROR);
			}
		}
		else {
			MojLogError(IMServiceApp::s_log, _T("findAccountIdResult: no matching loginState record found for %s %s"), m_username.data(), m_serviceName.data());
			// tell the outgoing Command handler we are done
			failMessage(ERROR_SEND_GENERIC_ERROR);
		}
	}

	return MojErrNone;
}

/*
 * Result of query for buddy with given username and serviceName
 *
 * If the buddy is found, we can send no matter what. If buddy is not found, we need to check if this service allows sending IM messages
 * to non buddies (gtalk does not allow this, aol does)
 */
MojErr SendOneMessageHandler::findBuddyResult(MojObject& result, MojErr findErr)
{

	if (findErr) {

		MojString error;
		MojErrToString(findErr, error);
		MojLogError(IMServiceApp::s_log, _T("findBuddyResult: error %d - %s"), findErr, error.data());
		// not much we can do here...
		failMessage(ERROR_SEND_GENERIC_ERROR);

	} else {

		// log the results
		MojString mojStringJson;
		result.toJson(mojStringJson);
		MojLogDebug(IMServiceApp::s_log, _T("findBuddyResult result: %s"), mojStringJson.data());

		// "results" in result
		MojObject results;
		result.get(_T("results"), results);

		// check to see if array is empty - there should really always be 0 or 1 item here
		if (!results.empty()){

			// buddy is on the list - send to the transport
			MojLogInfo(IMServiceApp::s_log, _T("findBuddyResult - user is on buddy list. No need to check account template."));
			sendToTransport();
		}
		else {
			MojLogInfo(IMServiceApp::s_log, _T("findBuddyResult - no buddy found. Need to check account template if this is allowed"));

			// check palm://com.palm.service.accounts/listAccountTemplates {"capability":"MESSAGING"} to get a list of account templates that support messaging.
			// If the account template includes "chatWithNonBuddies":false, it should fail to send.
			MojLogInfo(IMServiceApp::s_log, "Issuing listAccountTemplates request to com.palm.service.accounts");

			// parameters: {“capability”:”MESSAGING”}
			MojObject params;
			params.putString("capability", "MESSAGING");

			// Get a request object from the service
			MojRefCountedPtr<MojServiceRequest> req;
			MojErr err = m_service->createRequest(req);

			if (!err) {
				err = req->send(m_listAccountSlot, "com.palm.service.accounts", "listAccountTemplates", params);
			}

			if (err) {
				MojString error;
				MojErrToString(err, error);
				MojLogError(IMServiceApp::s_log, _T("findBuddyResult failed to send request to accounts: error %d - %s"), err, error.data());
				// not much we can do here...don't leave message still pending
				failMessage(ERROR_SEND_GENERIC_ERROR);
			}
		}
	}

	return MojErrNone;
}

/*
 * Result callback for the listAccountsTemplate
 *
 * result list form:
 * {
		"templateId": "com.palm.aol",
		"loc_name": "AOL",
		"validator": "palm://com.palm.imaccountvalidator/checkCredentials",
		"capabilityProviders": [
			{
				"capability": "MESSAGING",
				"capabilitySubtype": "IM",
				"id": "com.palm.aol.aim",
				"loc_name": "AIM",
				"loc_shortName": "AIM",
				"icon": {
					"loc_32x32": "/usr/palm/public/accounts/com.palm.aol/images/aim32x32.png",
					"loc_48x48": "/usr/palm/public/accounts/com.palm.aol/images/aim48x48.png",
					"splitter": "/usr/palm/public/accounts/com.palm.aol/images/aim_transport_splitter.png"
				},
				"implementation": "palm://com.palm.imlibpurple/",
				"onEnabled": "palm://com.palm.imlibpurple/onEnabled",
				"serviceName": "type_aim",
				"dbkinds": {
					"immessage": "com.palm.immessage.libpurple",
					"imcommand": "com.palm.imcommand.libpurple"
				}
			}
		]
	},
 */
MojErr SendOneMessageHandler::listAccountResult(MojObject& result, MojErr err)
{
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("listAccountTemplates failed. error %d - %s"), err, error.data());
		// not much we can do here...don't leave message pending
		failMessage(ERROR_SEND_GENERIC_ERROR);

	}
	else {
		IMServiceHandler::logMojObjectJsonString(_T("listAccountTemplates success: %s"), result);

		// if the flag is not there, assume we can send to a non buddy
		// if we got here, the message is going to a non buddy
		bool nonBuddyChatOK = true;

		// find the template and provider for our service and look for "chatWithNonBuddies":false
		// "results" in result
		MojObject results;
		result.get(_T("results"), results);

		// check to see if array is empty - should not be...
		if (!results.empty()){

			// find the messaging capability object
			MojObject capability;
			MojObject accountTemplate;
			MojString serviceName;
			bool found = false;
			bool foundService = false;

			MojObject::ConstArrayIterator templItr = results.arrayBegin();
			while (templItr != results.arrayEnd()) {
				accountTemplate = *templItr;
				IMServiceHandler::logMojObjectJsonString(_T("listAccountTemplates template: %s"), accountTemplate);
				
				// now find the capabilityProviders array
				MojObject providersArray;
				found = accountTemplate.get(XPORT_CAPABILITY_PROVIDERS, providersArray);
				if (found) {
					MojObject::ConstArrayIterator capItr = providersArray.arrayBegin();
					while (capItr != providersArray.arrayEnd()) {
						capability = *capItr;
						IMServiceHandler::logMojObjectJsonString(_T("listAccountTemplates capability: %s"), capability);
						
						// find the one for our service
						capability.get(XPORT_SERVICE_TYPE, serviceName, found);
						if (found) {
							MojLogInfo(IMServiceApp::s_log, _T("listAccountTemplates - capability service %s. "), serviceName.data());
							if (0 == serviceName.compare(m_serviceName)) {
								foundService = true;
								found = capability.get("chatWithNonBuddies", nonBuddyChatOK);
								MojLogInfo(IMServiceApp::s_log, _T("listAccountTemplates - found service %s. found %d, chatWithNonBuddies %d"), m_serviceName.data(), found, nonBuddyChatOK);
								break;
							}
						}
						else MojLogError(IMServiceApp::s_log, _T("error: no service name in capability provider"));
						capItr++;
					}
				}
				else MojLogError(IMServiceApp::s_log, _T("error: no provider array in templateId"));
				if (foundService)
					break;
				templItr++;
			}
		}
		if (nonBuddyChatOK){
			// send to the transport
			sendToTransport();
		}
		else {
			// permanent error - no retry option
			MojLogError(IMServiceApp::s_log, _T("error: Trying to send to user that is not on buddy list"));
			failMessage(ERROR_SEND_TO_NON_BUDDY);
		}
	}

	return MojErrNone;
}


/*
 * Send the messages off to libpurple transport
 */
MojErr SendOneMessageHandler::sendToTransport()
{
	MojErr err = MojErrNone;

	// can't log message text in production
	MojLogInfo (IMServiceApp::s_log, "sending message to transport. id: %s, serviceName: %s, username: %s, usernameTo: %s",
			m_currentMsgdbId.data(), m_serviceName.data(), m_username.data(), m_usernameTo.data());

	// Note, libpurple does not return errors on send - adapter only checks that the parameters are valid and that the user is logged in
	// otherwise assume success...
	LibpurpleAdapter::SendResult retVal = LibpurpleAdapter::sendMessage(m_serviceName.data(), m_username.data(), m_usernameTo.data(), m_messageText.data());

	// Now save the status
	MojObject propObject;
	if (LibpurpleAdapter::SENT == retVal) {
		// successful send
		propObject.putString(MOJDB_STATUS, IMMessage::statusStrings[Successful]);
		MojLogInfo(IMServiceApp::s_log, _T("LibpurpleAdapter::sendToTransport succeeded"));
	}
	else if (LibpurpleAdapter::USER_NOT_LOGGED_IN == retVal) {
		// user not logged in - put in queued state
		propObject.putString(MOJDB_STATUS, IMMessage::statusStrings[WaitingForConnection]);
		MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::sendToTransport - user not logged in. waiting for connection"));
	}
	else {
		// permanent error - no retry option
		propObject.putString(MOJDB_STATUS, IMMessage::statusStrings[Undeliverable]);

		// set the error category string for the UI
		const MojChar* errorCategory = transportErrorToCategory(retVal);
		propObject.putString(MOJDB_ERROR_CATEGORY, errorCategory);
		MojLogError(IMServiceApp::s_log, _T("LibpurpleAdapter::sendToTransport failed. error %d, category %s"), retVal, errorCategory);
	}

	// id to update
	propObject.putString(MOJDB_ID, m_currentMsgdbId);

	// set server timestamp to current time.
	MojInt64 now = time (NULL);
	propObject.put(MOJDB_SERVER_TIMESTAMP, now);

	// save the new fields - call merge
	// MojErr merge(Signal::SlotRef handler, const MojObject& obj, MojUInt32 flags = MojDb::FlagNone);
	err = m_dbClient.merge(this->m_imSaveMessageSlot, propObject);
	if (err) {
		MojLogError(IMServiceApp::s_log, _T("call to merge message into DB failed. err %d, DB id %s: "), err, m_currentMsgdbId.data() );
		// request to save failed - not much we can do here...
		m_outgoingIMHandler->messageFinished();
	}

	return MojErrNone;
}

/*
 * Callback for DB save results
 * Status changes
 */
MojErr SendOneMessageHandler::imSaveMessageResult(MojObject& result, MojErr saveErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (saveErr) {
		MojString error;
		MojErrToString(saveErr, error);
		MojLogError(IMServiceApp::s_log, _T("save sent im failed. error %d - %s"), saveErr, error.data());
	}
	else {
		MojString json;
		result.toJson(json);
		MojLogInfo(IMServiceApp::s_log, _T("save sent message %s. response: %s"), this->m_currentMsgdbId.data(), json.data());
	}

	// tell the outgoing message handler we are done
	m_outgoingIMHandler->messageFinished();
	
	return MojErrNone;
}

/*
 * Translate the libpurple error number to the category (string) saved in the DB
 */
const MojChar* SendOneMessageHandler::transportErrorToCategory(LibpurpleAdapter::SendResult result)
{
	if (LibpurpleAdapter::USER_NOT_LOGGED_IN == result)
		return ERROR_SEND_USER_NOT_LOGGED_IN;
	else
		return ERROR_SEND_GENERIC_ERROR;
}

/*
 * Fail the message in the DB
 * This is the result of an internal error that should not happen - we just need to get back to a consistent state
 */
MojErr SendOneMessageHandler::failMessage(const MojChar *errorString)
{
	// permanent error - no retry option
	MojObject propObject;
	propObject.putString(MOJDB_STATUS, IMMessage::statusStrings[Undeliverable]);

	// set the error category string for the UI
	propObject.putString(MOJDB_ERROR_CATEGORY,errorString);
	MojLogError(IMServiceApp::s_log, _T("failMessage error: %s"), errorString);

	// id to update
	propObject.putString(MOJDB_ID, m_currentMsgdbId);

	// set server timestamp to current time.
	MojInt64 now = time (NULL);
	propObject.put(MOJDB_SERVER_TIMESTAMP, now);

	// save the new fields - call merge
	// MojErr merge(Signal::SlotRef handler, const MojObject& obj, MojUInt32 flags = MojDb::FlagNone);
	MojErr err = m_dbClient.merge(this->m_imSaveMessageSlot, propObject);
	if (err) {
		MojLogError(IMServiceApp::s_log, _T("call to merge message into DB failed. err %d, DB id %s: "), err, m_currentMsgdbId.data() );
		// call to save failed - not much we can do here...
		m_outgoingIMHandler->messageFinished();
	}
	return MojErrNone;
}

