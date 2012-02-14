/* ============================================================
 * Date  : Mar 13, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJTESTRUNNER_H_
#define MOJTESTRUNNER_H_

#include "core/MojCoreDefs.h"
#include "core/MojApp.h"
#include "core/MojThread.h"

#define MojTestAssert(COND)	do if (!(COND)) {MojTestRunner::instance()->testFailed(_T(#COND), _T(__FILE__), __LINE__); MojErrThrow(MojErrTestFailed);} while(0)
#define MojTestErrCheck(VAR) do if ((VAR) != MojErrNone) {MojTestRunner::instance()->testError((VAR), _T(__FILE__), __LINE__); MojErrThrow(VAR);} while(0)
#define MojTestErrExpected(VAR, VAL) do {if ((VAR) == (VAL)) VAR = MojErrNone; else MojTestAssert((VAR) == (VAL));} while(0)

struct MojTestCaseRef
{
	MojTestCaseRef(MojTestCase& tc) : m_case(tc) {}
	MojTestCase& m_case;
};

class MojTestCase
{
public:
    virtual ~MojTestCase() {}
	virtual MojErr run() = 0;
	virtual void failed() {}
	virtual void cleanup() {}
	const MojChar* name() const { return m_name; }

	operator MojTestCaseRef() { return MojTestCaseRef(*this); }

protected:
	friend class MojTestRunner;
	MojTestCase(const MojChar* name) : m_name(name), m_failed(false) {}

	const MojChar* m_name;
	bool m_failed;
};

class MojTestRunner : public MojApp
{
public:
	MojTestRunner();
	virtual ~MojTestRunner();

	static MojTestRunner* instance() { return s_instance; }
	void testError(MojErr errTest, const MojChar* file, int line);
	void testFailed(const MojChar* cond, const MojChar* file, int line);

protected:
	virtual MojErr run();
	virtual void runTests() = 0;
	void test(MojTestCase& test);
	void test(MojTestCaseRef ref) { test(ref.m_case); }

private:
	virtual MojErr handleArgs(const StringVec& args);
	bool testEnabled(MojTestCase& test);

	int m_numSucceeded;
	int m_numFailed;
	MojTestCase* m_curTest;
	StringVec m_testNames;
	MojThreadMutex m_mutex;
	static MojTestRunner* s_instance;
};

#endif /* MOJTESTRUNNER_H_ */
