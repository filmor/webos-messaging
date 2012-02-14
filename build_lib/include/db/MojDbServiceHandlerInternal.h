/* ============================================================
 * Date  : Apr 29, 2010
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBSERVICEHANDLERINTERNAL_H_
#define MOJDBSERVICEHANDLERINTERNAL_H_

#include "db/MojDbDefs.h"
#include "db/MojDb.h"
#include "db/MojDbServiceHandlerBase.h"
#include "core/MojService.h"
#include "core/MojServiceMessage.h"
#include "core/MojServiceRequest.h"
#include "luna/MojLunaService.h"

class MojDbServiceHandlerInternal : public MojDbServiceHandlerBase
{
public:
	// method and service names
	static const MojChar* const PostBackupMethod;
	static const MojChar* const PostRestoreMethod;
	static const MojChar* const PreBackupMethod;
	static const MojChar* const PreRestoreMethod;
	static const MojChar* const ScheduledPurgeMethod;

	MojDbServiceHandlerInternal(MojDb& db, MojReactor& reactor, MojService& service);

	virtual MojErr open();
	virtual MojErr close();

	MojErr configure(bool fatalError);

private:
	class PurgeHandler : public MojSignalHandler
	{
	public:
		PurgeHandler(MojDbServiceHandlerInternal* serviceHandler, const MojObject& activityId);
		MojErr init();

	private:
		MojErr handleAdopt(MojObject& payload, MojErr errCode);
		MojErr handleComplete(MojObject& payload, MojErr errCode);
		MojErr initPayload(MojObject& payload);

		bool m_handled;
		MojObject m_activityId;
		MojDbServiceHandlerInternal* m_serviceHandler;
		MojRefCountedPtr<MojServiceRequest> m_subscription;
		MojServiceRequest::ReplySignal::Slot<PurgeHandler> m_adoptSlot;
		MojServiceRequest::ReplySignal::Slot<PurgeHandler> m_completeSlot;
	};

	class LocaleHandler : public MojSignalHandler
	{
	public:
		LocaleHandler(MojDb& db);
		MojErr handleResponse(MojObject& payload, MojErr errCode);

		MojDb& m_db;
		MojServiceRequest::ReplySignal::Slot<LocaleHandler> m_slot;
	};

	class AlertHandler : public MojSignalHandler
	{
	public:
		AlertHandler(MojService& service, const MojObject& payload);
		MojErr send();
		MojErr handleBootStatusResponse(MojObject& payload, MojErr err);
		MojErr handleAlertResponse(MojObject& payload, MojErr errCode);

		MojServiceRequest::ReplySignal::Slot<AlertHandler> m_bootStatusSlot;
		MojServiceRequest::ReplySignal::Slot<AlertHandler> m_alertSlot;
		MojRefCountedPtr<MojServiceRequest> m_subscription;
		MojService& m_service;
		MojObject m_payload;
	};

	MojErr handlePostBackup(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePostRestore(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePreBackup(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handlePreRestore(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr handleScheduledPurge(MojServiceMessage* msg, MojObject& payload, MojDbReq& req);

	typedef enum { NoSpaceAlert = -1, AlertLevelLow, AlertLevelMedium,
		AlertLevelHigh } AlertLevel;

	MojErr requestLocale();
	void initSpaceCheck();
	void stopSpaceCheck();
	MojErr handleSpaceCheck();
	static gboolean periodicSpaceCheck(gpointer data);
	MojErr generateSpaceAlert(AlertLevel level, MojInt64 bytesUsed, MojInt64 bytesAvailable);
	MojErr generateFatalAlert();
	MojErr generateAlert(const MojChar* event, const MojObject& msg);

	GSource* m_spaceCheckTimer;
	AlertLevel m_spaceAlertLevel;

	MojService& m_service;
	MojRefCountedPtr<LocaleHandler> m_localeChangeHandler;
	static const Method s_privMethods[];

	// Some of these need to be changed to configuration items
	static const MojInt32 SpaceCheckInterval;
	static const MojChar* const DatabaseRoot;
	static const MojDouble SpaceAlertLevels[];
	static const MojChar* const SpaceAlertNames[];
	static const MojInt32 NumSpaceAlertLevels;
};

#endif /* MOJDBSERVICEHANDLERINTERNAL_H_ */
