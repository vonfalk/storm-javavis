#pragma once
#include "NameSet.h"
#include "TypeChain.h"
#include "Gc.h"
#include "RunOn.h"
#include "RefHandle.h"
#include "Layout.h"
#include "VTable.h"
#include "Core/TypeFlags.h"

namespace storm {
	STORM_PKG(core.lang);

	// Description of a type in C++. Found in CppTypes.h
	struct CppType;
	class NamedThread;
	class Function;
	class OverridePart;

	/**
	 * Description of a type.
	 */
	class Type : public NameSet {
		STORM_CLASS;

		// Let allocation access gcType.
		friend void *runtime::allocObject(size_t s, Type *t);

	public:
		// Create a type declared in Storm.
		STORM_CTOR Type(Str *name, TypeFlags flags);
		STORM_CTOR Type(Str *name, Array<Value> *params, TypeFlags flags);

		// Create a type declared in C++.
		Type(Str *name, TypeFlags flags, Size size, GcType *gcType, const void *vtable);
		Type(Str *name, Array<Value> *params, TypeFlags flags, Size size, GcType *gcType, const void *vtable);

		// Destroy our resources.
		~Type();

		// Owning engine. Must be the first data member of this class!
		Engine &engine;

		// Make a GcType that describes an object inheriting from 'type'.
		static GcType *makeType(Engine &e, const GcType *type);

		// Create the type for Type (as this is special).
		static Type *createType(Engine &e, const CppType *type);

		// Set this type onto another object. This can *not* be used to trick the system into
		// resizing the object. This is only used once during startup.
		void setType(Object *object) const;

		// Set the super-class for this type.
		void STORM_FN setSuper(MAYBE(Type *) to);

		// Get the super-class for this type.
		inline MAYBE(Type *) STORM_FN super() { return chain ? chain->super() : null; }

		// Set the thread for this type. This will force the super-type to be TObject.
		void STORM_FN setThread(NamedThread *t);

		// Get where we want to run.
		RunOn STORM_FN runOn();

		// Keep track of what is added.
		virtual void STORM_FN add(Named *item);

		// Modify the 'find' behaviour slightly so it also considers superclasses.
		virtual MAYBE(Named *) STORM_FN find(SimplePart *part);
		using NameSet::find;

		// Receive notification of new additions.
		virtual void STORM_FN notifyAdded(NameSet *to, Named *what);

		/**
		 * These functions are safe to call from any thread.
		 */

		// Get a handle for this type.
		const Handle &CODECALL handle();

		// Get the GcType for instances of this object. Lazily creates the information if it is
		// needed. For values, this is the same as gcArrayType.
		const GcType *CODECALL gcType();

		// Get the raw GcType for arrays of this type. Usually, the handle is preferred, but
		// sometimes in early boot that is not possible.
		const GcType *CODECALL gcArrayType();

		// Get this class's size. (NOTE: This function is safe to call from any thread).
		Size STORM_FN size();

		/**
		 * The threadsafe part ends here.
		 */

		// If the type was initialized during early boot, we need to initialize vtables through here
		// at a suitable time, before any Storm-defined types are created.
		void vtableInit(const void *vtable);

		// Called by VTable when our vtable needs to grow the allocated space for the parent class.
		void vtableGrow(Nat pos, Nat count);

		// Late initialization.
		virtual void lateInit();

		// Force layout of all member variables.
		void STORM_FN doLayout();

		// Inheritance chain and membership lookup. TODO: Make private?
		TypeChain *chain;

		// Helpers for the chain.
		inline Bool STORM_FN isA(const Type *o) const { return chain->isA(o); }
		inline Int STORM_FN distanceFrom(const Type *from) const { return chain->distance(from); }

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Flags for this type.
		const TypeFlags typeFlags;

		// What kind of type is this type?
		virtual BasicTypeInfo::Kind builtInType() const;

		// Names for constructors and destructors.
		static const wchar *CTOR;
		static const wchar *DTOR;

	private:
		// Special constructor for creating the first type.
		Type(Engine &e, TypeFlags flags, Size size, GcType *gcType);

