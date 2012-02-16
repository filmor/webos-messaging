/*
 * SendOneCommandHandler.cpp
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

#include "SendOneCommandHandler.h"
#include "db/MojDbQuery.h"
#include "IMServiceApp.h"
#include "LibpurpleAdapter.h"
#include "BuddyStatusHandler.h"
#include "IMDefines.h"

SendOneCommandHandler::SendOneCommandHandler(MojService* service, OutgoingIMCommandHandler* const imHandler)
: m_imDeleteCommandSlot(this, &SendOneCommandHandler::imDeleteCommandResult),
  m_imSaveCommandSlot(this, &SendOneCommandHandler::imSaveCommandResult),
  m_removeBuddySlot(this, &SendOneCommandHandler::removeBuddyResult),
  m_removeContactSlot(this, &SendOneCommandHandler::removeContactResult),
  m_addBuddySlot(this, &SendOneCommandHandler::addBuddyResult),
  m_addContactSlot(this, &SendOneCommandHandler::addContactResult),
  m_findAccountIdForAddSlot(this, &SendOneCommandHandler::findAccountIdForAddResult),
  m_findAccountIdForRemoveSlot(this, &SendOneCommandHandler::findAccountIdForRemoveResult),
  m_service(service),
  m_dbClient(service, MojDbServiceDefs::ServiceName),
  m_tempdbClient(service, MojDbServiceDefs::TempServiceName)
{
	// remember the handler so we can let them know when we are done
	m_outgoingIMHandler = imHandler;

}

SendOneCommandHandler::~SendOneCommandHandler()
{

}

/*
 * Send one IMCommand that has handler set to transport
 *
 * errors returned up to OutgoingIMCommandHandler
 *
 *
 * @param imCmd - imCommand that was read out of the DB with handler set to transport
 {
   "id"      : "ImCommand",
   "type"    : "object",
   "properties" : {
      "command"       : {"type"        : "string",
                         "enum"        : ["blockBuddy","deleteBuddy","sendBuddyInvite","receivedBuddyInvite","createChatGroup","inviteToGroup","leaveGroup"],
                         "description" : "The command to be processed"},
      "params"        : {"type"        : "any",
                         "description" : "Parameters associated with the command are stored here."}
      "handler"       : {"type"        : "string",
                         "enum"        : ["transport","application"],
                         "description" : "Who is responsible for handling the command"},
      "targetUsername"  : {"type"        : "string",
                         "description" : "The buddyname the command will act on"},
      "fromUsername"  : {"type"        : "string",
                         "description" : "The username that originated the command"},
      "serviceName"   : {"type"        : "string",
                         "description" : "Name of originating service (see IMAddress of contacts load library*)."}
      }
 * }
 */
