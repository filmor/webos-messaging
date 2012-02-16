/* ============================================================
 * Date  : Jul 8, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJSOCKETMESSAGE_H_
#define MOJSOCKETMESSAGE_H_

#include "core/MojServiceMessage.h"
#include "core/MojObject.h"

class MojSocketService;

class MojSocketMessage : public MojServiceMessage
{
public:
	MojSocketMessage();
	~MojSocketMessage();


	virtual const MojChar* category() const;
	virtual const MojChar* method() const;
	virtual const MojChar* payload() const;

	virtual bool isResponse() const;
	virtual MojErr messageToken(Token& tokenOut) const;
	virtual MojErr senderToken(Token& tokenOut) const;
	static MojErr senderToken(MojSockT socket, Token& tokenOut);

	MojErr init(const MojString& category, const MojString& method, const MojChar* jsonPayload, const Token& tok);
	void setMethod(const MojString& method) { m_method = method; }
	void setToken(const Token& token) { m_token = token; }

	MojErr extract(const MojByte* data, MojSize len, MojSize& dataConsumed);
	MojSize serializedSize() const;
	MojErr serialize(MojDataWriter& writer) const;

	void setReplyInfo(MojSockT sock, MojSocketService* service);
	MojSockT getSocket() const { return m_socket; }

protected:
	virtual MojErr sendResponse(const MojChar* payload);

private:
	enum {
		None = 0,
		Response = 1
	};
	typedef MojUInt32 Flags;

	MojErr extractString(MojDataReader& reader, MojString& strOut);
	MojErr serializeString(MojDataWriter& writer, const MojString& str) const;

	MojString m_category;
	MojString m_method;
	MojString m_payloadJson;
	Token m_token;
	Flags m_flags;

	MojSockT m_socket;
	MojSocketService* m_service;
};

#endif /* MOJSOCKETMESSAGE_H_ */