		// The description of the type we maintain for the GC. If we're a value type,
		// this will have 'kind' set to 'tArray'.
		// Note: the type member of the GcType this is pointing to must never be changed after this
		// member is changed. Otherwise we confuse the GC.
		GcType *myGcType;

		// Handle (lazily created). If we're a value, this will be a RefHandle.
		const Handle *tHandle;

		// The content we're using for all references in the handle. TODO: Place it inside a RefSource?
		code::Content *handleContent;

		enum {
			toSFound,
			toSMissing,
			toSNoParent,
		};

		// How is the update of the toS function going?
		Nat handleToS;

		// Thread we should be running on if we indirectly inherit from TObject.
		NamedThread *useThread;

		// Our size (including base classes). If zero, we need to re-compute it.
		Size mySize;

		// Compute the size of our super-class.
		Size superSize();

		// Get the default super type for us.
		Type *defaultSuper() const;

		// Special case for the first Type.
		static void *operator new(size_t size, Engine &e, GcType *type);
		static void operator delete(void *mem, Engine &e, GcType *type);

		// Common initialization. For C++ types, passes the vtable. May be null.
		void init(const void *vtable);

		// Is this a value type?
		inline bool value() const { return (typeFlags & typeValue) == typeValue; }

		// Generate a handle for this type.
		void buildHandle();

		// Update the handle with the potentially relevant function 'fn'.
		void updateHandle(Function *fn);

		// Update the toS function in the handle if neccessary.
		void updateHandleToS(bool first, Function *newFn);

		// Notify that the thread changed.
		void notifyThread(NamedThread *t);

		// The member variable layout for this type. Not used for types declared in C++.
		Layout *layout;

		// Find only in this class.
		MAYBE(Named *) findHere(SimplePart *part);

		// The VTable for this class. Value types do not have a vtable, so 'vtable' is null for a value type.
		VTable *vtable;

		/**
		 * Helpers for deciding which functions shall be virtual.
		 */

		// Called when a function has been added here. Decides if 'f' should be virtual or not and
		// acts accordingly.
		void vtableFnAdded(Function *added);

		// Search towards the super class for a function overridden by 'fn'. Returns 'true' if an
		// overridden function was found.
		Bool vtableInsertSuper(Function *added);

		// Search towards the subclasses for an overriding function and make sure to insert it into
		// the vtable in 'slot'. Assuming 'parentFn' is the corresponding function in the closest
		// parent.
		void vtableInsertSubclasses(OverridePart *added);

		// Search towards the super class for an overriden function. Returns the most specialized match found.
		Function *vtableFindSuper(OverridePart *fn);

		// Search towards the subclasses for an overriding function. Returns 'true' if one is found.
		Bool vtableFindSubclass(OverridePart *fn);

		// Called by a child class to notify that a new function 'added' has been added in some
		// child class. If one is found in this class or any parent classes, that function is made
		// into a VTable-call (if it is not already) and its slot is returned. If none is found,
		// returns an invalid slot.
		VTableSlot vtableNewChildFn(OverridePart *added);

		// Make 'f' use the vtable when called.
		void vtableUse(Function *fn, VTableSlot slot);

		// Make 'f' not use the vtable when called.
		void vtableClear(Function *fn);

		// Insert 'f' into the vtable at 'slot'.
		void vtableInsert(Function *fn, VTableSlot slot);

		// Clear slot 'n' of the vtable.
		void vtableClear(VTableSlot slot);

		// Called when we have been attached to a new parent.
		void vtableNewSuper();

		// Called when one of our children has been removed.
		void vtableChildRemoved(Type *lost);

		// Called when our parent's vtable grew.
		void vtableParentGrown(Nat pos, Nat count);

	};


	/**
	 * Lookup used for finding functions in superclasses which 'match' overrides.
	 */
	class OverridePart : public SimplePart {
		STORM_CLASS;
	public:
		// Create, specifying the function used.
		STORM_CTOR OverridePart(Function *match);

		// Create, specifying the function used and a new owning class.
		STORM_CTOR OverridePart(Type *parent, Function *match);

		// Custom badness measure.
		virtual Int STORM_FN matches(Named *candidate) const;

	private:
		// Remember the result as well.
		Value result;
	};

}
