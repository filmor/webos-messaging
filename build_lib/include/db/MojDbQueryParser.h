/* ============================================================
 * Date  : Jul 24, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBQUERYPARSER_H_
#define MOJDBQUERYPARSER_H_

#include "db/MojDbDefs.h"

class MojDbQueryParser
{
public:
	MojDbQueryParser();

	MojErr parse(const MojChar* str, MojSize len = MojSizeMax);

private:
	typedef enum {
		StateWhitespace,
		StateSelect,
		StatePropName,
		StateObject,
		StateComp
	} State;

	static const MojSize MaxStackDepth = 3;

	State m_stack[MaxStackDepth];
	MojSize m_depth;
};

#endif /* MOJDBQUERYPARSER_H_ */
