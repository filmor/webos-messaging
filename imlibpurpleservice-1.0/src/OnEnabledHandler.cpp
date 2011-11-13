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
#include "IMDefines.h"

OnEnabledHandler::OnEnabledHandler(MojService* service)
: m_getAccountInfoSlot(this, &OnEnabledHandler::getAccountInfoResult),
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
}

OnEnabledHandler::~OnEnabledHandler()
{
}

/*
 * 1. get account details from accountservices or db?
 */
MojErr OnEnabledHandler::start(const MojObject& payload)
{

	IMServiceHandler::logMojObjectJsonString(_T("OnEnabledHandler payload: %s"), payload);

	MojString accountId;
	MojErr err = payload.getRequired(_T("accountId"), accountId);
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
		params.put(_T("accountId"), accountId);
		err = req->send(m_getAccountInfoSlot, "com.palm.service.accounts", "getAccountInfo", params, 1);
		if (err) {
			MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::start: getAccountInfo id %s failed. error %d"), accountId.data(), err);
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

	MojString accountId;
	err = result.getRequired("_id", accountId);
	if (err != MojErrNone || accountId.empty()) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::getAccountInfoResult accountId empty or error %d"), err);
		return err;
	}

	MojString username;
	err = result.getRequired("username", username);
	if (err != MojErrNone || username.empty()) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::getAccountInfoResult username empty or error %d"), err);
		return err;
	}

	MojString serviceName;
	getServiceNameFromCapabilityId(serviceName);
	if (serviceName.empty()) {
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler::getAccountInfoResult serviceName empty"));
		return err;
	}

	if (m_enable) {
		err = accountEnabled(accountId, serviceName, username);
	} else {
		err = accountDisabled(accountId, serviceName, username);
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
		if (templateId == "org.webosinternals.messaging.aol.aim")
			serviceName.assign(SERVICENAME_AIM);
		else if (templateId == "org.webosinternals.messaging.facebook")
			serviceName.assign(SERVICENAME_FACEBOOK);
		else if (templateId == "org.webosinternals.messaging.google.talk")
			serviceName.assign(SERVICENAME_GTALK);
		else if (templateId == "org.webosinternals.messaging.gadu")
			serviceName.assign(SERVICENAME_GADU);
		else if (templateId == "org.webosinternals.messaging.groupwise")
			serviceName.assign(SERVICENAME_GROUPWISE);
		else if (templateId == "org.webosinternals.messaging.icq")
			serviceName.assign(SERVICENAME_ICQ);
		else if (templateId == "org.webosinternals.messaging.jabber")
			serviceName.assign(SERVICENAME_JABBER);
		else if (templateId == "org.webosinternals.messaging.live")
			serviceName.assign(SERVICENAME_LIVE);
		else if (templateId == "org.webosinternals.messaging.wlm")
			serviceName.assign(SERVICENAME_WLM);
		else if (templateId == "org.webosinternals.messaging.myspace")
			serviceName.assign(SERVICENAME_MYSPACE);
		else if (templateId == "org.webosinternals.messaging.qq")
			serviceName.assign(SERVICENAME_QQ);
		else if (templateId == "org.webosinternals.messaging.sametime")
			serviceName.assign(SERVICENAME_SAMETIME);
		else if (templateId == "org.webosinternals.messaging.sipe")
			serviceName.assign(SERVICENAME_SIPE);
		else if (templateId == "org.webosinternals.messaging.xfire")
			serviceName.assign(SERVICENAME_XFIRE);
		else if (templateId == "org.webosinternals.messaging.yahoo")
			serviceName.assign(SERVICENAME_YAHOO);
		else
			err = MojErrNotImpl;
	}
	return err;
}

