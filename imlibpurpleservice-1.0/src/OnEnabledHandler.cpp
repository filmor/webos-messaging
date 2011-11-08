/*
 * OnEnabledHandler.cpp
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

#include "OnEnabledHandler.h"
#include "db/MojDbQuery.h"
#include "IMServiceApp.h"
#include "IMServiceHandler.h"
#include "IMDefines.h"
#include "PalmImCommon.h"

OnEnabledHandler::OnEnabledHandler(MojService* service, IMServiceApp::Listener* listener)
: m_getAccountInfoSlot(this, &OnEnabledHandler::getAccountInfoResult),
  m_findImLoginStateSlot(this, &OnEnabledHandler::findImLoginStateResult),
  m_addImLoginStateSlot(this, &OnEnabledHandler::addImLoginStateResult),
  m_deleteImLoginStateSlot(this, &OnEnabledHandler::deleteImLoginStateResult),
  m_deleteImMessagesSlot(this, &OnEnabledHandler::deleteImMessagesResult),
  m_deleteImCommandsSlot(this, &OnEnabledHandler::deleteImCommandsResult),
  m_deleteContactsSlot(this, &OnEnabledHandler::deleteContactsResult),
  m_deleteImBuddyStatusSlot(this, &OnEnabledHandler::deleteImBuddyStatusResult),
  m_service(service),
  m_dbClient(service, MojDbServiceDefs::ServiceName),
  m_tempdbClient(service, MojDbServiceDefs::TempServiceName),
  m_enable(false)
{
	// tell listener we are now active
	m_listener = listener;
	m_listener->ProcessStarting();
}

OnEnabledHandler::~OnEnabledHandler()
{
	// tell listener we are done
	m_listener->ProcessDone();
}

/*
 * 1. get account details from accountservices or db?
 */
MojErr OnEnabledHandler::start(const MojObject& payload)
{

	IMServiceHandler::logMojObjectJsonString(_T("OnEnabledHandler payload: %s"), payload);

	MojErr err = payload.getRequired(_T("accountId"), m_accountId);
	if (err != MojErrNone) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler accountId missing so bailing. error %d"), err);
		return err;
	}

	err = payload.getRequired(_T("enabled"), m_enable);
	if (err != MojErrNone) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler enabled missing, assuming 'false'"));
		m_enable = false;
	}
	
	err = payload.getRequired(_T("capabilityProviderId"), m_capabilityProviderId);
	if (err != MojErrNone) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler capabilityProviderId missing so bailing. error %d"), err);
		return err;
	}

	MojRefCountedPtr<MojServiceRequest> req;
	err = m_service->createRequest(req);
	if (err != MojErrNone) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::start createRequest failed. error %d"), err);
	} else {
		MojObject params;
		params.put(_T("accountId"), m_accountId);
		err = req->send(m_getAccountInfoSlot, "com.palm.service.accounts", "getAccountInfo", params, 1);
		if (err) {
			MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::start: getAccountInfo id %s failed. error %d"), m_accountId.data(), err);
		}
	}

	return MojErrNone;
}

/*
 */
MojErr  OnEnabledHandler::getAccountInfoResult(MojObject& payload, MojErr resultErr)
{
	MojLogTrace(IMServiceApp::s_log);
	IMServiceHandler::logMojObjectJsonString(_T("OnEnabledHandler::getAccountInfoResult payload: %s"), payload);

	if (resultErr != MojErrNone) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler getAccountInfo result error %d"), resultErr);
		return resultErr;
	}

	MojObject result;
	MojErr err = payload.getRequired("result", result);
	if (err != MojErrNone || result.empty()) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::getAccountInfoResult result empty or error %d"), err);
		return err;
	}

	err = result.getRequired("_id", m_accountId);
	if (err != MojErrNone || m_accountId.empty()) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::getAccountInfoResult accountId empty or error %d"), err);
		return err;
	}

	err = result.getRequired("username", m_username);
	if (err != MojErrNone || m_username.empty()) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::getAccountInfoResult username empty or error %d"), err);
		return err;
	}

	getServiceNameFromCapabilityId(m_serviceName);
	if (m_serviceName.empty()) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::getAccountInfoResult serviceName empty"));
		return err;
	}

	if (m_enable) {
		err = accountEnabled();
	} else {
		err = accountDisabled();
	}

	return err;
}