MojErr SendOneCommandHandler::doSend(const MojObject imCmd) {

	IMServiceHandler::logMojObjectJsonString(_T("send command: %s"),imCmd);
	MojString command;
	LibpurpleAdapter::SendResult retVal = LibpurpleAdapter::SENT;
	bool found = false;

	MojErr err = imCmd.get(MOJDB_COMMAND, command, found);
	MojErrCheck(err);

	// serviceName
	err = imCmd.get(MOJDB_SERVICE_NAME, m_serviceName, found);
	MojErrCheck(err);

	// get the id so we can update the status in  the DB after sending
	err = imCmd.getRequired(MOJDB_ID, m_currentCmdDbId);
	MojErrCheck(err);

	// for receiving invite, the user account and remote user are reversed: targetUsername is us and the fromUsername is the remote buddy
	if (0 == command.compare(_T("receivedBuddyInvite"))) {
		// username of current account
		err = imCmd.get(MOJDB_FROM_USER, m_buddyName, found);
		MojErrCheck(err);

		// buddy / remote user
		err = imCmd.get(MOJODB_TARGET_USER, m_username, found);
		MojErrCheck(err);
	}
	else {
		// username of current account
		err = imCmd.get(MOJDB_FROM_USER, m_username, found);
		MojErrCheck(err);

		// buddy / remote user
		err = imCmd.get(MOJODB_TARGET_USER, m_buddyName, found);
		MojErrCheck(err);
	}

	// which command?
	if (0 == command.compare(_T("blockBuddy"))) {
		retVal = blockBuddy(imCmd);
	}
	else if (0 == command.compare(_T("deleteBuddy"))) {
		retVal = removeBuddy(imCmd);
	}
	else if (0 == command.compare(_T("sendBuddyInvite"))) {
		retVal = inviteBuddy(imCmd);
	}
	else if (0 == command.compare(_T("receivedBuddyInvite"))) {
		retVal = receivedBuddyInvite(imCmd);
	}
	else {
		MojLogError(IMServiceApp::s_log, _T("doSend: unknown command %s"), command.data());
		retVal = LibpurpleAdapter::SEND_FAILED;
	}

	// we can't just delete the command if the user is not logged on...
	// need to save command in "waiting" state waiting for user to login
	if (LibpurpleAdapter::USER_NOT_LOGGED_IN == retVal) {
		// user not logged in - put in queued state
		MojLogError(IMServiceApp::s_log, _T("doSend - can't process command - user not logged in. Waiting for connection"));
		MojObject propObject;
		propObject.putString(MOJDB_STATUS, IMMessage::statusStrings[WaitingForConnection]);

		// id to update
		propObject.putString(MOJDB_ID, m_currentCmdDbId);

		// save the new fields - call merge
		err = m_dbClient.merge(this->m_imSaveCommandSlot, propObject);
		if (err) {
			MojLogError(IMServiceApp::s_log, _T("doSend - DB merge command failed. err %d, DB id %s: "), err, m_currentCmdDbId.data() );
		}
	}
	else {
		// delete command so we don't keep processing it
		// put id in an array
		MojObject idsToDelete;  // array
		err = idsToDelete.push(m_currentCmdDbId);

		// luna://com.palm.db/del '{"ids":[2]}'
		IMServiceHandler::logMojObjectJsonString(_T("deleting imcommand: %s"), idsToDelete);
		err = m_dbClient.del(this->m_imDeleteCommandSlot, idsToDelete.arrayBegin(), idsToDelete.arrayEnd());

		if (err) {
			MojLogError(IMServiceApp::s_log, _T("doSend - DB del command failed. err %d, DB id %s: "), err, m_currentCmdDbId.data() );
		}
	}

	if (LibpurpleAdapter::SENT != retVal) {
		// something went wrong - nothing more we can do
		MojLogError(IMServiceApp::s_log, _T("doSend: command failed"));
		m_outgoingIMHandler->messageFinished();

	}

	return MojErrNone;
}

/*
 * Callback for DB delete results
 */
MojErr SendOneCommandHandler::imDeleteCommandResult(MojObject& result, MojErr delErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (delErr) {
		MojString error;
		MojErrToString(delErr, error);
		MojLogError(IMServiceApp::s_log, _T("delete imcommand failed. error %d - %s"), delErr, error.data());
	}
	else {
		MojString json;
		result.toJson(json);
		MojLogInfo(IMServiceApp::s_log, _T("delete imcommand %s. response: %s"), this->m_currentCmdDbId.data(), json.data());
	}

	return MojErrNone;
}

/*
 * Callback for DB save results
 *
 * Status change to "waiting-for-connection"
 */
MojErr SendOneCommandHandler::imSaveCommandResult(MojObject& result, MojErr saveErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (saveErr) {
		MojString error;
		MojErrToString(saveErr, error);
		MojLogError(IMServiceApp::s_log, _T("save imcommand failed. error %d - %s"), saveErr, error.data());
	}
	else {
		MojString json;
		result.toJson(json);
		MojLogInfo(IMServiceApp::s_log, _T("save imcommand %s. response: %s"), this->m_currentCmdDbId.data(), json.data());
	}

	return MojErrNone;
}


/*
 * blockBuddy command: Send the blockBuddy command off to libPurple
 * remove buddy from buddyStatus and contact DB kinds
 * imcommand
 * {
   "params" : {
     "block"           : {"type"        : "boolean",
                          "description" : "whether to block or unblock the buddy."}
     }
 * }
 */
