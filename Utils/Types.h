#pragma once

#include <vector>

namespace util {

	// Class encapsulating creation and recognition of subclasses
	// of the class T. See usage example in "Tests/TypesTest.cpp"
	// Use the Types class below. The TypesBase class is not designed
	// to be used directly.
	template <class T, class CtorFn>
	class TypesBase : NoCopy {
	public:
		// Invalid id constant.
		static const nat NO_ID = -1;

		// Add a known subclass.
		template <class Derived>
		void add(const String &name);

		// Get the type id of "t".
		nat typeOf(const T *t) const;

		// Get the type name of "t".
		String typeNameOf(const T *t) const;

		// Get the type name of a type with id "id".
		String typeName(nat id) const;

		// Get the total number of types.
		nat size() const;

		// Any types known?
		inline bool any() const { return size() > 0; }
	protected:
		// Recognizer function pointer.
		typedef bool (* Recognizer)(const T *);

		typedef CtorFn Ctor;

		// All information required by a single derived type.
		struct Type {
			Ctor ctor;
			Recognizer recognize;
			String name;
		};

		// List of all known types.
		vector<Type> types;

		template <class D>
		static bool recognize(const T *t) {
			return typeid(*t) == typeid(D);
		}
	};

	// Specialization for the case of a ctor param.
	template <class T, class Param = void>
	class Types : public TypesBase<T, T *(*)(const Param &)> {
	public:
		// Add an object.
		template <class D>
		void add(const String &name) {
			Type type = {
				&Types<T, Param>::create<D>,
				&Types<T, Param>::recognize<D>,
				name,
			};
			types.push_back(type);
		}

		// Create an object.
		T *create(nat id, const Param &p) const {
			if (id == NO_ID) return null;
			return (*types[id].ctor)(p);
		}

	private:
		template <class D>
		static T *create(const Param &p) {
			return new D(p);
		}
	};

	// Specialization for the case of no ctor param.
	template <class T>
	class Types<T, void> : public TypesBase<T, T *(*)()> {
	public:
		// Add an object.
		template <class D>
		void add(const String &name) {
			Type type = {
				&Types<T, void>::create<D>,
				&Types<T, void>::recognize<D>,
				name,
			};
			types.push_back(type);
		}

		// Create an object.
		T *create(nat id) const {
			if (id == NO_ID) return null;
			return (*types[id].ctor)();
		}

	private:
		template <class D>
		static T *create() {
			return new D();
		}
	};


	template <class T, class Param>
	template <class D>
	void TypesBase<T, Param>::add(const String &name) {
		Type type = {
			&TypeFunctions<T, D, Param>::create,
			&TypeFunctions<T, D, Param>::recognize,
			name,
		};
		types.push_back(type);
	}

	template <class T, class Param>
	nat TypesBase<T, Param>::typeOf(const T *t) const {
		for (nat i = 0; i < types.size(); i++) {
			const Type &c = types[i];
			if ((*c.recognize)(t)) return i;
		}
		return NO_ID;
	}

	template <class T, class Param>
	String TypesBase<T, Param>::typeNameOf(const T *t) const {
		return typeName(typeOf(t));
	}

	template <class T, class Param>
	String TypesBase<T, Param>::typeName(nat id) const {
		if (id == NO_ID) return L"";
		return types[id].name;
	}

	template <class T, class Param>
	nat TypesBase<T, Param>::size() const {
		return types.size();
	}
}