void OnEnabledHandler::getServiceNameFromCapabilityId(MojString& serviceName) 
{
	if (m_capabilityProviderId == CAPABILITY_AIM)
		serviceName.assign(SERVICENAME_AIM);	
	else if (m_capabilityProviderId == CAPABILITY_FACEBOOK)
		serviceName.assign(SERVICENAME_FACEBOOK);
	else if (m_capabilityProviderId == CAPABILITY_GTALK)
		serviceName.assign(SERVICENAME_GTALK);
	else if (m_capabilityProviderId == CAPABILITY_GADU)
		serviceName.assign(SERVICENAME_GADU);
	else if (m_capabilityProviderId == CAPABILITY_GROUPWISE)
		serviceName.assign(SERVICENAME_GROUPWISE);
	else if (m_capabilityProviderId == CAPABILITY_ICQ)
		serviceName.assign(SERVICENAME_ICQ);
	else if (m_capabilityProviderId == CAPABILITY_JABBER)
		serviceName.assign(SERVICENAME_JABBER);
	else if (m_capabilityProviderId == CAPABILITY_LIVE)
		serviceName.assign(SERVICENAME_LIVE);
	else if (m_capabilityProviderId == CAPABILITY_WLM)
		serviceName.assign(SERVICENAME_WLM);	
	else if (m_capabilityProviderId == CAPABILITY_MYSPACE)
		serviceName.assign(SERVICENAME_MYSPACE);
	else if (m_capabilityProviderId == CAPABILITY_QQ)
		serviceName.assign(SERVICENAME_QQ);
	else if (m_capabilityProviderId == CAPABILITY_SAMETIME)
		serviceName.assign(SERVICENAME_SAMETIME);
	else if (m_capabilityProviderId == CAPABILITY_SIPE)
		serviceName.assign(SERVICENAME_SIPE);
	else if (m_capabilityProviderId == CAPABILITY_XFIRE)
		serviceName.assign(SERVICENAME_XFIRE);
	else if (m_capabilityProviderId == CAPABILITY_YAHOO)
		serviceName.assign(SERVICENAME_YAHOO);
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
MojErr OnEnabledHandler::accountEnabled(const MojString& accountId, const MojString& serviceName, const MojString& username)
{
	//TODO: first issue a merge in case the account already exists?
	MojLogTrace(IMServiceApp::s_log);
	MojLogInfo(IMServiceApp::s_log, _T("accountEnabled id=%s, serviceName=%s"), accountId.data(), serviceName.data());

	MojObject imLoginState;
	imLoginState.putString(_T("_kind"), IM_LOGINSTATE_KIND);
	imLoginState.put(_T("accountId"), accountId);
	imLoginState.put(_T("serviceName"), serviceName);
	imLoginState.put(_T("username"), username);
	imLoginState.putString(_T("state"), LOGIN_STATE_OFFLINE);
	imLoginState.putInt(_T("availability"), PalmAvailability::ONLINE); //default to online so we automatically login at first
	MojErr err = m_dbClient.put(m_addImLoginStateSlot, imLoginState);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.put() failed: error %d - %s"), err, error.data());
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
MojErr OnEnabledHandler::accountDisabled(const MojString& accountId, const MojString& serviceName, const MojString& username)
{
	MojLogTrace(IMServiceApp::s_log);
	MojLogInfo(IMServiceApp::s_log, _T("accountDisabled id=%s, serviceName=%s"), accountId.data(), serviceName.data());

	// delete com.palm.imloginstate.libpurple record
	MojDbQuery queryLoginState;
	queryLoginState.from(IM_LOGINSTATE_KIND);
	queryLoginState.where(_T("serviceName"), MojDbQuery::OpEq, serviceName);
	queryLoginState.where(_T("username"), MojDbQuery::OpEq, username);
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
	queryLoginState.where(_T("serviceName"), MojDbQuery::OpEq, serviceName);
	queryLoginState.where(_T("username"), MojDbQuery::OpEq, username);
	err = m_dbClient.del(m_deleteImMessagesSlot, queryMessage);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(immessage) failed: error %d - %s"), err, error.data());
	}

	// delete com.palm.imcommand.libpurple records
	MojDbQuery queryCommand;
	queryCommand.from(IM_IMCOMMAND_KIND);
	queryLoginState.where(_T("serviceName"), MojDbQuery::OpEq, serviceName);
	queryLoginState.where(_T("username"), MojDbQuery::OpEq, username);
	err = m_dbClient.del(m_deleteImMessagesSlot, queryCommand);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(imcommand) failed: error %d - %s"), err, error.data());
	}

	// delete com.palm.contact.libpurple record
	MojDbQuery queryContact;
	queryContact.from(IM_CONTACT_KIND);
	queryContact.where(_T("accountId"), MojDbQuery::OpEq, accountId);
	err = m_dbClient.del(m_deleteContactsSlot, queryContact);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(contact) failed: error %d - %s"), err, error.data());
	}

	// delete com.palm.imbuddystatus.libpurple record
	MojDbQuery queryBuddyStatus;
	queryBuddyStatus.from(IM_BUDDYSTATUS_KIND);
	queryBuddyStatus.where(_T("accountId"), MojDbQuery::OpEq, accountId);
	err = m_tempdbClient.del(m_deleteImBuddyStatusSlot, queryBuddyStatus);
	if (err != MojErrNone) {
		MojString error;
		MojErrToString(err, error);
		MojLogError(IMServiceApp::s_log, _T("OnEnabledHandler db.del(imbuddystatus) failed: error %d - %s"), err, error.data());
	}

	// now we need to tell libpurple to disconnect the account so we don't get more messages for it
	// LoginCallbackInterface is null because we don't need to do any processing on the callback - all the DB kinds are already gone.
	LibpurpleAdapter::logout(serviceName, username, NULL);

	return MojErrNone;
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