MojErr OnEnabledHandler::getDefaultServiceName(const MojObject& accountResult, MojString& serviceName)
{
	MojString templateId;
	MojErr err = accountResult.getRequired("templateId", templateId);
	if (err != MojErrNone) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler templateId empty or error %d"), err);
	} else {
		if (templateId == "com.palm.google")
			serviceName.assign(SERVICENAME_GTALK);
		else if (templateId == "com.palm.aol")
			serviceName.assign(SERVICENAME_AIM);
		else if (templateId == "com.palm.icq")
			serviceName.assign(SERVICENAME_ICQ);
		else
			err = MojErrNotImpl;
	}
	return err;
}

void OnEnabledHandler::getServiceNameFromCapabilityId(MojString& serviceName) 
{
	if (m_capabilityProviderId == CAPABILITY_GTALK)
		serviceName.assign(SERVICENAME_GTALK);	
	else if (m_capabilityProviderId == CAPABILITY_AIM)
		serviceName.assign(SERVICENAME_AIM);

}

/*
 * Example messaging capability provider
 *  "capabilityProviders": [{
            "_id": "2+MR",
            "capability": "MESSAGING",
            "id": "com.palm.google.talk",
            "capabilitySubtype": "IM",
            "loc_name": "Google Talk",
            "icon": {
                "loc_32x32": "/usr/palm/public/accounts/com.palm.google/images/gtalk32x32.png",
                "loc_48x48": "/usr/palm/public/accounts/com.palm.google/images/gtalk48x48.png",
                "splitter": "/usr/palm/public/accounts/com.palm.google/images/gtalk_transport_splitter.png"
            },
            "implementation": "palm://com.palm.imlibpurple/",
            "onEnabled": "palm://com.palm.imlibpurple/onEnabled"
            "serviceName":"type_aim",
            "dbkinds": {
                "immessage":"com.palm.immessage.libpurple:1",
                "imcommand":"com.palm.imcommand.libpurple:1"
            }
    }],
 */
MojErr OnEnabledHandler::getMessagingCapabilityObject(const MojObject& capabilityProviders, MojObject& messagingObj)
{
	// iterate thru the capabilities array
	MojErr err = MojErrRequiredPropNotFound;
	MojString capability;
	MojObject::ConstArrayIterator itr = capabilityProviders.arrayBegin();
	// This shouldn't happen, but check if there's nothing to do.
	while (itr != capabilityProviders.arrayEnd()) {
		messagingObj = *itr;
		err = messagingObj.getRequired("capability", capability);
		if (capability == "MESSAGING") {
			err = MojErrNone;
			break;
		}
		++itr;
	}

	return err;
}

/*
 * Enabling an IM account requires the following
 *     add com.palm.imloginstate.libpurple record
 */
MojErr OnEnabledHandler::accountEnabled()
{
	MojLogTrace(IMServiceApp::s_log);
	MojLogInfo(IMServiceApp::s_log, _T("accountEnabled id=%s, serviceName=%s"), m_accountId.data(), m_serviceName.data());

	// first see if an imloginstate record already exists for this account. This can happen on an OTA update or restore
	// construct our where clause - find by username and servicename
	MojDbQuery query;
	query.where("serviceName", MojDbQuery::OpEq, m_serviceName);
	query.where("username", MojDbQuery::OpEq, m_username);
	query.from(IM_LOGINSTATE_KIND);

	MojErr err = m_dbClient.find(this->m_findImLoginStateSlot, query);
	if (err) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("accountEnabled: dbClient.find() failed: error %d - %s"), err, error.data());
		// query failed - not much we can do here...
	}

	return err;
}

/*
 * Disabling an account means doing the following
 *     delete com.palm.imloginstate.libpurple record
 *     delete com.palm.immessage.libpurple records
 *     delete com.palm.imcommand.libpurple records
 *     delete com.palm.contact.libpurple records
 *     delete com.palm.imbuddystatus.libpurple records
 *     delete com.palm.imgroupchat.libpurple records -- groupchats not currently supported
 * Note: the ChatThreader service takes care of removing empty chats
 */