LibpurpleAdapter::SendResult SendOneCommandHandler::blockBuddy(const MojObject imCmd) {

	bool found = false;

	// params
	MojObject params;
	found = imCmd.get(MOJDB_PARAMS, params);
	IMServiceHandler::logMojObjectJsonString(_T("command params: %s"),params);
	bool block = true;
	found = params.get(XPORT_BLOCK, block);

	MojLogInfo (IMServiceApp::s_log, "sending blockBuddy command to transport. id: %s, serviceName: %s, username: %s, buddyUsername: %s, block: %d",
			m_currentCmdDbId.data(), m_serviceName.data(), m_username.data(), m_buddyName.data(), block);


	// Note, libpurple does not return errors on send - adapter only checks that the parameters are valid and that the user is logged in
	// otherwise assume success...
	LibpurpleAdapter::SendResult retVal = LibpurpleAdapter::blockBuddy(m_serviceName.data(), m_username.data(), m_buddyName.data(), block);

	if (LibpurpleAdapter::SENT == retVal) {

		// if the buddy was blocked, remove from buddyStatus and contacts list, otherwise add it
		if (block)
			removeBuddyFromDb();
		// Note: we don't actually have UI to unblock a buddy as of 6/28/2010 so this is not used...
		else addBuddyToDb();
	}

	return retVal;
}

/*
 * removeBuddy command: Tell libpurple to remove the buddy
 * Remove the buddy from buddyStatus and contact DB kinds
 * No parameters for this command
 */
LibpurpleAdapter::SendResult SendOneCommandHandler::removeBuddy(const MojObject imCmd) {

	MojLogInfo (IMServiceApp::s_log, "removeBuddy id: %s, serviceName: %s, username: %s, buddyUsername: %s",
			m_currentCmdDbId.data(), m_serviceName.data(), m_username.data(), m_buddyName.data());

	// Note, libpurple does not return errors on send - adapter only checks that the parameters are valid and that the user is logged in
	// otherwise assume success...
	LibpurpleAdapter::SendResult retVal = LibpurpleAdapter::removeBuddy(m_serviceName.data(), m_username.data(), m_buddyName.data());

	if (LibpurpleAdapter::SENT == retVal) {
		// remove from buddyStatus and contacts list
		// could do this on the libpurple callback, but that gets sent immediately from purple_blist_remove_buddy and doesn't check for errors anyway...
		removeBuddyFromDb();
	}

	return retVal;
}

/*
 * inviteBuddy: tell libpurple to add the buddy
 * add buddy to buddyStatus and contact DB kinds
 *
 * imcommand:
 * {
   "params" : {
      "group"            : {"type"        : "string",
                            "optional"    : true,
                            "description" : "The name of the group to put the buddy in."},
      "message"          : {"type"        : "string",
                            "optional"    : true,
                            "description" : "If the IM transport supports custom invite messages."}
     }
 * }
 *
 */
LibpurpleAdapter::SendResult SendOneCommandHandler::inviteBuddy(const MojObject imCmd) {

	bool found = false;

	// params
	MojObject params;
	MojString group;
	found = imCmd.get(MOJDB_PARAMS, params);
	IMServiceHandler::logMojObjectJsonString(_T("command params: %s"),params);
	if (found) {
		params.get(XPORT_GROUP, group, found);
	}

	MojLogInfo (IMServiceApp::s_log, "sending addBuddy command to transport. id: %s, serviceName: %s, username: %s, buddyUsername: %s, groupName: %s",
			m_currentCmdDbId.data(), m_serviceName.data(), m_username.data(), m_buddyName.data(), group.data());

	// Note, libpurple does not return errors on send - adapter only checks that the parameters are valid and that the user is logged in
	// otherwise assume success...
	LibpurpleAdapter::SendResult retVal = LibpurpleAdapter::addBuddy(m_serviceName.data(), m_username.data(), m_buddyName.data(), group.data());

	// Note: if the remote user rejects our invite, buddy gets deleted on the server but we do not get notified, so the contact will remain, but the buddy list gets re-synced on the next login

	if (LibpurpleAdapter::SENT == retVal) {
		// add to buddyStatus and contacts list. They will get removed later if the invite is rejected
		addBuddyToDb();
	}
	return retVal;
}

/*
 * receivedBuddyInvite - user either accepted or rejected an incoming invitation to be a buddy
 * fromUserName is the user who sent the original invitation
 * Send the accept or reject to the invite to libpurple
 *
 * imcommand:
 * {
   "params" : {
      "accept"           : {"type"        : "boolean",
                            "optional"    : true,
                            "description" : "This is set by the application and indicates whether the invitation was accepted or rejected."}
      "group"            : {"type"        : "string",
                            "description" : "The name of the group to put the buddy in."}
      "message"          : {"type"        : "string",
                            "optional"    : true,
                            "description" : "This is set by the application. It is used if the IM transport supports an invite response message."}
      }
 * }
 *
 * Note: group and custom message are not currectly supported in AIM or GTalk
 */
