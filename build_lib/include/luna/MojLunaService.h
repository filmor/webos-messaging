/* ============================================================
 * Date  : Apr 14, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJLUNASERVICE_H_
#define MOJLUNASERVICE_H_

#include "luna/MojLunaDefs.h"
#include "core/MojService.h"
#include "core/MojString.h"
#include "glib.h"

class MojLunaService : public MojService
{
public:
	MojLunaService(bool allowPublicMethods = false, MojMessageDispatcher* queue = NULL);
	virtual ~MojLunaService();

	virtual MojErr open(const MojChar* serviceName);
	virtual MojErr close();
	virtual MojErr dispatch();
	virtual MojErr addCategory(const MojChar* name, CategoryHandler* handler);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, bool onPublic);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, bool onPublic, const MojString& proxyRequester);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, bool onPublic, const char *proxyRequester);
	MojErr attach(GMainLoop* loop);

	LSPalmService* getService();

private:
	friend class MojLunaMessage;
	typedef MojVector<LSMethod> MethodVec;

	class LunaCategory : public Category
	{
	public:
		LunaCategory(MojService* service, CategoryHandler* handler, const MethodVec& pubMethods, const MethodVec& privMethods)
		: Category(service, handler), m_pubMethods(pubMethods), m_privMethods(privMethods) {}

		MethodVec m_pubMethods;
		MethodVec m_privMethods;
	};

	static const MojChar* const UriScheme;

	virtual MojErr sendImpl(MojServiceRequest* req, const MojChar* service, const MojChar* method, Token& tokenOut);
	virtual MojErr cancelImpl(MojServiceRequest* req);
	virtual MojErr dispatchReplyImpl(MojServiceRequest* req, MojServiceMessage *msg, MojObject& payload, MojErr errCode);
	virtual MojErr enableSubscriptionImpl(MojServiceMessage* msg);
	virtual MojErr removeSubscriptionImpl(MojServiceMessage* msg);

	static bool handleCancel(LSHandle* sh, LSMessage* msg, void* ctx);
	static bool handleRequest(LSHandle* sh, LSMessage* msg, void* ctx);
	static bool handleResponse(LSHandle* sh, LSMessage* msg, void* ctx);

	LSHandle* getHandle(bool onPublic = false);

	bool m_allowPublicMethods;
	LSPalmService* m_service;
	LSHandle* m_handle;
	GMainLoop* m_loop;
};

#endif /* MOJLUNASERVICE_H_ */
