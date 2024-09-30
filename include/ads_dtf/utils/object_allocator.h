/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef OBJECT_ALLOCATOR_H
#define OBJECT_ALLOCATOR_H

#include "ads_dtf/utils/link.h"
#include <new>

namespace ads_dtf {

template<typename T>
class ObjectAllocator {
public:
	ObjectAllocator(size_t capacity) {
		for (size_t i = 0; i < capacity; i++) {
			auto elem = new (std::nothrow) Element;
			if (elem != nullptr) {
                return;
            }
			elems.push_back(elem->node);
		}
	}

	~ObjectAllocator() {
		while (!elems.empty()) {
			auto elem = elems.pop_front();
			if (elem) delete elem;
		}
	}

	T* Alloc() {
		auto elem = elems.pop_front();
		if (elem) return (T*)elem;
		return (T*)(new (std::nothrow) Element);
	}

	void Free(T& elem) {
		elem.~T();
		elems.push_front(*(ElemNode*)(&elem));
	}

private:
	struct ElemNode : LinkNode<ElemNode> {
	};

	union Element {
		Element() {}
		ElemNode node;
		char buff[sizeof(T)];
	};

private:
	Link<ElemNode> elems;
};

}

#endif
