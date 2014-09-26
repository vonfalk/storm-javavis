#pragma once

//#include <hash_map>
#include <afxtempl.h>

namespace util {

	//En kombination av en hashmap och en länkad lista, dvs en ordnad hashmap.
	template <class K, class V>
	class HashQueue {
	public:
		HashQueue() : nullItem() { headNode = tailNode = null; };
		virtual ~HashQueue() { clear(); };

		void clear();
		bool lookup(const K &key, V &value) const;
		void addHead(const K &key, const V &value);
		void addTail(const K &key, const V &value);
		void remove(const K &key);
		int size() const;
		V &head();
		V &tail();
		void removeHead();
		void removeTail();

		//Bra att ha: Möjlighet att iterera över strukturen!
	private:
		class Node {
		public:
			Node(const K &key, const V &value) { this->key = key; this->value = value; next = prev = null; };
			Node(const K &key, const V &value, Node *next) { this->key = key; this->value = value; prev = null; this->next = next; };
			Node(Node *prev, const K &key, const V &value) { this->key = key; this->value = value; this->prev = prev; next = null; };

			K key;
			V value;
			Node *prev;
			Node *next;
		};

		CMap<K, const K &, Node *, Node *&> map;
		//hash_map<K, Node *> map;

		Node *headNode;
		Node *tailNode;

		V nullItem;

		void remove(Node *node);
	};


	//////////////////////////////////////////////////////////////////////////
	// Implementation
	//////////////////////////////////////////////////////////////////////////

	template <class K, class V>
	void HashQueue<K, V>::clear() {
		Node *current = headNode;
		while (current) {
			Node *next = current->next;
			delete current;
			current = next;
		}

		map.RemoveAll();
		headNode = tailNode = null;
	}

	template <class K, class V>
	bool HashQueue<K, V>::lookup(const K &key, V &value) const {
		Node *internalValue;
		if (map.Lookup(key, internalValue)) {
			value = internalValue->value;
			return true;
		}
		return false;
	}

	template <class K, class V>
	void HashQueue<K, V>::addHead(const K &key, const V &value) {
		remove(key);

		if (headNode == null) {
			headNode = tailNode = new Node(key, value);
		} else {
			Node *lastHead = headNode;
			headNode = new Node(key, value, headNode);
			lastHead->prev = headNode;
		}

		map[key] = headNode;
	}

	template <class K, class V>
	void HashQueue<K, V>::addTail(const K &key, const V &value) {
		remove(key);

		if (tailNode == null) {
			headNode = tailNode = new Node(key, value);
		} else {
			Node *lastTail = tailNode;
			tailNode = new Node(tailNode, key, value);
			lastTail->next = tailNode;
		}

		map[key] = tailNode;
	}

	template <class K, class V>
	void HashQueue<K, V>::remove(const K &key) {
		Node *item = null;
		if (map.Lookup(key, item)) {
			remove(item);
		}
	}

	template <class K, class V>
	void HashQueue<K, V>::remove(Node *node) {
		if (headNode == node) {
			headNode = node->next;
		}

		if (tailNode == node) {
			tailNode = node->prev;
		}

		if (node->prev) node->prev->next = node->next;
		if (node->next) node->next->prev = node->prev;

		map.RemoveKey(node->key);
		delete node;
	}

	template <class K, class V>
	void HashQueue<K, V>::removeHead() {
		remove(headNode);
	}

	template <class K, class V>
	void HashQueue<K, V>::removeTail() {
		remove(tailNode);
	}

	template <class K, class V>
	int HashQueue<K, V>::size() const {
		return map.GetSize();
	}

	template <class K, class V>
	V &HashQueue<K, V>::head() {
		if (headNode) return headNode->value;
		return nullItem;
	}

	template <class K, class V>
	V &HashQueue<K, V>::tail() {
		if (tailNode) return tailNode->value;
		return nullItem;
	}
};