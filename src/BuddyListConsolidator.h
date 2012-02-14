/*
 * BuddyListConsolidator.h
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
 * BuddyListConsolidator class synchronizes buddy lists between server and device.
 */

#ifndef _IM_BUDDYLISTCONSOLIDATOR_H_
#define _IM_BUDDYLISTCONSOLIDATOR_H_

#include <memory>
#include <map>
#include "core/MojService.h"
#include "db/MojDbServiceClient.h"

/**
 * This in an interface for helper classes used to create the add, merge, and delete lists
 */
class BuddyConsolidationHelperInterface {
public:
	virtual bool getUsername(const MojObject& contact, MojString& username) = 0;
	virtual bool hasChanges(MojObject& oldContact, MojObject& newBuddy, MojObject& diffs) = 0;
	virtual bool formatForDB(const MojString& accountId, const MojObject& buddy, MojObject& contact) = 0;
};

// Helps consolidate the com.palm.contact dbkind
class ContactConsolidationHelper : public BuddyConsolidationHelperInterface {
public:
	virtual bool getUsername(const MojObject& contact, MojString& username);
	virtual bool hasChanges(MojObject& oldContact, MojObject& newBuddy, MojObject& diffs);
	virtual bool formatForDB(const MojString& accountId, const MojObject& buddy, MojObject& contact);
};

// Helps consolidate the com.palm.imbuddystatus dbkind
class BuddyStatusConsolidationHelper : public BuddyConsolidationHelperInterface {
public:
	virtual bool getUsername(const MojObject& contact, MojString& username);
	virtual bool hasChanges(MojObject& oldContact, MojObject& newBuddy, MojObject& diffs);
	virtual bool formatForDB(const MojString& accountId, const MojObject& buddy, MojObject& contact);
};

/**
 * Consolidation logic to determine which bucket (add, update, delete) to put each buddy into.
 * This is used after retrieving a full list of buddies from the remote service.
 */
class BuddyListConsolidator : public MojSignalHandler {
public:
	BuddyListConsolidator(MojService* service, const MojString& accountId);
	virtual ~BuddyListConsolidator();

	bool isAllDataSet() { return (m_setFlags == (BuddyListConsolidator::CONTACTS_SET | BuddyListConsolidator::BUDDYSTATUS_SET | BuddyListConsolidator::NEW_BUDDYLIST_SET)); }

	void setContacts(MojObject& contacts);
	void setBuddyStatus(MojObject& buddies);
	void setNewBuddyList(MojObject& buddies);

	MojErr consolidateContacts();
	MojErr consolidateBuddyStatus();

private:
	typedef std::map<MojString, MojObject> BuddyMap;
	void getNewBuddyMap(BuddyMap& buddiesMap);
	MojErr consolidate(const MojObject& contacts, BuddyConsolidationHelperInterface& helper, MojObject& contactsToAdd, MojObject& contactsToDelete, MojObject& contactsToMerge);

	MojDbClient::Signal::Slot<BuddyListConsolidator> m_contactsAddSlot;
	MojErr contactsAddResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<BuddyListConsolidator> m_contactsDeleteSlot;
	MojErr contactsDeleteResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<BuddyListConsolidator> m_contactsMergeSlot;
	MojErr contactsMergeResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<BuddyListConsolidator> m_buddyStatusAddSlot;
	MojErr buddyStatusAddResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<BuddyListConsolidator> m_buddyStatusDeleteSlot;
	MojErr buddyStatusDeleteResult(MojObject& result, MojErr err);

	MojDbClient::Signal::Slot<BuddyListConsolidator> m_buddyStatusMergeSlot;
	MojErr buddyStatusMergeResult(MojObject& result, MojErr err);

	static const unsigned int CONTACTS_SET;
	static const unsigned int BUDDYSTATUS_SET;
	static const unsigned int NEW_BUDDYLIST_SET;
	unsigned int m_setFlags;

	const MojString m_accountId;
	MojObject m_contacts;
	MojObject m_buddyStatus;
	MojObject m_newBuddyList;

	MojDbServiceClient m_dbClient;
	MojDbServiceClient m_tempdbClient;
};

#endif /* _IM_BUDDYLISTCONSOLIDATOR_H_ */
