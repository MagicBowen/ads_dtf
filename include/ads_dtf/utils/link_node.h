/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef LINK_NODE_H
#define LINK_NODE_H

namespace ads_dtf {

template <typename T> struct List;

template <typename T>
struct LinkNode {
	LinkNode() {
		link.prev = 0;
		link.next = 0;
	}

	void remove() {
		// Notice: Just used in scenes careless num of link!!!
		link.prev->link.next = link.next;
		link.next->link.prev = link.prev;
	}

	T* next() {
		return link.next;
	}

	const T* next() const {
		return link.next;
	}

	T* prev() {
		return link.prev;
	}

	const T* prev() const {
		return link.prev;
	}

	friend struct List<T>;

	struct Chain {
		T * volatile next;
		T * volatile prev;
	}; // __cacheline_aligned;

	Chain link;
};

}

#endif
