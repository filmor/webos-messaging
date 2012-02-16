/*
 * LibpurpleAdapterPrefs.h
 *
 */

#ifndef LIBPURPLEADAPTERPREFS_H_
#define LIBPURPLEADAPTERPREFS_H_

#include "core/MojService.h"
#include "db/MojDbServiceClient.h"
#include "LibpurpleAdapter.h"

class LibpurpleAdapterPrefs : public MojSignalHandler, public LibpurplePrefsHandler
{
public:	
	void init();
	LibpurpleAdapterPrefs(MojService* service);
	virtual ~LibpurpleAdapterPrefs();
	MojErr LoadAccountPreferences(const char* templateId, const char* UserName);
	
	MojErr GetServerPreferences(const char* templateId, const char* UserName, char*& ServerName, char*& ServerPort, bool& ServerTLS, bool& BadCert);
	char* GetStringPreference(const char* Preference, const char* templateId, const char* UserName);
	bool GetBoolPreference(const char* Preference, const char* templateId, const char* UserName);
	void setaccountprefs(MojString templateId, PurpleAccount* account);
private:
	MojDbClient::Signal::Slot<LibpurpleAdapterPrefs> m_findCommandSlot;
	MojErr findCommandResult(MojObject& result, MojErr err);

	// Pointer to the service used for creating/sending requests.
	MojService*	m_service;
	
	// Database client used to make any requests (put, watch, find, etc.) to Mojo DB
	MojDbServiceClient m_dbClient;
	MojDbServiceClientBatch* m_dbClientBatch;
};

#endif /* LIBPURPLEADAPTERPREFS_H_ */
