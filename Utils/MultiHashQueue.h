#pragma once

#include <afxtempl.h>
//#include <hash_map>

namespace util {

	//En kombination av en hashmap och en länkad lista, dvs en ordnad hashmap.
	template <class K1, class K2, class V>
	class MultiHashQueue {
	public:
		MultiHashQueue() : nullItem() { headNode = tailNode = null; totalSize = 0; };
		virtual ~MultiHashQueue() { clear(); };

		void clear();
		bool lookup(const K1 &key, V &value) const;
		bool lookup(const K2 &key, V &value) const;
		void addHead(const K1 &key, const K2 &key2, const V &value);
		void addTail(const K1 &key, const K2 &key2, const V &value);
		bool update(const K1 &key, const K2 &newKey2, const V &newValue);
		bool update(const K2 &key, const K1 &newKey1, const V &newValue);
		void remove(const K1 &key);
		void remove(const K2 &key);
		int size() const;
		V &head();
		V &tail();
		void removeHead();
		void removeTail();

		//Bra att ha: Möjlighet att iterera över strukturen!
	private:
		class Node {
		public:
			Node(const K1 &key1, const K2 &key2, const V &value) { this->key1 = key1; this->key2 = key2; this->value = value; next = prev = null; };
			Node(const K1 &key1, const K2 &key2, const V &value, Node *next) { this->key1 = key1; this->key2 = key2; this->value = value; prev = null; this->next = next; };
			Node(Node *prev, const K1 &key1, const K2 &key2, const V &value) { this->key1 = key1; this->key2 = key2; this->value = value; this->prev = prev; next = null; };

			K1 key1;
			K2 key2;
			V value;
			Node *prev;
			Node *next;
		};

		CMap<K1, const K1 &, Node *, Node *&> map1;
		CMap<K2, const K2 &, Node *, Node *&> map2;
		//hash_map<K1, Node *> map1;
		//hash_map<K2, Node *> map2;

		Node *headNode;
		Node *tailNode;

		V nullItem;

		int totalSize;

		void remove(Node *node);
	};


	//////////////////////////////////////////////////////////////////////////
	// Implementation
	//////////////////////////////////////////////////////////////////////////

	template <class K1, class K2, class V>
	void MultiHashQueue<K1, K2, V>::clear() {
		Node *current = headNode;
		while (current) {
			Node *next = current->next;
			delete current;
			current = next;
		}

		map1.RemoveAll();
		map2.RemoveAll();
		headNode = tailNode = null;
		totalSize = 0;
	}

	template <class K1, class K2, class V>
	bool MultiHashQueue<K1, K2, V>::lookup(const K1 &key, V &value) const {
		Node *internalValue;
		if (map1.Lookup(key, internalValue)) {
			value = internalValue->value;
			return true;
		}
		return false;
	}

	template <class K1, class K2, class V>
	bool MultiHashQueue<K1, K2, V>::lookup(const K2 &key, V &value) const {
		Node *internalValue;
		if (map2.Lookup(key, internalValue)) {
			value = internalValue->value;
			return true;
		}
		return false;
	}

	template <class K1, class K2, class V>
	void MultiHashQueue<K1, K2, V>::addHead(const K1 &key1, const K2 &key2, const V &value) {
		if (headNode == null) {
			headNode = tailNode = new Node(key1, key2, value);
		} else {
			Node *lastHead = headNode;
			headNode = new Node(key1, key2, value, headNode);
			lastHead->prev = headNode;
		}

		map1[key1] = headNode;
		map2[key2] = headNode;
		totalSize++;
	}

	template <class K1, class K2, class V>
	void MultiHashQueue<K1, K2, V>::addTail(const K1 &key1, const K2 &key2, const V &value) {
		if (tailNode == null) {
			headNode = tailNode = new Node(key1, key2, value);
		} else {
			Node *lastTail = tailNode;
			tailNode = new Node(tailNode, key1, key2, value);
			lastTail->next = tailNode;
		}

		map1[key1] = tailNode;
		map2[key2] = tailNode;
		totalSize++;
	}

	template <class K1, class K2, class V>
	void MultiHashQueue<K1, K2, V>::remove(const K1 &key) {
		Node *item = null;
		if (map1.Lookup(key, item)) {
			remove(item);
		}
	}

	template <class K1, class K2, class V>
	void MultiHashQueue<K1, K2, V>::remove(const K2 &key) {
		Node *item = null;
		if (map2.Lookup(key, item)) {
			remove(item);
		}
	}

	template <class K1, class K2, class V>
	void MultiHashQueue<K1, K2, V>::remove(Node *node) {
		if (headNode == node) {
			headNode = node->next;
		}

		if (tailNode == node) {
			tailNode = node->prev;
		}

		if (node->prev) node->prev->next = node->next;
		if (node->next) node->next->prev = node->prev;

		map1.RemoveKey(node->key1);
		map2.RemoveKey(node->key2);
		delete node;
		totalSize--;
	}

	template <class K1, class K2, class V>
	bool MultiHashQueue<K1, K2, V>::update(const K1 &key, const K2 &newKey2, const V &newValue) {
		Node *item = null;
		if (map1.Lookup(key, item)) {
			map2.RemoveKey(item->key2);
			map2[newKey2] = item;
			item->value = newValue;
			return true;
		} else {
			return false;
		}
	}

	template <class K1, class K2, class V>
	bool MultiHashQueue<K1, K2, V>::update(const K2 &key, const K1 &newKey1, const V &newValue) {
		Node *item = null;
		if (map2.Lookup(key, item)) {
			map1.RemoveKey(item->key1);
			map1[newKey1] = item;
			item->value = newValue;
			return true;
		} else {
			return false;
		}
	}

	template <class K1, class K2, class V>
	void MultiHashQueue<K1, K2, V>::removeHead() {
		remove(headNode);
	}

	template <class K1, class K2, class V>
	void MultiHashQueue<K1, K2, V>::removeTail() {
		remove(tailNode);
	}

	template <class K1, class K2, class V>
	int MultiHashQueue<K1, K2, V>::size() const {
		return totalSize;
	}

	template <class K1, class K2, class V>
	V &MultiHashQueue<K1, K2, V>::head() {
		if (headNode) return headNode->value;
		return nullItem;
	}

	template <class K1, class K2, class V>
	V &MultiHashQueue<K1, K2, V>::tail() {
		if (tailNode) return tailNode->value;
		return nullItem;
	}
};