LibpurpleAdapter::SendResult SendOneCommandHandler::receivedBuddyInvite(const MojObject imCmd) {

//	MojLogError(IMServiceApp::s_log, _T("receiveBuddyInvite - command not implemented in imlippurple"));
//	return false;

	bool found = false;
	LibpurpleAdapter::SendResult retVal = LibpurpleAdapter::SENT;

	// params
	MojObject params;
	bool accept = false;
	found = imCmd.get(MOJDB_PARAMS, params);
	IMServiceHandler::logMojObjectJsonString(_T("command params: %s"),params);
	if (found) {
		found = params.get(XPORT_ACCEPT, accept);
	}

	MojLogInfo (IMServiceApp::s_log, "sending receiveBuddyInvite to transport. id: %s, serviceName: %s, username: %s, buddyName: %s",
			m_currentCmdDbId.data(), m_serviceName.data(), m_username.data(), m_buddyName.data());

	// send invite accept or reject to libpurple
	if (accept)
		retVal = LibpurpleAdapter::authorizeBuddy(m_serviceName.data(), m_username.data(), m_buddyName.data());
	else
		retVal = LibpurpleAdapter::declineBuddy(m_serviceName.data(), m_username.data(), m_buddyName.data());


	if (LibpurpleAdapter::SENT == retVal) {
		// if accept is true, add to buddyStatus and contacts list. Libpurple seems to automatically add the remote buddy to our list
		if (accept)
			addBuddyToDb();

		// otherwise we are done with this command
		else
			m_outgoingIMHandler->messageFinished();
	}
	// note: if there was an error, doSend calls messageFinished...
	return retVal;
}

/*
 * Remove the buddy record from the buddyStatus db
 */
MojErr SendOneCommandHandler::removeBuddyFromDb() {

	// first we have to figure out the accountId - query ImLoginState

	// construct our where clause - find by username and servicename
	MojDbQuery query;
	query.where("serviceName", MojDbQuery::OpEq, m_serviceName);
	query.where("username", MojDbQuery::OpEq, m_username);
	query.from(IM_LOGINSTATE_KIND);

	// call del
	// virtual MojErr del(Signal::SlotRef handler, const MojDbQuery& query,
	//					   MojUInt32 flags = MojDb::FlagNone);
	MojErr err = m_dbClient.find(this->m_findAccountIdForRemoveSlot, query);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("removeBuddyFromDb: dbClient.find() failed: error %d - %s"), err, error.data());
		// query failed - not much we can do here...
		m_outgoingIMHandler->messageFinished();
	}

	return MojErrNone;
}

/*
 * Result of query for loginState entry with given username and serviceName when removing a buddy
 *
 * Remove buddy from buddyStatus and contact DB
 */
MojErr SendOneCommandHandler::findAccountIdForRemoveResult(MojObject& result, MojErr findErr)
{

	if (findErr) {

		MojString error;
		MojErrToString(findErr, error);
		MojLogError(IMServiceApp::s_log, _T("findAccountIdResult failed: error %d - %s"), findErr, error.data());

	} else {

		// parse out the accountId
		readAccountIdFromResults(result);

		if (!m_accountId.empty()){

			// construct our where clause - find by username, accountId and servicename
			MojDbQuery query;
			query.where("username", MojDbQuery::OpEq, m_buddyName);
			query.where("accountId", MojDbQuery::OpEq, m_accountId);
			query.from(IM_BUDDYSTATUS_KIND);

			// call del
			// virtual MojErr del(Signal::SlotRef handler, const MojDbQuery& query,
			//					   MojUInt32 flags = MojDb::FlagNone);
			MojErr err = m_tempdbClient.del(m_removeBuddySlot, query);
			if (err) {
				MojString error;
				MojErrToString(err, error);
				MojLogError(IMServiceApp::s_log, _T("removeBuddy: dbClient.del() failed: error %d - %s"), err, error.data());
				// tell the outgoing Command handler we are done
				m_outgoingIMHandler->messageFinished();
			}
		}
		else {
			MojLogError(IMServiceApp::s_log, _T("findAccountIdResult: no matching loginState record found for %s %s"), m_username.data(), m_serviceName.data());
			// tell the outgoing Command handler we are done
			m_outgoingIMHandler->messageFinished();
		}
	}

	return MojErrNone;
}


/*
 * Callback for DB delete results
 * deleted buddyStatus record
 */
