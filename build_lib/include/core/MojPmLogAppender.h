/* ============================================================
 * Date  : Sep 20, 2010
 * Copyright 2010 Palm, Inc. All rights reserved.
 * ============================================================ */


#ifndef MOJPMLOGAPPENDER_H_
#define MOJPMLOGAPPENDER_H_

#include "core/MojCoreDefs.h"
#include "core/MojLogEngine.h"

class MojPmLogAppender : public MojLogAppender
{
public:
	MojPmLogAppender();
	~MojPmLogAppender();

	virtual MojErr append(MojLogger::Level level, MojLogger* logger, const MojChar* format, va_list args);
};

#endif /* MOJPMLOGAPPENDER_H_ */
