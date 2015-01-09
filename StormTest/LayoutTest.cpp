#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/TypeVar.h"
#include "Storm/TypeLayout.h"

static TypeVar *var(Engine &e, Type *type, const String &name) {
	return CREATE(TypeVar, e, intType(e), type, name);
}

static void debug(const TypeLayout &l) {
	l.format(std::wcout);
	l.format(std::wcout, Size::sByte);
	l.format(std::wcout, Size::sInt);
	l.format(std::wcout, Size::sPtr);
}

#define defVar(name, type) Auto<TypeVar> _##name = var(e, type, _T(#name)); TypeVar *name = _##name.borrow()

BEGIN_TEST(LayoutTest) {
	Engine e(Path::executable());

	defVar(bool1, boolType(e));
	defVar(bool2, boolType(e));
	defVar(bool3, boolType(e));
	defVar(bool4, boolType(e));
	defVar(int1, intType(e));
	defVar(int2, intType(e));
	defVar(int3, intType(e));
	defVar(int4, intType(e));
	defVar(obj1, Str::type(e));
	defVar(obj2, Str::type(e));
	defVar(obj3, Str::type(e));
	defVar(obj4, Str::type(e));

	{
		TypeLayout layout;
		layout.add(bool1);
		layout.add(int1);
		layout.add(obj1);

		CHECK_EQ(layout.offset(Size(), bool1), Offset());
		CHECK_EQ(layout.offset(Size(), int1), Offset::sInt);
		CHECK_EQ(layout.offset(Size(), obj1), Offset::sInt * 2);

		CHECK_EQ(layout.offset(Size::sInt, bool1), Offset::sPtr);
		CHECK_EQ(layout.offset(Size::sInt, int1), Offset::sPtr + Offset::sInt);
		CHECK_EQ(layout.offset(Size::sInt, obj1), Offset::sPtr + Offset::sInt * 2);

		CHECK_EQ(layout.offset(Size::sPtr, bool1), Offset::sPtr);
		CHECK_EQ(layout.offset(Size::sPtr, int1), Offset::sPtr + Offset::sInt);
		CHECK_EQ(layout.offset(Size::sPtr, obj1), Offset::sPtr + Offset::sInt * 2);
	}


	{
		TypeLayout layout;
		layout.add(bool1);
		layout.add(bool2);
		layout.add(bool3);
		layout.add(bool4);
		layout.add(int1);

		CHECK_EQ(layout.offset(Size(), bool1), Offset());
		CHECK_EQ(layout.offset(Size(), bool2), Offset::sByte);
		CHECK_EQ(layout.offset(Size(), bool3), Offset::sByte * 2);
		CHECK_EQ(layout.offset(Size(), bool4), Offset::sByte * 3);
		CHECK_EQ(layout.offset(Size(), int1), Offset::sInt);
	}

	{
		TypeLayout layout;
		layout.add(bool1);
		layout.add(bool2);
		layout.add(bool3);
		layout.add(bool4);
		layout.add(obj1);

		CHECK_EQ(layout.offset(Size(), bool1), Offset());
		CHECK_EQ(layout.offset(Size(), bool2), Offset::sByte);
		CHECK_EQ(layout.offset(Size(), bool3), Offset::sByte * 2);
		CHECK_EQ(layout.offset(Size(), bool4), Offset::sByte * 3);
		CHECK_EQ(layout.offset(Size(), obj1), Offset::sPtr);
	}

	{
		TypeLayout layout;
		layout.add(int1);
		layout.add(int2);
		layout.add(obj1);
		layout.add(obj2);

		CHECK_EQ(layout.offset(Size(), int1), Offset());
		CHECK_EQ(layout.offset(Size(), int2), Offset::sInt);
		CHECK_EQ(layout.offset(Size(), obj1), Offset::sInt * 2);
		CHECK_EQ(layout.offset(Size(), obj2), Offset::sInt * 2 + Offset::sPtr);
	}

} END_TEST
