/* ============================================================
 * Date  : Apr 15, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBTEXTTOKENIZER_H_
#define MOJDBTEXTTOKENIZER_H_

#include "db/MojDbDefs.h"
#include "core/MojRefCount.h"
#include "core/MojSet.h"
#include "unicode/ubrk.h"

class MojDbTextTokenizer : public MojRefCounted
{
public:
	typedef MojSet<MojDbKey> KeySet;

	MojDbTextTokenizer();
	~MojDbTextTokenizer();

	MojErr init(const MojChar* locale);
	MojErr tokenize(const MojString& text, MojDbTextCollator* collator, KeySet& keysOut) const;

private:
	struct IterCloser
	{
		void operator()(UBreakIterator* ubrk)
		{
			ubrk_close(ubrk);
		}
	};
	typedef MojAutoPtrBase<UBreakIterator, IterCloser> IterPtr;

	IterPtr m_ubrk;
};

#endif /* MOJDBTEXTTOKENIZER_H_ */
