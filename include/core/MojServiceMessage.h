/* ============================================================
 * Date  : July 7, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJSERVICEMESSAGE_H_
#define MOJSERVICEMESSAGE_H_

#include "core/MojCoreDefs.h"
#include "core/MojMessage.h"
#include "core/MojService.h"

class MojServiceMessage : public MojMessage
{
public:
	typedef MojUInt32 Token;
	typedef MojSignal<MojServiceMessage*> CancelSignal;

	static const MojChar* const ReturnValueKey;
	static const MojChar* const ErrorCodeKey;
	static const MojChar* const ErrorTextKey;

	virtual ~MojServiceMessage();

	MojSize numReplies() const { return m_numReplies; }
	MojService::Category* serviceCategory() const { return m_category; }
	virtual MojObjectVisitor& writer() = 0;
	virtual const MojChar* appId() const = 0;
	virtual const MojChar* category() const = 0;
	virtual const MojChar* method() const = 0;
	virtual const MojChar* senderAddress() const = 0;
	virtual const MojChar* senderId() const= 0;
	virtual const MojChar* senderName() const;
	virtual MojErr payload(MojObjectVisitor& visitor) const = 0;
	virtual MojErr payload(MojObject& objOut) const = 0;
	virtual Token token() const = 0;
	virtual bool hasData() const = 0;

	void notifyCancel(CancelSignal::SlotRef cancelHandler);

	MojErr reply();
	MojErr reply(const MojObject& payload);
	MojErr replySuccess();
	MojErr replySuccess(MojObject& payload);
	MojErr replyError(MojErr code);
	MojErr replyError(MojErr code, const MojChar* text);

protected:
	friend class MojService;

	MojServiceMessage(MojService* service, MojService::Category* category);
	MojErr close();
	MojErr dispatchCancel() { return m_cancelSignal.fire(this); }
	MojErr enableSubscription();
	virtual MojErr handleCancel();
	virtual MojErr replyImpl() = 0;
	virtual MojErr dispatch();
	void dispatchMethod(MojService::DispatchMethod method) { m_dispatchMethod = method; }

	MojSize m_numReplies;
	CancelSignal m_cancelSignal;
	MojService* m_service;
	MojService::Category* m_category;
	MojService::DispatchMethod m_dispatchMethod;
	bool m_subscribed;
};

#endif /* MOJSERVICEMESSAGE_H_ */
