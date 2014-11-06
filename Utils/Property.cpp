#include "StdAfx.h"
#include "Property.h"

Property::Property(const Property &o) : name(o.name), data(null), type(o.type), edit(o.edit), onUpdate(o.onUpdate) {
	if (o.data) data = o.data->clone();
}

Property &Property::operator =(const Property &o) {
	name = o.name;
	del(data);
	if (o.data) data = o.data->clone();
	type = o.type;
	edit = o.edit;
	onUpdate = o.onUpdate;
	return *this;
}

Property::~Property() {
	delete data;
}

template <class T>
void Property::genericSetter(Type assumedType, const T &value) {
	verifyType(assumedType);
	Value<T> *v = (Value<T> *)data;
	*v = value;
	onUpdate();
}

void Property::setBool(bool v) {
	genericSetter(tBool, v);
}

void Property::setNat(nat v) {
	genericSetter(tNat, v);
}

void Property::setInt(int v) {
	genericSetter(tInt, v);
}

void Property::setFloat(float v) {
	genericSetter(tFloat, v);
}

void Property::setString(const String &v) {
	genericSetter(tString, v);
}

void Property::setColor(const Color &v) {
	genericSetter(tColor, v);
}

template <class T>
T Property::genericGetter(Type assumedType) const {
	verifyType(assumedType);
	Value<T> *v = (Value<T> *)data;
	return *v;
}

bool Property::getBool() const {
	return genericGetter<bool>(tBool);
}

int Property::getInt() const {
	return genericGetter<int>(tInt);
}

nat Property::getNat() const {
	return genericGetter<nat>(tNat);
}

float Property::getFloat() const {
	return genericGetter<float>(tFloat);
}

String Property::getString() const {
	return genericGetter<String>(tString);
}

Color Property::getColor() const {
	return genericGetter<Color>(tColor);
}
