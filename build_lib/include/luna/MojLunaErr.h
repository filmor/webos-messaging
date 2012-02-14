/* ============================================================
 * Date  : Apr 17, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJLUNAERR_H_
#define MOJLUNAERR_H_

#include "luna/MojLunaDefs.h"
#include "core/MojNoCopy.h"

class MojLunaErr : private MojNoCopy
{
public:
	MojLunaErr() { LSErrorInit(&m_err); }
	~MojLunaErr() { LSErrorFree(&m_err); }

	MojErr toMojErr() const;
	const MojChar* message() const { return m_err.message; }
	const MojChar* file() const { return m_err.file; }
	int line() const { return m_err.line; }

	operator LSError*() { return &m_err; }

private:
	mutable LSError m_err;
};

#define MojLsErrCheck(RETVAL, LSERR) if (!(RETVAL)) MojErrThrowMsg((LSERR).toMojErr(), _T("luna-service: %s (%s:%d)"), (LSERR).message(), (LSERR).file(), (LSERR).line())
#define MojLsErrAccumulate(EACC, RETVAL, LSERR) if (!(RETVAL)) EACC = (LSERR).toMojErr()

#endif /* MOJLUNAERR_H_ */