MojErr OnEnabledHandler::accountDisabled()
{
	MojLogTrace(IMServiceApp::s_log);
	MojLogInfo(IMServiceApp::s_log, _T("accountDisabled id=%s, serviceName=%s"), m_accountId.data(), m_serviceName.data());

	// delete com.palm.imloginstate.libpurple record
	MojDbQuery queryLoginState;
	queryLoginState.from(IM_LOGINSTATE_KIND);

	// we can get multiple calls to onEnabled(false) even after we have deleted the account records, so search by account Id so
	// we don't delete a new account with same username/servicename (see DFISH-16682)
//	queryLoginState.where(_T("serviceName"), MojDbQuery::OpEq, m_serviceName);
//	queryLoginState.where(_T("username"), MojDbQuery::OpEq, m_username);
	queryLoginState.where(_T("accountId"), MojDbQuery::OpEq, m_accountId);
	MojErr err = m_dbClient.del(m_deleteImLoginStateSlot, queryLoginState);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(imloginstate) failed: error %d - %s"), err, error.data());
	}

	// delete com.palm.immessage.libpurple records
	//TODO: need to query both from & recipient addresses. Simplify this???
	MojDbQuery queryMessage;
	queryMessage.from(IM_IMMESSAGE_KIND);
	queryMessage.where(_T("serviceName"), MojDbQuery::OpEq, m_serviceName);
	queryMessage.where(_T("username"), MojDbQuery::OpEq, m_username);
	err = m_dbClient.del(m_deleteImMessagesSlot, queryMessage);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(immessage) failed: error %d - %s"), err, error.data());
	}

	// delete com.palm.imcommand.libpurple records
	MojDbQuery queryCommand;
	queryCommand.from(IM_IMCOMMAND_KIND);
	queryCommand.where(_T("serviceName"), MojDbQuery::OpEq, m_serviceName);
	queryCommand.where(_T("fromUsername"), MojDbQuery::OpEq, m_username);
	err = m_dbClient.del(m_deleteImCommandsSlot, queryCommand);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(imcommand) failed: error %d - %s"), err, error.data());
	}

	// delete com.palm.contact.libpurple record
	MojDbQuery queryContact;
	queryContact.from(IM_CONTACT_KIND);
	queryContact.where(_T("accountId"), MojDbQuery::OpEq, m_accountId);
	err = m_dbClient.del(m_deleteContactsSlot, queryContact);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(contact) failed: error %d - %s"), err, error.data());
	}

	// delete com.palm.imbuddystatus.libpurple record
	MojDbQuery queryBuddyStatus;
	queryBuddyStatus.from(IM_BUDDYSTATUS_KIND);
	queryBuddyStatus.where(_T("accountId"), MojDbQuery::OpEq, m_accountId);
	err = m_tempdbClient.del(m_deleteImBuddyStatusSlot, queryBuddyStatus);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(imbuddystatus) failed: error %d - %s"), err, error.data());
	}

	// now we need to tell libpurple to disconnect the account so we don't get more messages for it
	// LoginCallbackInterface is null because we don't need to do any processing on the callback - all the DB kinds are already gone.
	LibpurpleAdapter::logout(m_serviceName, m_username, NULL);

	return MojErrNone;
}