MojErr SendOneCommandHandler::removeBuddyResult(MojObject& result, MojErr delErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (delErr) {
		MojString error;
		MojErrToString(delErr, error);
		MojLogError(IMServiceApp::s_log, _T("removeBuddyResult failed. error %d - %s"), delErr, error.data());
	}
	else {
		MojString json;
		result.toJson(json);
		MojLogInfo(IMServiceApp::s_log, _T("removeBuddyResult response: %s"), json.data());
	}

	// now delete from contacts DB
	//construct our where clause - find by username and servicename
	MojDbQuery query;
	MojErr err;
	err = createContactQuery(query);

	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("removeBuddyResult: createContactsQuery failed: error %d - %s"), err, error.data());
	}

	// call del
	// virtual MojErr del(Signal::SlotRef handler, const MojDbQuery& query,
	//					   MojUInt32 flags = MojDb::FlagNone);
	err = m_dbClient.del(this->m_removeContactSlot, query);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("removeBuddyResult: dbClient.del() failed: error %d - %s"), err, error.data());
		// tell the outgoing Command handler we are done
		m_outgoingIMHandler->messageFinished();
	}
	return MojErrNone;
}

/*
 * Callback for DB delete results
 * deleted buddyStatus record
 */
MojErr SendOneCommandHandler::removeContactResult(MojObject& result, MojErr delErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (delErr) {
		MojString error;
		MojErrToString(delErr, error);
		MojLogError(IMServiceApp::s_log, _T("removeContactResult failed. error %d - %s"), delErr, error.data());
	}
	else {
		MojString json;
		result.toJson(json);
		MojLogInfo(IMServiceApp::s_log, _T("removeContactResult response: %s"), json.data());
	}

	// last callback - tell the outgoing Command handler we are finally done
	m_outgoingIMHandler->messageFinished();

	return MojErrNone;
}

/*
 * Add buddy record to the buddyStatus and contact DB
 */
MojErr SendOneCommandHandler::addBuddyToDb() {

	// first we have to figure out the accountId - query ImLoginState

	//construct our where clause - find by username and servicename
	MojDbQuery query;
	query.where("serviceName", MojDbQuery::OpEq, m_serviceName);
	query.where("username", MojDbQuery::OpEq, m_username);
	query.from(IM_LOGINSTATE_KIND);

	// call del
	// virtual MojErr del(Signal::SlotRef handler, const MojDbQuery& query,
	//					   MojUInt32 flags = MojDb::FlagNone);
	MojErr err = m_dbClient.find(this->m_findAccountIdForAddSlot, query);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("addBuddy: dbClient.find() failed: error %d - %s"), err, error.data());
		// query failed - not much we can do here...
		m_outgoingIMHandler->messageFinished();
	}

	return MojErrNone;
}

/*
 * Parse the account id from the results object for the accountId query
 * saved in m_accountId member variable
 *
 */
MojErr SendOneCommandHandler::readAccountIdFromResults(MojObject& result)
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
	}
	else {
		MojLogError(IMServiceApp::s_log, _T("readAccountIdFromResults: no matching loginState record found for %s %s"), m_username.data(), m_serviceName.data());
	}

	return MojErrNone;
}

/*
 * Result of query for loginState entry with given username and serviceName when adding a buddy
 *
 * Save buddy in buddyStatus and contact DB
 */
MojErr SendOneCommandHandler::findAccountIdForAddResult(MojObject& result, MojErr findErr)
{

	if (findErr) {

		MojString error;
		MojErrToString(findErr, error);
		MojLogError(IMServiceApp::s_log, _T("findAccountIdResult failed: error %d - %s"), findErr, error.data());

	} else {

		// parse out the accountId
		readAccountIdFromResults(result);

		if (!m_accountId.empty()){
			MojObject buddyStatus;
			buddyStatus.putString("_kind", IM_BUDDYSTATUS_KIND);
			buddyStatus.put("accountId", m_accountId);
			buddyStatus.put("username", m_buddyName);
			buddyStatus.put("serviceName", m_serviceName);

			// log it
			MojString json;
			buddyStatus.toJson(json);
			MojLogInfo(IMServiceApp::s_log, _T("saving buddy status to db: %s"), json.data());

			// save it
			// the save generates a call to the save result handler
			MojErr err = m_tempdbClient.put(m_addBuddySlot, buddyStatus);
			if (err) {
				MojString error;
				MojErrToString(err, error);
				MojLogError(IMServiceApp::s_log, _T("findAccountIdResult: dbClient.put() failed: error %d - %s"), err, error.data());
				// tell the outgoing Command handler we are done
				m_outgoingIMHandler->messageFinished();
			}
		}
		else {
			MojLogError(IMServiceApp::s_log, _T("findAccountIdResult: no matching loginState record found for %s %s"), m_username.data(), m_serviceName.data());
			// tell the outgoing Command handler we are done
			m_outgoingIMHandler->messageFinished();
		}
	}

	return MojErrNone;
}


