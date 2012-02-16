/* ============================================================
 * Date  : Jan 5, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJDBDEFS_H_
#define MOJDBDEFS_H_

#include "core/MojCoreDefs.h"

class MojDb;
class MojDbBatch;
class MojDbClient;
class MojDbCursor;
class MojDbIndex;
class MojDbKey;
class MojDbKeyRange;
class MojDbKind;
class MojDbKindEngine;
class MojDbObjectItem;
class MojDbPermissionEngine;
class MojDbPropExtractor;
class MojDbPutHandler;
class MojDbQuery;
class MojDbQueryFilter;
class MojDbQueryPlan;
class MojDbQuotaEngine;
class MojDbReq;
class MojDbSearchCursor;
class MojDbServiceClient;
class MojDbServiceClientBatch;
class MojDbServiceHandler;
class MojDbTextCollator;
class MojDbTextTokenizer;
class MojDbWatcher;

class MojDbStorageCursor;
class MojDbStorageDatabase;
class MojDbStorageEngine;
class MojDbStorageIndex;
class MojDbStorageItem;
class MojDbStorageQuery;
class MojDbStorageSeq;
class MojDbStorageTxn;

enum MojDbOp {
	OpNone 		= 0,
	OpUndefined = (1 << 0),
	OpCreate 	= (1 << 1),
	OpRead 		= (1 << 2),
	OpUpdate 	= (1 << 3),
	OpDelete 	= (1 << 4),
	OpExtend 	= (1 << 5),
	OpKindUpdate = (1 << 6)
};

enum MojDbCollationStrength {
	MojDbCollationInvalid,
	MojDbCollationPrimary,
	MojDbCollationSecondary,
	MojDbCollationTertiary,
	MojDbCollationQuaternary,
	MojDbCollationIdentical
};

#endif /* MOJDBDEFS_H_ */
