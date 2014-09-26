#pragma once

namespace util {

	template <class T>
	class Queue {
	public:
		Queue() {
			first = 0;
			last = 0;
			//use = false;
			empt = T();
			InitializeCriticalSection(&cs);
		}

		virtual ~Queue() {
			empty();
			DeleteCriticalSection(&cs);
		}

		void empty() {
			wait();

			data *tFirst = first;
			data *tLast = last;
			first = last = 0; //Tom för omvärlden

			done();

			data *now = tFirst;
			while (now != 0) {
				data *tmp = now;
				now = now->next;

				delete tmp;
			}
		}

		bool isEmpty() {
			if (first == 0) return true;
			return false;
		}

		T &peek() {
			if (isEmpty()) return empt;
			return first->item;
		}

		T pop() {
			if (isEmpty()) return T();
			wait();

			if (first == last) {
				data *ptr = first;
				last = first = 0;
				T tmp = ptr->item;
				delete ptr;

				done();
				return tmp;
			} else {
				data *ptr = first;
				first = first->next;

				T tmp = ptr->item;
				delete ptr;

				done();
				return tmp;
			}
		}

		void push(const T &item) {
			wait();

			if (isEmpty()) {
				first = last = new data(item);
			} else {
				data *tmp = new data(item);
				last->next = tmp;
				last = tmp;
			}
			done();
		}

		int count() {
			wait();
			int ret = 0;
			data *now = first;
			while (now != 0) {
				now = now->next;
				ret++;
			}

			done();
			return ret;
		}

	private:
		class data {
		public:
			data(const T &item) {
				this->item = item;
				next = 0;
			}

			data(const T &item, data *next) {
				this->item = item;
				this->next = next;
			}

			~data() {
			}

			T item;
			data *next;
			data *prev;
		};

		data *first, *last;
		//bool use;
		CRITICAL_SECTION cs;
		T empt;

		void wait()
		{
			EnterCriticalSection(&cs);
			/*while(use) {
				Sleep(0);
			}
			use = true;*/
		}

		void done()
		{
			LeaveCriticalSection(&cs);
		}
	};

}