/**
 * Callback from the save buddy in buddyStatus DB call.
 *
 * Now save in contacts db
 */
MojErr SendOneCommandHandler::addBuddyResult(MojObject& result, MojErr saveErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (saveErr) {
		MojString error;
		MojErrToString(saveErr, error);
		MojLogError(IMServiceApp::s_log, _T("addBuddyResult failed. error %d - %s"), saveErr, error.data());
	}
	else {
		IMServiceHandler::logMojObjectJsonString(_T("addBuddyResult success: %s"), result);
	}

	// now add to contacts DB
	// create the DB object
	//{
	//    accountId:<>,
	//    displayName:<>,
	//    photos:[{localFilepath:<>}],
	//    ims:[{value:<>, type:<>}]
	// }
	MojObject contact;
	contact.putString("_kind", IM_CONTACT_KIND);
	contact.put("accountId", m_accountId);
	contact.put("remoteId", m_buddyName); // using username as remote ID since we don't have anything else and this should be unique

	// "ims" array
	MojObject newImsArray, newImObj;
	newImObj.put("value", m_buddyName);
	newImObj.put("type", m_serviceName);
	// Note we need this twice since some services look for "serviceName" and some look for "type"... Maybe we can evenutally standardize...
	newImObj.put("serviceName", m_serviceName);
	newImsArray.push(newImObj);
	contact.put("ims", newImsArray);

	// need to add email address too for contacts linker
	//    email:[{value:<>}]
	MojString emailAddr;
	bool validEmail = true;
	emailAddr.assign(m_buddyName.data());
	// for AIM, need to add back the "@aol.com" to the email
	if (MojInvalidIndex == m_buddyName.find('@')) {
		if (0 == m_serviceName.compare(SERVICENAME_AIM))
			emailAddr.append("@aol.com");
		else
			validEmail = false;
	}

	if (validEmail) {
		MojObject newEmailArray, newEmailObj;
		newEmailObj.put("value", emailAddr);
		newEmailArray.push(newEmailObj);
		contact.put("emails", newEmailArray);
	}

	// log it
	MojString json;
	contact.toJson(json);
	MojLogInfo(IMServiceApp::s_log, _T("saving contact to db: %s"), json.data());

	// save it
	// the save generates a call to the save result handler
	MojErr err = m_dbClient.put(this->m_addContactSlot, contact);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("addBuddyResult: dbClient.put() failed: error %d - %s"), err, error.data());
		// tell the outgoing Command handler we are done
		m_outgoingIMHandler->messageFinished();
	}

	return MojErrNone;
}

/**
 * Callback from the save buddy in contact DB call.
 */
MojErr SendOneCommandHandler::addContactResult(MojObject& result, MojErr saveErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (saveErr) {
		MojString error;
		MojErrToString(saveErr, error);
		MojLogError(IMServiceApp::s_log, _T("addContactResult failed. error %d - %s"), saveErr, error.data());
	}
	else {
		IMServiceHandler::logMojObjectJsonString(_T("addContactResult success: %s"), result);
	}

	// last callback - tell the outgoing Command handler we are finally done
	m_outgoingIMHandler->messageFinished();

	return MojErrNone;
}

/*
 * Create the query to find the buddy in contacts table by username and serviceName
 *
 * errors returned to caller
 */
MojErr SendOneCommandHandler::createContactQuery(MojDbQuery& query)
{

	MojErr err = query.where("ims.value", MojDbQuery::OpEq, m_buddyName);
	MojErrCheck(err);

	err = query.where("ims.type", MojDbQuery::OpEq, m_serviceName);
	MojErrCheck(err);

	//add our kind to the object
	err = query.from(IM_CONTACT_KIND);
	MojErrCheck(err);

	// log the query
	MojObject queryObject;
	err = query.toObject(queryObject);
	MojErrCheck(err);
	IMServiceHandler::logMojObjectJsonString(_T("createContactQuery: %s"), queryObject);

	return MojErrNone;
}

