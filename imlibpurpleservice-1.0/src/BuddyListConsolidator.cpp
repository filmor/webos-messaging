/*
 * BuddyListConsolidator.cpp
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

#include "IMDefines.h"
#include "BuddyListConsolidator.h"
#include "IMServiceApp.h"

const unsigned int BuddyListConsolidator::CONTACTS_SET = 0x01;
const unsigned int BuddyListConsolidator::BUDDYSTATUS_SET = 0x02;
const unsigned int BuddyListConsolidator::NEW_BUDDYLIST_SET = 0x04;

BuddyListConsolidator::BuddyListConsolidator(MojService* service, const MojString& accountId)
: m_contactsAddSlot(this, &BuddyListConsolidator::contactsAddResult),
  m_contactsDeleteSlot(this, &BuddyListConsolidator::contactsDeleteResult),
  m_contactsMergeSlot(this, &BuddyListConsolidator::contactsMergeResult),
  m_buddyStatusAddSlot(this, &BuddyListConsolidator::buddyStatusAddResult),
  m_buddyStatusDeleteSlot(this, &BuddyListConsolidator::buddyStatusDeleteResult),
  m_buddyStatusMergeSlot(this, &BuddyListConsolidator::buddyStatusMergeResult),
  m_setFlags(0),
  m_accountId(accountId),
  m_dbClient(service, MojDbServiceDefs::ServiceName),
  m_tempdbClient(service, MojDbServiceDefs::TempServiceName)
{
}

BuddyListConsolidator::~BuddyListConsolidator()
{
	MojLogDebug(IMServiceApp::s_log, _T("BuddyListConsolidator destructing"));
}


void BuddyListConsolidator::setContacts(MojObject& contacts)
{
	m_setFlags |= BuddyListConsolidator::CONTACTS_SET;
	m_contacts = contacts;
}

void BuddyListConsolidator::setBuddyStatus(MojObject& buddies)
{
	m_setFlags |= BuddyListConsolidator::BUDDYSTATUS_SET;
	m_buddyStatus = buddies;
}

void BuddyListConsolidator::setNewBuddyList(MojObject& buddies)
{
	m_setFlags |= BuddyListConsolidator::NEW_BUDDYLIST_SET;
	m_newBuddyList = buddies;
}

// Copy the new buddies list into a map for quick lookup
void BuddyListConsolidator::getNewBuddyMap(BuddyMap& buddiesMap)
{
	MojObject::ConstArrayIterator buddyItr = m_newBuddyList.arrayBegin();
	MojObject buddyEntry;
	MojString username;
	MojErr err;
	while (buddyItr != m_newBuddyList.arrayEnd())
	{
		buddyEntry = *buddyItr;
		err = buddyEntry.getRequired("username", username);
		if (err == MojErrNone)
		{
			buddiesMap[username] = buddyEntry;
		}
		else
		{
			MojLogError(IMServiceApp::s_log, _T("setNewBuddyList received buddy with no username"));
		}
		buddyItr++;
	}
}

MojErr BuddyListConsolidator::consolidateContacts()
{
	MojObject contactsToAdd, contactIdsToDelete, contactsToMerge;
	ContactConsolidationHelper contactsHelper;
	MojErr err = consolidate(m_contacts, contactsHelper, contactsToAdd, contactIdsToDelete, contactsToMerge);
	if (err != MojErrNone)
	{
		MojLogError(IMServiceApp::s_log, _T("BuddyListConsolidator consolidate returned error=%d"), err);
		//TODO what to do here???
	}
	else
	{
		if (!contactsToAdd.empty())
		{
			m_dbClient.put(m_contactsAddSlot, contactsToAdd.arrayBegin(), contactsToAdd.arrayEnd());
		}

		if (!contactIdsToDelete.empty())
		{
			m_dbClient.del(m_contactsDeleteSlot, contactIdsToDelete.arrayBegin(), contactIdsToDelete.arrayEnd());
		}

		if (!contactsToMerge.empty())
		{
			m_dbClient.merge(m_contactsMergeSlot, contactsToMerge.arrayBegin(), contactsToMerge.arrayEnd());
		}
	}

	return err;
}

MojErr BuddyListConsolidator::consolidateBuddyStatus()
{
	MojObject buddiesToAdd, buddyIdsToDelete, buddiesToMerge;
	BuddyStatusConsolidationHelper buddyStatusHelper;
	MojErr err = consolidate(m_buddyStatus, buddyStatusHelper, buddiesToAdd, buddyIdsToDelete, buddiesToMerge);
	if (err != MojErrNone)
	{
		MojLogError(IMServiceApp::s_log, _T("BuddyListConsolidator consolidate returned error=%d"), err);
		//TODO what to do here???
	}
	else
	{
		if (!buddiesToAdd.empty())
		{
			m_tempdbClient.put(m_buddyStatusAddSlot, buddiesToAdd.arrayBegin(), buddiesToAdd.arrayEnd());
		}

		if (!buddyIdsToDelete.empty())
		{
			m_tempdbClient.del(m_buddyStatusDeleteSlot, buddyIdsToDelete.arrayBegin(), buddyIdsToDelete.arrayEnd());
		}

		if (!buddiesToMerge.empty())
		{
			m_tempdbClient.merge(m_buddyStatusMergeSlot, buddiesToMerge.arrayBegin(), buddiesToMerge.arrayEnd());
		}
	}

	return err;
}

/*
 * This consolidates the list of buddies received from the remote service with the list of buddies in the
 * contacts db. The algo is basically
 *	for each buddy in old_buddy_list
 *	  if buddy !exists in new_table
 *		add record ID to the delete list
 *	  if buddy exists in new_table
 *		remove from new_table
 *		if buddy has changes
 *		  add diffs to the merge list
 *	end for
 *	items remaining in the new_table need to be added to the contacts db.
 */
