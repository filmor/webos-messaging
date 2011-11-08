/* ============================================================
 * Date  : Jan 21, 2010
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBKINDSTATE_H_
#define MOJDBKINDSTATE_H_

#include "core/MojSharedTokenSet.h"
#include "core/MojSet.h"
#include "db/MojDbKindEngine.h"

class MojDbKindState : public MojSharedTokenSet
{
public:
	static const MojChar* const IndexIdsKey;
	static const MojChar* const KindTokensKey;
	static const MojChar* const TokensKey;

	typedef MojSet<MojString> StringSet;

	MojDbKindState(const MojString& kindId, MojDbKindEngine* kindEngine);
	virtual ~MojDbKindState() {}

	MojErr init(const StringSet& strings, MojDbReq& req);
	MojErr indexId(const MojChar* indexName, MojDbReq& req, MojObject& idOut, bool& createdOut);
	MojErr delIndex(const MojChar* indexName, MojDbReq& req);

	MojInt64 token() const { return m_kindToken; }
	virtual MojErr tokenSet(TokenVec& vecOut, MojObject& tokensObjOut) const;
	virtual MojErr addToken(const MojChar* propName, MojUInt8& tokenOut, TokenVec& vecOut, MojObject& tokenObjOut);

private:
	MojErr addPropImpl(const MojChar* propName, bool write, MojUInt8& tokenOut, TokenVec& vecOut, MojObject& tokenObjOut);
	MojErr initKindToken(MojDbReq& req);
	MojErr initTokens(MojDbReq& req, const StringSet& strings);
	MojErr id(const MojChar* name, const MojChar* objKey, MojDbReq& req, MojObject& idOut, bool& createdOut);
	MojErr readIds(const MojChar* key, MojDbReq& req, MojObject& objOut, MojRefCountedPtr<MojDbStorageItem>& itemOut);
	MojErr writeIds(const MojChar* key, const MojObject& obj, MojDbReq& req, MojRefCountedPtr<MojDbStorageItem>& oldItem);
	MojErr writeTokens(const MojObject& tokensObj);
	MojErr writeObj(const MojChar* key, const MojObject& val, MojDbStorageDatabase* db,
			MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageItem>& oldItem);
	MojErr readObj(const MojChar* key, MojObject& val, MojDbStorageDatabase* db,
			MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageItem>& oldItem);

	mutable MojThreadMutex m_lock;
	MojRefCountedPtr<MojDbStorageItem> m_oldTokensItem;
	MojString m_kindId;
	MojObject m_tokensObj;
	TokenVec m_tokenVec;
	MojInt64 m_kindToken;
	MojUInt8 m_nextToken;
	MojDbKindEngine* m_kindEngine;
};

#endif /* MOJDBKINDSTATE_H_ */