MojErr OnEnabledHandler::findImLoginStateResult(MojObject& payload, MojErr err)
{
	MojLogTrace(IMServiceApp::s_log);

	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogCritical(IMServiceApp::s_log, _T("findImLoginStateResult failed: error %d - %s"), err, error.data());
	}
	else {
		// "results" in result
		MojObject results;
		payload.get(_T("results"), results);

		// check to see if array is empty - normally it will be if this is a newly created account. There should never be more than 1 item here
		if (!results.empty()){

			IMServiceHandler::logMojObjectJsonString(_T("findImLoginStateResult found existing imLoginState record: %s"), payload);

			// if there is a record already, make sure the account id matches.
			MojObject loginState;
			MojObject::ConstArrayIterator itr = results.arrayBegin();
			bool foundOne = false;
			while (itr != results.arrayEnd()) {
				if (foundOne) {
					MojLogError(IMServiceApp::s_log,
							_T("findImLoginStateResult: found more than one ImLoginState with same username/serviceName - using the first one"));
					break;
				}
				loginState = *itr;
				foundOne = true;
				itr++;
			}

			MojString accountId;
			MojErr err = loginState.getRequired("accountId", accountId);
			if (err) {
				MojLogError(IMServiceApp::s_log, _T("findImLoginStateResult: missing accountId in loginState entry"));
			}
			if (0 != accountId.compare(m_accountId)) {
				MojLogError(IMServiceApp::s_log, _T("findImLoginStateResult: existing loginState record does not have matching account id. accountId = %s"), accountId.data());

				// delete this record
				MojObject idsToDelete;
				MojString dbId;
				err = loginState.getRequired("_id", dbId);
				if (err) {
					MojLogError(IMServiceApp::s_log, _T("findImLoginStateResult: missing dbId in loginState entry"));
				}
				else {
				    idsToDelete.push(dbId);

					// luna://com.palm.db/del '{"ids":[2]}'
					MojLogInfo(IMServiceApp::s_log, _T("findImLoginStateResult: deleting loginState entry id: %s"), dbId.data());
					err = m_dbClient.del(this->m_deleteImLoginStateSlot, idsToDelete.arrayBegin(), idsToDelete.arrayEnd());
					if (err != MojErrNone) {
						MojString error;
						MojErrToString(err, error);
						MojLogError(IMServiceApp::s_log, _T("findImLoginStateResult: db.del() failed: error %d - %s"), err, error.data());
					}
				}
			}
			// if the account id matches, leave the old record and we are done
			else return MojErrNone;
		}

		// no existing record found or the old one was deleted - create a new one
		MojLogInfo(IMServiceApp::s_log, _T("findImLoginStateResult: no matching loginState record found for %s, %s. creating a new one"), m_username.data(), m_serviceName.data());
		MojObject imLoginState;
		imLoginState.putString(_T("_kind"), IM_LOGINSTATE_KIND);
		imLoginState.put(_T("accountId"), m_accountId);
		imLoginState.put(_T("serviceName"), m_serviceName);
		imLoginState.put(_T("username"), m_username);
		imLoginState.putString(_T("state"), LOGIN_STATE_OFFLINE);
		imLoginState.putInt(_T("availability"), PalmAvailability::ONLINE); //default to online so we automatically login at first
		MojErr err = m_dbClient.put(m_addImLoginStateSlot, imLoginState);
		if (err != MojErrNone) {
			MojString error;
			MojErrToString(err, error);
			MojLogError(IMServiceApp::s_log, _T("findImLoginStateResult: db.put() failed: error %d - %s"), err, error.data());
		}

	}
	return err;
}

MojErr OnEnabledHandler::addImLoginStateResult(MojObject& payload, MojErr err)
{
	MojLogTrace(IMServiceApp::s_log);

	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogCritical(IMServiceApp::s_log, _T("addLoginStateResult failed: error %d - %s"), err, error.data());
		//TODO retry adding the record. Not worrying about this for now because the add would only fail in
		// extreme conditions.
	}
	return err;
}


MojErr OnEnabledHandler::deleteImLoginStateResult(MojObject& payload, MojErr err)
{
	MojLogTrace(IMServiceApp::s_log);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogCritical(IMServiceApp::s_log, _T("deleteImLoginStateResult failed: error %d - %s"), err, error.data());
	}
	return err;
}

MojErr OnEnabledHandler::deleteImMessagesResult(MojObject& payload, MojErr err)
{
	MojLogTrace(IMServiceApp::s_log);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogCritical(IMServiceApp::s_log, _T("deleteImMessagesResult failed: error %d - %s"), err, error.data());
	}
	return err;
}

MojErr OnEnabledHandler::deleteImCommandsResult(MojObject& payload, MojErr err)
{
	MojLogTrace(IMServiceApp::s_log);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogCritical(IMServiceApp::s_log, _T("deleteImCommandsResult failed: error %d - %s"), err, error.data());
	}
	return err;
}

MojErr OnEnabledHandler::deleteContactsResult(MojObject& payload, MojErr err)
{
	MojLogTrace(IMServiceApp::s_log);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogCritical(IMServiceApp::s_log, _T("deleteContactsResult failed: error %d - %s"), err, error.data());
	}
	return err;
}

MojErr OnEnabledHandler::deleteImBuddyStatusResult(MojObject& payload, MojErr err)
{
	MojLogTrace(IMServiceApp::s_log);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogCritical(IMServiceApp::s_log, _T("deleteImBuddyStatusResult failed: error %d - %s"), err, error.data());
	}
	return err;
}

