/* ============================================================
 * Date  : Apr 15, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJLUNAMESSAGE_H_
#define MOJLUNAMESSAGE_H_

#include "luna/MojLunaDefs.h"
#include "luna/MojLunaService.h"
#include "core/MojJson.h"
#include "core/MojServiceMessage.h"

class MojLunaMessage : public MojServiceMessage
{
public:
	MojLunaMessage(MojService* service);
	MojLunaMessage(MojService* service, LSMessage* msg, MojService::Category* category = NULL, bool response = false);
	~MojLunaMessage();

	virtual MojObjectVisitor& writer() { return m_writer; }
	virtual bool hasData() const { return (!m_writer.json().empty()); }
	virtual const MojChar* appId() const { return LSMessageGetApplicationID(m_msg); }
	virtual const MojChar* category() const { return LSMessageGetCategory(m_msg); }
	virtual const MojChar* method() const { return LSMessageGetMethod(m_msg); }
	virtual const MojChar* senderId() const { return LSMessageGetSenderServiceName(m_msg); }
	virtual const MojChar* senderAddress() const { return LSMessageGetSender(m_msg); }
	virtual const MojChar* queue() const;
	virtual Token token() const;
	virtual MojErr payload(MojObjectVisitor& visitor) const;
	virtual MojErr payload(MojObject& objOut) const;
	const MojChar* payload() const { return LSMessageGetPayload(m_msg); }

	LSMessage* impl() { return m_msg; }
	void reset(LSMessage* msg = NULL);
	bool isPublic() const { return LSMessageIsPublic(reinterpret_cast<MojLunaService*>(m_service)->m_service, m_msg); }

private:
	friend class MojLunaService;

	virtual MojErr replyImpl();
	MojErr processSubscriptions();
	void releaseMessage();

	MojLunaService* lunaService() { return reinterpret_cast<MojLunaService*>(m_service); }
	void token(Token tok) { m_token = tok; }

	mutable MojObject m_payload;
	LSMessage* m_msg;
	bool m_response;
	Token m_token;
	MojJsonWriter m_writer;
};

#endif /* MOJLUNAMESSAGE_H_ */
