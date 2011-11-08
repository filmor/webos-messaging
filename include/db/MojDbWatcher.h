/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBWATCHER_H_
#define MOJDBWATCHER_H_

#include "db/MojDbDefs.h"
#include "db/MojDbQueryPlan.h"
#include "core/MojSignal.h"
#include "core/MojThread.h"

class MojDbWatcher : public MojSignalHandler
{
public:
	typedef MojVector<MojDbKeyRange> RangeVec;
	typedef MojSignal<> Signal;

	MojDbWatcher(Signal::SlotRef handler);
	~MojDbWatcher();

	void domain(const MojString& val) { m_domain = val; }
	const MojString& domain() const { return m_domain; }
	const RangeVec& ranges() const { return m_ranges; }

	void init(MojDbIndex* index, const RangeVec& ranges, bool desc, bool active);
	MojErr activate(const MojDbKey& endKey);
	MojErr fire(const MojDbKey& key);

private:
	typedef enum {
		StateInvalid,
		StatePending,
		StateActive
	} State;

	virtual MojErr handleCancel();
	MojErr fireImpl();
	MojErr invalidate();

	MojThreadMutex m_mutex;
	Signal m_signal;
	RangeVec m_ranges;
	MojDbKey m_limitKey;
	MojDbKey m_fireKey;
	MojString m_domain;
	bool m_desc;
	State m_state;
	MojDbIndex* m_index;
};

#endif /* MOJDBWATCHER_H_ */
