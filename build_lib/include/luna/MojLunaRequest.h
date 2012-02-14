/* ============================================================
 * Date  : Nov 2, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJLUNAREQUEST_H_
#define MOJLUNAREQUEST_H_

#include "MojLunaDefs.h"
#include "core/MojJson.h"
#include "core/MojServiceRequest.h"

class MojLunaRequest : public MojServiceRequest
{
public:
	MojLunaRequest(MojService* service);
	MojLunaRequest(MojService* service, bool onPublic);
	MojLunaRequest(MojService* service, bool onPublic, const MojString&
		requester);

	bool onPublic() const { return m_onPublic; }
	bool isProxyRequest() const { return !m_requester.empty(); }
	bool cancelled() const { return m_cancelled; }
	const MojChar* payload() const { return m_writer.json(); }
	const MojChar* getRequester() const { return m_requester.data(); }
	virtual MojObjectVisitor& writer() { return m_writer; }

private:
	friend class MojLunaService;

	void cancelled(bool val) { m_cancelled = val; }

	MojJsonWriter m_writer;

	MojString m_requester;

	bool m_onPublic;
	bool m_cancelled;
};

#endif /* MOJLUNAREQUEST_H_ */
