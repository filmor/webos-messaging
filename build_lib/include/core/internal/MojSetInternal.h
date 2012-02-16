/* ============================================================
 * Date  : Feb 23, 2009
 * Copyright 2009 Palm, Inc. All rights reserved.
 * ============================================================ */

#ifndef MOJSETINTERNAL_H_
#define MOJSETINTERNAL_H_

template<>
struct MojComp<const MojSet<class T, class COMP> >
{
	int operator()(const MojSet<T, COMP>& val1, const MojSet<T, COMP>& val2)
	{
		return val1.compare(val2);
	}
};

template<>
struct MojComp<MojSet<class T, class COMP> >
: public MojComp<const MojSet<T, COMP> > {};

template<class T, class COMP> inline
bool MojSet<T, COMP>::ConstIterator::operator==(const Iterator& rhs) const
{
	return m_node == rhs.m_node;
}

template<class T, class COMP> inline
bool MojSet<T, COMP>::ConstIterator::operator!=(const Iterator& rhs) const
{
	return m_node != rhs.m_node;
}

template<class T, class COMP> inline
void MojSet<T, COMP>::ConstIterator::operator=(const Iterator& rhs)
{
	m_node = rhs.m_node;
}

template<class T, class COMP>
MojErr MojSet<T, COMP>::put(const ValueType& val)
{
	MojAutoPtr<Node> node(new SetNode(val));
	MojAllocCheck(node.get());
	MojErr err = MojRbTreeBase::put(node, &val);
	MojErrCheck(err);

	return MojErrNone;
}

template<class T, class COMP>
MojErr MojSet<T, COMP>::intersect(const MojSet& set)
{
	bool found = false;
	for (ConstIterator i = begin(); i != end();) {
		const ValueType& val = *i;
		++i;
		if (!set.contains(val)) {
			MojErr err = del(val, found);
			MojErrCheck(err);
		}
	}
	return MojErrNone;
}

template<class T, class COMP>
int MojSet<T, COMP>::compareFull(const Node* node1, const Node* node2) const
{
	MojAssert(node1 && node2);
	return COMP()(static_cast<const SetNode*>(node1)->m_val, static_cast<const SetNode*>(node2)->m_val);
}

template<class T, class COMP>
int MojSet<T, COMP>::compareKey(const Node* node, const void* key) const
{
	MojAssert(node && key);
	return COMP()(static_cast<const SetNode*>(node)->m_val, *static_cast<const ValueType*>(key));
}

template<class T, class COMP>
void MojSet<T, COMP>::deleteNode(Node* node) const
{
	MojAssert(node);
	delete static_cast<SetNode*>(node);
}

template<class T, class COMP>
MojRbTreeBase::Node* MojSet<T, COMP>::cloneNode(const Node* node) const
{
	MojAssert(node);
	return new SetNode(static_cast<const SetNode*>(node)->m_val);
}

template<class T, class COMP>
const void* MojSet<T, COMP>::key(const Node* node) const
{
	MojAssert(node);
	return &static_cast<const SetNode*>(node)->m_val;
}

#endif /* MOJSETINTERNAL_H_ */
