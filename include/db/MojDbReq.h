/* ============================================================
 * Date  : Oct 19, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBREQ_H_
#define MOJDBREQ_H_

#include "db/MojDbDefs.h"
#include "core/MojString.h"

struct MojDbReqRef
{
	explicit MojDbReqRef(MojDbReq& req) : m_req(req) {}
	MojDbReq* operator->() { return &m_req; }
	operator MojDbReq&() { return m_req; }
	
	MojDbReq& m_req;
};

class MojDbReq : private MojNoCopy
{
public:
	MojDbReq(bool admin = true);
	~MojDbReq();

	MojErr domain(const MojChar* domain);
	void admin(bool privilege) { m_admin = privilege; }
	void txn(MojDbStorageTxn* txn) { m_txn = txn; }

	void beginBatch();
	MojErr endBatch();

	MojErr begin(MojDb* db, bool lockSchema);
	MojErr end(bool commitNow = true);
	MojErr abort();
	MojErr curKind(const MojDbKind* kind);

	const MojString& domain() const { return m_domain; }
	bool admin() const { return m_admin; }
	bool batch() const { return m_batch; }
	MojDbStorageTxn* txn() const { return m_txn.get(); }

	operator MojDbReqRef() { return MojDbReqRef(*this); }

private:
	void lock(MojDb* db, bool lockSchema);
	void unlock();
	MojErr commit();

	MojString m_domain;
	MojRefCountedPtr<MojDbStorageTxn> m_txn;
	MojUInt32 m_beginCount;
	MojDb* m_db;
	bool m_admin;
	bool m_batch;
	bool m_schemaLocked;
};

class MojDbAdminGuard : private MojNoCopy
{
public:
	MojDbAdminGuard(MojDbReq& req, bool setPrivilege = true);
	~MojDbAdminGuard() { unset(); }

	void set() { m_req.admin(true); }
	void unset() { m_req.admin(m_oldPrivilege); }

private:
	MojDbReq& m_req;
	bool m_oldPrivilege;
};

#endif /* MOJDBREQ_H_ */
