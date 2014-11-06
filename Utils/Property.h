#pragma once

#include "Function.h"
#include "Color.h"
#include "Value.h"

// Class for a single property in a class. Used for dynamic
// changing of certain properties by the framework.
class Property {
public:
	// The type of this specific property. These shall
	// remain constant between different versions of the
	// software. Otherwise reading back data from files will fail.
	enum Type {
		// No data, just a placeholder.
		tNone = 0x0,

		tBool = 0x01,

		// Integer types.
		tInt = 0x02, tNat,

		// Float types.
		tFloat = 0x10,

		// Strings
		tString = 0x20,

		// Color
		tColor = 0x30,
	};

	// How to edit this property (preferably). Each of the types are given a default
	// edit property. However, this can be changed. Note that not all types apply for
	// all types.
	enum Edit {
		// No editing (only for tNone).
		eNone,

		// Checkbox (only for tBool).
		eCheck,

		// Simple text box, default for integer, float and string types.
		eText,

		// Color chooser (only for tColor)
		eColor,
	};

	// Create a property with a string as well as a pointer to the data.
	Property(const String &name) : name(name), data(null), type(tNone), edit(eNone) {}
	Property(const String &name, Value<bool> *d) : name(name), data(d), type(tBool), edit(eCheck) {}
	Property(const String &name, Value<nat> *d) : name(name), data(d), type(tInt), edit(eText) {}
	Property(const String &name, Value<int> *d) : name(name), data(d), type(tNat), edit(eText) {}
	Property(const String &name, Value<float> *d) : name(name), data(d), type(tFloat), edit(eText) {}
	Property(const String &name, Value<String> *d) : name(name), data(d), type(tString), edit(eText) {}
	Property(const String &name, Value<Color> *d) : name(name), data(d), type(tColor), edit(eColor) {}

	Property(const Property &o);
	Property &operator =(const Property &o);
	~Property();

	// Callback to be called when this property is updated.
	Fn<void> onUpdate;

	// Get the name.
	const String &getName() const { return name; }
	void setName(const String &name) { this->name = name; }

	// Get the type.
	inline Type getType() const { return type; }

	// Get the editing type.
	inline Edit getEdit() const { return edit; }

	// TODO: Provide means of altering the editing method.

	// Get data.
	bool getBool() const;
	int getInt() const;
	nat getNat() const;
	float getFloat() const;
	String getString() const;
	Color getColor() const;

	// Set data.
	void setBool(bool v);
	void setInt(int v);
	void setNat(nat v);
	void setFloat(float v);
	void setString(const String &v);
	void setColor(const Color &v);
private:
	String name;
	ValueBase *data;
	Type type;
	Edit edit;

	// Verify our type.
	void verifyType(Type t) const { assert(getType() == t); };

	// Generic getter and setter
	template <class T>
	T genericGetter(Type assumedType) const;
	template <class T>
	void genericSetter(Type assumedType, const T &value);
};


