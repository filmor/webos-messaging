/* ============================================================
 * Date  : Jan 13, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJNEW_H_
#define MOJNEW_H_

#include <new>

#ifdef MOJ_INTERNAL
	// ensure that we always use the nothrow version of new internally
	inline void* operator new(std::size_t size)
	{
		return operator new(size, std::nothrow);
	}

	inline void* operator new[](std::size_t size)
	{
		return operator new[](size, std::nothrow);
	}
#endif /* MOJ_INTERNAL */

#endif /* MOJNEW_H_ */
