#pragma once

/************************************************************************/
/* Notera: Klasserna ska vara ordnade så att den mest generella klassen */
/* är först (överst) i listan och de mest specifika klasserna är sist i */
/* listan (nederst). Annars kan getType dra fel slutsatser.             */
/************************************************************************/

namespace util {

	template<class BaseClass, class T>
	bool recognize(const BaseClass *object) {
		return dynamic_cast<const T *>(object) != null;
	}

	template<class T, class S, class CreateArg>
	T *createObject(CreateArg arg) {
		return new S(arg);
	}


	template<class T, class CreateArg>
	class TypeInformation {
	public:
		TypeInformation(int numObjects);
		~TypeInformation();

		template<class S>
		void add(const CString &name, int id) {
			recognizers[id] = recognize<T, S>;
			creators[id] = createObject<T, S, CreateArg>;
			names[id] = name;
		}

		int getCount() const { return numClasses; };
		CString getName(int id) const;
		int getType(const T *c) const;
		T *create(int id, CreateArg arg) const;
	private:
		typedef bool (*recognizer)(const T *);
		typedef T *(*creator)(CreateArg);

		//int lastId;

		recognizer *recognizers;
		creator *creators;
		CString *names;
		int numClasses;
	};


	template<class T, class CreateArg>
	TypeInformation<T, CreateArg>::TypeInformation(int size) {
		recognizers = new recognizer[size];
		creators = new creator[size];
		names = new CString[size];
		//lastId = 0;
		numClasses = size;
	}

	template<class T, class CreateArg>
	TypeInformation<T, CreateArg>::~TypeInformation() {
		delete []recognizers;
		delete []creators;
		delete []names;
	}

	template<class T, class CreateArg>
	CString TypeInformation<T, CreateArg>::getName(int id) const {
		if ((id < 0) || (id >= numClasses)) return L"INVALID TYPE";
		return names[id];
	}

	template<class T, class CreateArg>
	int TypeInformation<T, CreateArg>::getType(const T *c) const {
		for (int i = numClasses - 1; i >= 0; i--) {
			if ((*(recognizers[i]))(c)) {
				return i;
			}
		}

		return -1;
	}

	template<class T, class CreateArg>
	T *TypeInformation<T, CreateArg>::create(int id, CreateArg arg) const {
		if ((id < 0) || (id >= numClasses)) return null;
		return (*(creators[id]))(arg);
	}
};

#define DECLARE_TYPE_INFORMATION(baseClass, createArg, memberName) \
	class TypeInfo ## memberName : public util::TypeInformation< baseClass , createArg > { \
	public: \
		TypeInfo ## memberName(); \
	}; \
	static TypeInfo ## memberName memberName;

#define DECLARE_PUBLIC_GET_INTERFACE(baseClass, memberName) \
	static inline int getTypeCount() { return memberName.getCount(); }; \
	static inline int getType(const baseClass *object) { return memberName.getType(object); }; \
	static inline CString getTypeName(int typeId) { return memberName.getName(typeId); }; \
	static inline CString getTypeName(const baseClass *object) { return getTypeName(getType(object)); };

#define DECLARE_TYPE_GETTER(memberName) \
	inline int getType() const { return memberName.getType(this); }; \
	inline CString getTypeName() const { return memberName.getName(memberName.getType(this)); };

#define DECLARE_PUBLIC_CREATE_INTERFACE(baseClass, createArg, memberName) \
	static inline baseClass *createType(int id, createArg arg) { return memberName.create(id, arg); };

#define BEGIN_TYPES(thisClass, memberName, numTypes) \
	thisClass::TypeInfo ## memberName thisClass::memberName; \
	thisClass::TypeInfo ## memberName::TypeInfo ## memberName() : TypeInformation(numTypes) { \
		int currentId = 0;

//Felaktigt deklarerade typer ...
#define END_TYPES() \
		ASSERT(currentId == getCount()); \
	}

#define TYPE(type, name) \
	ASSERT(currentId < getCount()); \
	add< type >(name, currentId++);

#include "SpecializedTypeInformation.h"