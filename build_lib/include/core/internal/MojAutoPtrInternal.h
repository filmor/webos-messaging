/* ============================================================
 * Date  : Jan 7, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJAUTOPTRINTERNAL_H_
#define MOJAUTOPTRINTERNAL_H_

template<class T, class DTOR>
void MojSharedPtrBase<T, DTOR>::destroy()
{
	if (m_impl && --(m_impl->m_refcount) == 0)
		delete m_impl;
}

template<class T, class DTOR>
MojErr MojSharedPtrBase<T, DTOR>::reset(T* p)
{
	if (m_impl && --(m_impl->m_refcount) == 0) {
		if (p) {
			m_impl->destroy();
			m_impl->m_p = p;
			m_impl->m_refcount = 1;
		} else {
			delete m_impl;
		}
	} else if (p) {
		m_impl = new MojSharedPtrImpl<T, DTOR>(p);
		if (!m_impl) {
			DTOR()(p);
			MojErrThrow(MojErrNoMem);
		}
	} else {
		m_impl = NULL;
	}

	return MojErrNone;
}

template<class T, class DTOR>
MojErr MojSharedPtrBase<T, DTOR>::resetChecked(T* p)
{
	if (p == NULL)
		MojErrThrow(MojErrNoMem);
	MojErr err = reset(p);
	MojErrCheck(err);

	return MojErrNone;
}

template<class T, class DTOR>
void MojSharedPtrBase<T, DTOR>::assign(MojSharedPtrImpl<T, DTOR>* impl)
{
	destroy();
	m_impl = impl;
	init();
}

#endif /* MOJAUTOPTRINTERNAL_H_ */
