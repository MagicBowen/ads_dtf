/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef LINK_H
#define LINK_H

#include "ads_dtf/utils/link_node.h"
#include <stddef.h>
#include <new>

namespace ads_dtf {

template<typename T>
struct Link {

	struct Iterator {
		Iterator() noexcept :
				elem(0), next(0) {
		}

		Iterator(T *elem) noexcept :
				elem(elem), next(next_of(elem)) {
		}

		Iterator(const Iterator &rhs) noexcept :
				elem(rhs.elem), next(rhs.next) {
		}

		Iterator& operator=(const Iterator &other) noexcept {
			if (this != &other) {
				elem = other.elem;
				next = other.next;
			}
			return *this;
		}

		bool operator==(const Iterator &rhs) const noexcept {
			return elem == rhs.elem;
		}

		bool operator!=(const Iterator &rhs) const noexcept {
			return !(*this == rhs);
		}

		void reset() noexcept {
			elem = 0;
			next = 0;
		}

		T* operator->() noexcept {
			return elem;
		}

		T& operator*() noexcept {
			return *elem;
		}

		T* value() const noexcept {
			return elem;
		}

		Iterator operator++(int) noexcept {
			Iterator i = *this;

			elem = next;
			next = next_of(elem);

			return i;
		}

		Iterator& operator++() noexcept {
			elem = next;
			next = next_of(elem);

			return *this;
		}

	private:
		static T* next_of(T *elem) noexcept {
			return elem == 0 ? 0 : Link<T>::next_of(elem);
		}

		static T* prev_of(T *elem) noexcept {
			return elem == 0 ? 0 : Link<T>::prev_of(elem);
		}
	private:
		T *elem;
		T *next;
	};

	struct ReverseIterator {
		ReverseIterator() noexcept :
				elem(0), next(0) {
		}

		ReverseIterator(T *elem) noexcept :
				elem(elem), next(next_of(elem)) {
		}

		ReverseIterator(const ReverseIterator &rhs) noexcept :
				elem(rhs.elem), next(rhs.next) {
		}

		ReverseIterator& operator=(const ReverseIterator &other) noexcept {
			if (this != &other) {
				elem = other.elem;
				next = other.next;
			}
			return *this;
		}

		bool operator==(const ReverseIterator &rhs) const noexcept {
			return elem == rhs.elem;
		}

		bool operator!=(const ReverseIterator &rhs) const noexcept {
			return !(*this == rhs);
		}

		void reset() noexcept {
			elem = 0;
			next = 0;
		}

		T* operator->() noexcept {
			return elem;
		}

		T& operator*() noexcept {
			return *elem;
		}

		T* value() const noexcept {
			return elem;
		}

		ReverseIterator operator++(int) noexcept {
			ReverseIterator i = *this;

			elem = next;
			next = next_of(elem);

			return i;
		}

		ReverseIterator& operator++() noexcept {
			elem = next;
			next = next_of(elem);

			return *this;
		}

	private:
		static T* next_of(T *elem) noexcept {
			return elem == 0 ? 0 : Link<T>::prev_of(elem);
		}

	private:
		T *elem;
		T *next;
	};

	Link() : num(0) {
		head.next = sentinel();
		head.prev = sentinel();
	}

	Link(Link && other) noexcept {
		*this = std::move(other);
	}

	Link& operator=(Link && other) noexcept {
		if (this != &other) {
			this->reset();
			concat(other);
		}
		return *this;
	}

	bool empty() const {
		return head.next == sentinel();
	}

	size_t size() const {
		return num;
	}

	Iterator begin() const {
		return head.next;
	}

	Iterator end() const {
		return const_cast<T*>(sentinel());
	}

	ReverseIterator rbegin() const {
		return head.prev;
	}

	ReverseIterator rend() const {
		return const_cast<T*>(sentinel());
	}

	bool is_back(T *elem) const {
		return elem == head.prev;
	}

	bool is_front(T *elem) const {
		return elem == head.next;
	}

	T* front() const {
		return empty() ? 0 : head.next;
	}

	T* back() const {
		return empty() ? 0 : head.prev;
	}

	void push_back(T &elem) {
		elem.link.next = sentinel();
		elem.link.prev = sentinel()->link.prev;
		sentinel()->link.prev->link.next = &elem;
		sentinel()->link.prev = &elem;

		num++;
	}

	void push_front(T &elem) {
		elem.link.prev = sentinel();
		elem.link.next = sentinel()->link.next;
		sentinel()->link.next->link.prev = &elem;
		sentinel()->link.next = &elem;

		num++;
	}

	void push_back(Iterator &elem) {
		push_back(*elem);
	}

	void push_front(Iterator &elem) {
		push_front(*elem);
	}

	T* pop_front() {
		Iterator i = begin();
		if (i == end()) {
			return 0;
		}

		erase(i);
		return &(*i);
	}

	void remove(T &elem) {
		elem.remove();
		num--;
	}

	void erase(Iterator elem) {
		if (!elem.value())
			return;

		remove(*elem);
	}

	void clear() {
		while(pop_front() != 0);
	}

	Iterator next_of(Iterator &i) const {
		return (i == end()) ? end() : ++i;
	}

private:
	static T* next_of(T *elem) {
		return elem->link.next;
	}

	static T* prev_of(T *elem) {
		return elem->link.prev;
	}

	void reset() {
		head.next = sentinel();
		head.prev = sentinel();
		num = 0;
	}

	T* sentinel() {
		return (T*)((char *)(&(head.next)) - offsetof(T, link));
	}

	const T* sentinel() const {
		return (T*)((char *)(&(head.next)) - offsetof(T, link));
	}

private:
	typename LinkNode<T>::Chain head;
	size_t num;
};

}

#endif