MojErr BuddyListConsolidator::consolidate(const MojObject& contacts, BuddyConsolidationHelperInterface& helper, MojObject& contactsToAdd, MojObject& contactsToDelete, MojObject& contactsToMerge)
{
	MojObject oldContact, newBuddy, contactDiff;
	MojString username, recordId;
	MojErr err;
	BuddyMap newBuddyList;
	getNewBuddyMap(newBuddyList);
	MojObject::ConstArrayIterator oldContactItr = contacts.arrayBegin();
	while (oldContactItr != contacts.arrayEnd())
	{
		oldContact = *oldContactItr;
		if (!helper.getUsername(oldContact, username))
		{
			MojLogError(IMServiceApp::s_log, _T("consolidate found a buddy with no username. it will be skipped."));
		}
		else
		{
			newBuddy = newBuddyList[username];
			// If newBuddy is empty, then the buddy no longer exist so add its record ID to the delete list
			if (newBuddy.empty())
			{
				err = oldContact.getRequired("_id", recordId);
				if (err != MojErrNone)
				{
					MojLogError(IMServiceApp::s_log, _T("can't add to delete list because _id is missing. err=%d"), err);
				}
				else
				{
					MojLogDebug(IMServiceApp::s_log, _T("adding buddy=%s to delete list"), username.data());
					contactsToDelete.push(recordId);
				}
			}
			else
			{
				// Remove from the new buddy's table so the newBuddyList will eventually only contain
				// buddies to be added to contacts
				newBuddyList.erase(username);
				// If there are changes, add the changes to the merge list.
				contactDiff.clear(); //TODO: validate this clears old properties from the object
				if (helper.hasChanges(oldContact, newBuddy, contactDiff))
				{
					// Note: this assumes the buddyDiff includes the _id
					MojLogDebug(IMServiceApp::s_log, _T("adding buddy=%s to merge list"), username.data());
					contactsToMerge.push(contactDiff);
				}
			}
		}
		oldContactItr++;
	}

	// At this point, contactsToDelete and contactsToMerge are ready. Now copy any buddies that
	// are still in m_newBuddyList to contactsToAdd.
	BuddyMap::iterator newBuddyItr = newBuddyList.begin();
	while (newBuddyItr != newBuddyList.end())
	{
		newBuddy.assign(newBuddyItr->second);
		MojObject newContact;
		if (helper.formatForDB(m_accountId, newBuddy, newContact))
		{
			contactsToAdd.push(newContact);
		}
		newBuddyItr++;
	}

	return MojErrNone;
}


