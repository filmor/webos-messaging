/*
 * IMMessage.cpp
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
 * IMMessage class handles saving and retrieving an immessage object from the DB
 */

#include "IMMessage.h"
#include "IMServiceHandler.h"
#include "sanitize.h"

const char* IMMessage::statusStrings[] = {
	"successful",
	"pending",
	"failed",
	"permanent-fail",
	"waiting-for-connection",
};

const char* IMMessage::folderStrings[] = {
	"inbox",
	"outbox",
	"drafts",
	"system",
	"transient"
};

// allowable html tags to be ignored by the sanitizer
const char* IMMessage::trustedTags[] = {"b", "/b", "i", "/i", "br", "/br", "u", "/u", "B", "/B", "I", "/I", "BR", "/BR", "U", "/U",NULL};
//just for testing
//const char* IMMessage::trustedTags[] = {"b", "/b", "i", "/i", "br", "/br", "u", "/u", "B", "/B", "I", "/I", "BR", "/BR", "U", "/U", "body", "/body", NULL};

IMMessage::IMMessage()
{
	deviceTimestamp = 0;
	serverTimestamp = 0;
	status = Successful;
	folder = Inbox;
}

IMMessage::~IMMessage() {

}

/*
 * Initialize from an incoming libPurple callback parameters
 *
 * payload form: (Sprint)
 * { "serviceName": "gmail", "username": "xxx@gmail.com", "usernameFrom": "palm@gmail.com", "messageText": "<body>test reply to phone<\/body>" }
 * { "serviceName": "aol", "username": "xxx@aol.com", "usernameFrom": "palm", "messageText": "test incoming message" }
 *
 *
 */
MojErr IMMessage::initFromCallback(const char* serviceName, const char* username, const char* usernameFrom, const char* message) {

	MojErr err;

	// unescape, strip html
	//    #/tmp/testsanitize "<a>foo</a>"
	//    sanitized: &lt;a&gt;foo&lt;/a&gt;
	//    unsanitized: <a>foo</a>
	//    removed: foo

	// first unescape since gtalk sends everything escaped
	// message is const char* - need a char* version
	char *unescapedMessage = unsanitizeHtml((char*)message);

	// now remove offending html
	// char * sanitizeHtml(const char *input, char **except, bool remove);
	//     remove set to true if you want the the tags to actually be removed vs just "escaped".
	//     except is an array of char* containing tags to ignore when sanitizing. The
	//          last element must be a null. You need to include both beginning and ending
	//          tag if you want both removed (i.e. "b", "/b", "i", "/i")
	char *sanitizedMessage = sanitizeHtml(unescapedMessage, (char**)IMMessage::trustedTags, true);

	// can't keep this log...
	//MojLogInfo(IMServiceApp::s_log, _T("original message: %s, unescaped message: %s, sanitized message: %s"), message, unescapedMessage, sanitizedMessage);

	err = msgText.assign(sanitizedMessage);
	MojErrCheck(err);
	// cleanup
	free(unescapedMessage);
	free(sanitizedMessage);

	// remove blanks and convert to lowercase
	// for AOL, this is screen name with no "@aol.com"...
	MojString formattedUserName;
	err = formattedUserName.assign(usernameFrom);
	MojErrCheck(err);
	err = unformatFromAddress(formattedUserName, fromAddress);
	MojErrCheck(err);

	err = toAddress.assign(username);
	MojErrCheck(err);
	err = msgType.assign(serviceName);
	MojErrCheck(err);

	// set time stamp to current time - seconds since 1/1/1970
	// for some reason multiplying a time_t by 1000 doesn't work - you have to convert to a long first...
	//deviceTimestamp = time (NULL) * 1000;
	MojInt64 sec = time (NULL);
	deviceTimestamp = sec * 1000;
	// server timestamp is new Date().getTime()
	serverTimestamp = sec * 1000;

	return MojErrNone;
}


/**
 * Return MojObject formated to save to MojDb
 * Used for incoming messages
 */
MojErr IMMessage::createDBObject(MojObject& returnObj) {

	MojLogInfo(IMServiceApp::s_log, _T("createMojObject"));

	// Communications kind
	// From/to address is itself a CommunicationsAddress object
	// "name" and "type" properties are optional
	// Sender
	MojObject addressObj;
	MojErr err = addressObj.putString(MOJDB_ADDRESS, fromAddress);
	MojErrCheck(err);
	err = returnObj.put(MOJDB_FROM, addressObj);
	MojErrCheck(err);

	// Recipient
	MojObject recipientArray, recipient;
	err = recipient.put(MOJDB_ADDRESS, toAddress);
	MojErrCheck(err);
	err = recipientArray.push(recipient);
	MojErrCheck(err);
	err = returnObj.put(MOJDB_TO, recipientArray);
	MojErrCheck(err);

	// Message kind
	err = returnObj.putString(MOJDB_FOLDER, folderStrings[folder]);
	MojErrCheck(err);
	err = returnObj.putString(MOJDB_STATUS, statusStrings[status]);
	MojErrCheck(err);

	err = returnObj.putString(MOJDB_MSG_TEXT, msgText);
	MojErrCheck(err);

	// username - since this is incoming message, the username is always in toAddress
	err = returnObj.putString(MOJDB_USERNAME, toAddress);
	MojErrCheck(err);

	// service name
	err = returnObj.putString(MOJDB_SERVICENAME, msgType);
	MojErrCheck(err);

	// localTimestamp
	err = returnObj.putInt(MOJDB_DEVICE_TIMESTAMP, deviceTimestamp);
	MojErrCheck(err);

	// incoming server
	err = returnObj.putInt(MOJDB_SERVER_TIMESTAMP, serverTimestamp);
	MojErrCheck(err);

	IMServiceHandler::privatelogIMMessage(_T("DB Message object %s:"), returnObj, MOJDB_MSG_TEXT);

	return err;
}

/*
 * Unformat the from address - removes blanks and converts to lower case
 *
 *  see IMAddressFormatter.unformat() - palm.com.messaging.data
 */
MojErr IMMessage::unformatFromAddress(const MojString formattedScreenName, MojString& unformattedName) {

	if(!formattedScreenName.empty()) {
		MojVector<MojString> stringTokens;
		MojErr err = formattedScreenName.split(' ', stringTokens);
		MojErrCheck(err);

		MojVector<MojString>::ConstIterator itr = stringTokens.begin();
		while (itr != stringTokens.end()) {
			err = unformattedName.append(*itr);
			MojErrCheck(err);
			itr++;
		}
		err = unformattedName.toLower();
		MojErrCheck(err);
	}
	return MojErrNone;
}

