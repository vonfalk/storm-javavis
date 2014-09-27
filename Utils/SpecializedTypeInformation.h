#pragma once

/************************************************************************/
/* Notera: Klasserna ska vara ordnade så att den mest generella klassen */
/* är först (överst) i listan och de mest specifika klasserna är sist i */
/* listan (nederst). Annars kan getType dra fel slutsatser.             */
/************************************************************************/

namespace util {

	template <class T, class S>
	T *createObject() {
		return new S();
	}

	template<class T>
	class TypeInformation<T, void> {
	public:
		TypeInformation(int numObjects);
		~TypeInformation();

		template<class S>
		void add(const String &name, int id) {
			recognizers[id] = recognize<T, S>;
			creators[id] = createObject<T, S>;
			names[id] = name;
		}

		int getCount() const { return numClasses; };
		String getName(int id) const;
		int getType(const T *c) const;
		T *create(int id) const;
	private:
		typedef bool (*recognizer)(const T *);
		typedef T *(*creator)();

		//int lastId;

		recognizer *recognizers;
		creator *creators;
		String *names;
		int numClasses;
	};


	template<class T>
	TypeInformation<T, void>::TypeInformation(int size) {
		recognizers = new recognizer[size];
		creators = new creator[size];
		names = new String[size];
		//lastId = 0;
		numClasses = size;
	}

	template<class T>
	TypeInformation<T, void>::~TypeInformation() {
		delete []recognizers;
		delete []creators;
		delete []names;
	}

	template<class T>
	String TypeInformation<T, void>::getName(int id) const {
		if ((id < 0) || (id >= numClasses)) return L"INVALID TYPE";
		return names[id];
	}

	template<class T>
	int TypeInformation<T, void>::getType(const T *c) const {
		for (int i = numClasses - 1; i >= 0; i--) {
			if ((*(recognizers[i]))(c)) {
				return i;
			}
		}

		return -1;
	}

	template<class T>
	T *TypeInformation<T, void>::create(int id) const {
		if ((id < 0) || (id >= numClasses)) return null;
		return (*(creators[id]))();
	}
};