MojErr BuddyListConsolidator::contactsAddResult(MojObject& payload, MojErr resultErr)
{
	//TODO probably just log errors since there's no easy way to retry adding the contacts
	return MojErrNone;
}


MojErr BuddyListConsolidator::contactsDeleteResult(MojObject& payload, MojErr resultErr)
{
	//TODO probably just log errors since there's no easy way to retry deleting the contacts
	return MojErrNone;
}


MojErr BuddyListConsolidator::contactsMergeResult(MojObject& payload, MojErr resultErr)
{
	//TODO probably just log errors since there's no easy way to retry merging the contacts
	return MojErrNone;
}


MojErr BuddyListConsolidator::buddyStatusAddResult(MojObject& payload, MojErr resultErr)
{
	//TODO probably just log errors since there's no easy way to retry adding the buddystatus
	return MojErrNone;
}


MojErr BuddyListConsolidator::buddyStatusDeleteResult(MojObject& payload, MojErr resultErr)
{
	//TODO probably just log errors since there's no easy way to retry deleting the buddystatus
	return MojErrNone;
}


MojErr BuddyListConsolidator::buddyStatusMergeResult(MojObject& payload, MojErr resultErr)
{
	//TODO probably just log errors since there's no easy way to retry merging the buddystatus
	return MojErrNone;
}


///////////////////////////////////////////////////////////////////////////////////
bool ContactConsolidationHelper::getUsername(const MojObject& contact, MojString& username)
{
	bool found = false;
	MojObject imsArray, imObj;
	MojErr err = contact.getRequired("ims", imsArray);
	if (err != MojErrNone)
	{
		MojLogError(IMServiceApp::s_log, _T("getUsernameFromContact found a contact with no ims property."));
	}
	else
	{
		if (imsArray.at(0, imObj))
		{
			err = imObj.getRequired("value", username);
			found = (err == MojErrNone);
		}
	}
	return found;
}


// Compares the avatar path and displayName for contacts. If there are diffs, the changed properties
// are added to the diffs object along with the _id property so it can be used with db.merge
//TODO: compare avatar, displayName, status, availability, and group for buddy status.
bool ContactConsolidationHelper::hasChanges(MojObject& oldContact, MojObject& newBuddy, MojObject& diffs)
{
	bool hasChanges = false;
	bool oldFound = false, newFound = false;
	MojString oldDisplayName, newAvatar, newDisplayName;
	oldContact.get("nickname", oldDisplayName, oldFound);
	newBuddy.get("displayName", newDisplayName, newFound);
	if (oldDisplayName != newDisplayName)
	{
		if (oldDisplayName.length() > 1 && newDisplayName.empty())
		{
			// Needs to contain at least empty string to overwrite the existing display name
			newDisplayName.assign("");
		}
		hasChanges = true;
		diffs.put("displayName", newDisplayName);
	}

	// Buddy avatar is stored in the Contact.photo object
	newBuddy.get("avatar", newAvatar, newFound);
	MojObject oldPhotos, firstPhoto;
	oldContact.get("photos", oldPhotos);
	// If the old contact has no photo and the new contact does, then make an array and add it
	if (oldPhotos.empty())
	{
		if (newAvatar.length() > 1)
		{
			hasChanges = true;
			firstPhoto.put("localPath", newAvatar);
			firstPhoto.put("value", newAvatar);
			firstPhoto.putString("type", "type_square"); // AIM and GTalk generally send small, square-ish images
			MojObject newPhotos;
			newPhotos.push(firstPhoto);
			diffs.put("photos", newPhotos);
		}
	}
	else
	{
		MojObject::ArrayIterator photosItr;
		oldPhotos.arrayBegin(photosItr);
		firstPhoto = *photosItr;
		MojString oldAvatar;
		firstPhoto.get("localPath", oldAvatar, oldFound);
		if (oldAvatar != newAvatar)
		{
			hasChanges = true;
			MojObject newPhotos;
			if (oldAvatar.length() > 1 && newAvatar.empty())
			{
				// To remove the photo, set the photos array to contain an empty object
				// TODO: verify with contacts guys
				MojObject emptyObject;
				newPhotos.push(emptyObject);
			}
			else
			{
				firstPhoto.put("localPath", newAvatar);
				firstPhoto.put("value", newAvatar);
				firstPhoto.putString("type", "type_square"); // AIM and GTalk generally send small, square-ish images
				newPhotos.push(firstPhoto);
			}
			diffs.put("photos", newPhotos);
		}
	}

	if (hasChanges == true)
	{
		MojString recordId;
		MojErr err = oldContact.getRequired("_id", recordId);
		if (err != MojErrNone)
		{
			MojLogError(IMServiceApp::s_log, _T("oldContact has no id. Should never happen."));
		}
		diffs.put("_id", recordId);
	}

	return hasChanges;
}


