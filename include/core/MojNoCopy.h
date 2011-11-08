/* ============================================================
 * Date  : Jan 12, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJNOCOPY_H_
#define MOJNOCOPY_H_

class MojNoCopy
{
protected:
	MojNoCopy() {}

private:
	MojNoCopy(const MojNoCopy&);
	MojNoCopy& operator=(const MojNoCopy&);
};

#endif /* MOJNOCOPY_H_ */
