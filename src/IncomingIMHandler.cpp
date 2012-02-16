/*
 * IncomingIMHandler.cpp
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
 * IncomingIMHandler class handles incoming IM messages
 */


#include "IncomingIMHandler.h"
#include "db/MojDbQuery.h"
#include "IMServiceApp.h"

/*
 * Note the order of the globals inited below, it matters
 */
IncomingIMHandler::IncomingIMHandler(MojService* service)
: m_IMSaveMessageSlot(this, &IncomingIMHandler::IMSaveMessageResult),
  m_service(service),
  m_dbClient(service)
{

}


IncomingIMHandler::~IncomingIMHandler() {

}

/**
 * callback from the save message call.
 */
MojErr IncomingIMHandler::IMSaveMessageResult(MojObject& result, MojErr saveErr)
{
	MojLogTrace(IMServiceApp::s_log);

	if (saveErr) {
		MojString error;
		MojErrToString(saveErr, error);
		MojLogError(IMServiceApp::s_log, _T("database put failed. error %d - %s"), saveErr, error.data());
	}
	else {
		IMServiceHandler::logMojObjectJsonString(_T("database put success: %s"), result);
	}

	return MojErrNone;
}

/*
 * Save the new incoming IM message to the DB
 *
 * @return MojErr if error - caller will log it
 */
MojErr IncomingIMHandler::saveNewIMMessage(MojRefCountedPtr<IMMessage> IMMessage) {

	MojErr err;

	// The message we are handling
	m_IMMessage = IMMessage;

	// save it
	MojObject dbObject;
	err = m_IMMessage->createDBObject(dbObject);
	MojErrCheck(err);

	//add our kind to the object
	//luna://com.palm.db/put '{"objects":[{"_kind":"com.palm.test:1","foo":1,"bar":1000}]}'
	err = dbObject.putString(_T("_kind"), PALM_DB_IMMESSAGE_KIND);
	MojErrCheck(err);

	// log it - OK to show body in debug log
	MojString json;
	err = dbObject.toJson(json);
	MojErrCheck(err);
	MojLogDebug(IMServiceApp::s_log, _T("saving message to db: %s"), json.data());
	MojLogInfo(IMServiceApp::s_log, _T("saving message to db"));

	// save it
	// the save generates a call to the save result handler
	err = m_dbClient.put(this->m_IMSaveMessageSlot, dbObject);
	MojErrCheck(err);

	return MojErrNone;
}