// Coerce the IM buddy into com.palm.contact format
// {
//    accountId:<>,
//    displayName:<>,
//    photos:[{localPath:<>, value:<>}],
//    ims:[{value:<>, type:<>}]
// }
bool ContactConsolidationHelper::formatForDB(const MojString& accountId, const MojObject& buddy, MojObject& contact)
{
	bool valid = false;
	bool found;
	MojString username, serviceName, nickname;
	// Validate that this buddy has a username, if not it is invalid and won't be added to contacts
	MojErr err = buddy.getRequired("username", username);
	if (err != MojErrNone)
	{
		MojLogError(IMServiceApp::s_log, _T("This new contact has no buddy name. it will be skipped."));
	}
	else
	{
		err = buddy.getRequired("serviceName", serviceName);
		if (err != MojErrNone)
		{
			MojLogError(IMServiceApp::s_log, _T("This new contact has no serviceName. it will be skipped."));
		}
		else
		{
			MojObject newImsArray, newImObj;
			contact.putString("_kind", IM_CONTACT_KIND);
			contact.put("accountId", accountId);
			contact.put("remoteId", username); // using username as remote ID since we don't have anything else and this should be unique

			// If the buddy has a displayName, put it in the contact's nickname property
			if (MojErrNone == buddy.getRequired("displayName", nickname))
	        {
				contact.put("nickname", nickname);
	        }

			// "ims" array
			newImObj.put("value", username);
			newImObj.put("type", serviceName);
			//newImObj.put("serviceName", serviceName);
			newImsArray.push(newImObj);
			contact.put("ims", newImsArray);

			// need to add email address too for contacts linker
			//    email:[{value:<>}]
			MojString emailAddr;
			bool validEmail = true;
			emailAddr.assign(username);
			// for AIM, need to add back the "@aol.com" to the email
			if (MojInvalidIndex == username.find('@')) {
				if (0 == serviceName.compare(SERVICENAME_AIM))
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


			// "photos" array
			MojString avatar;
			buddy.get("avatar", avatar, found);
			if (found && !avatar.empty())
			{
				MojObject newPhotosArray, newPhotoObj;
				newPhotoObj.put("localPath", avatar);
				newPhotoObj.put("value", avatar);
				newPhotoObj.putString("type", "type_square"); // AIM and GTalk generally send small, square-ish images
				newPhotosArray.push(newPhotoObj);
				contact.put("photos", newPhotosArray);
			}

			valid = true;
			MojLogDebug(IMServiceApp::s_log, _T("contact with username=%s is valid"), username.data());
		}
	}

	return valid;
}


///////////////////////////////////////////////////////////////////////////////////
bool BuddyStatusConsolidationHelper::getUsername(const MojObject& buddy, MojString& username)
{
	MojErr err = buddy.getRequired("username", username);
	if (err != MojErrNone)
	{
		MojLogError(IMServiceApp::s_log, _T("BuddyStatusConsolidationHelper::getUsername found a user with no username."));
	}
	return (err == MojErrNone);
}


// Compares the availability, group, and status. If there are diffs, the changed properties
// are added to the diffs object along with the _id property so it can be used with db.merge
bool BuddyStatusConsolidationHelper::hasChanges(MojObject& oldBuddy, MojObject& newBuddy, MojObject& diffs)
{
	bool hasChanges = false;

	bool oldFound = false, newFound = false;
	MojInt32 oldAvailability, newAvailability;
	oldBuddy.get("availability", oldAvailability, oldFound);
	newBuddy.get("availability", newAvailability, newFound);
	if (oldAvailability != newAvailability && newFound)
	{
		hasChanges = true;
		diffs.put("availability", newAvailability);
	}

	MojString oldGroup, newGroup;
	oldBuddy.get("group", oldGroup, oldFound);
	newBuddy.get("group", newGroup, newFound);
	if (oldGroup != newGroup)
	{
		hasChanges = true;
		if (oldGroup.length() > 1 && newGroup.empty())
		{
			// Just want to erase the property
			diffs.putString("group", "");
		}
		else
		{
			diffs.put("group", newGroup);
		}
	}

	MojString oldStatus, newStatus;
	oldBuddy.get("status", oldStatus, oldFound);
	newBuddy.get("status", newStatus, newFound);
	if (oldStatus != newStatus)
	{
		hasChanges = true;
		if (oldStatus.length() > 1 && newStatus.empty())
		{
			// Just want to erase the property
			diffs.putString("status", "");
		}
		else
		{
			diffs.put("status", newStatus);
		}
	}

	if (hasChanges == true)
	{
		MojString recordId;
		MojErr err = oldBuddy.getRequired("_id", recordId);
		if (err != MojErrNone)
		{
			MojLogError(IMServiceApp::s_log, _T("oldBuddy has no id. Should never happen."));
		}
		diffs.put("_id", recordId);
	}

	return hasChanges;
}


// Coerce the IM buddy into com.palm.imbuddystatus format
// {
//    accountId:<>,
//    username:<>,
//    serviceName:<>,
//    availability:<>,
//    group:<>,
//    status:<>,
// }
bool BuddyStatusConsolidationHelper::formatForDB(const MojString& accountId, const MojObject& buddy, MojObject& buddyStatus)
{
	bool valid = false;
	bool found;
	MojString username, serviceName, group, status;
	MojInt32 availability;
	// Validate that this buddy has a username, if not it is invalid and won't be added to contacts
	MojErr err = buddy.getRequired("username", username);
	if (err != MojErrNone)
	{
		MojLogError(IMServiceApp::s_log, _T("This new buddy has no buddy name. it will be skipped."));
	}
	else
	{
		err = buddy.getRequired("serviceName", serviceName);
		if (err != MojErrNone)
		{
			MojLogError(IMServiceApp::s_log, _T("This new buddy has no serviceName. it will be skipped."));
		}
		else
		{
			valid = true;
			buddyStatus.putString("_kind", IM_BUDDYSTATUS_KIND);
			buddyStatus.put("accountId", accountId);
			buddyStatus.put("username", username);
			buddyStatus.put("serviceName", serviceName);
			buddy.get("availability", availability, found);
			if (!found)
			{
				buddyStatus.put("availability", (MojInt32)PalmAvailability::OFFLINE);
			}
			else
			{
				buddyStatus.put("availability", availability);
			}

			buddy.get("group", group, found);
			if (!found)
			{
				buddyStatus.putString("group", "");
			}
			else
			{
				buddyStatus.put("group", group);
			}

			buddy.get("status", status, found);
			if (!found)
			{
				buddyStatus.putString("status", "");
			}
			else
			{
				buddyStatus.put("status", status);
			}
		}
	}

	return valid;
}


