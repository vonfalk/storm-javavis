#include "StdAfx.h"
#include "Properties.h"

void Properties::add(const Property &p) {
	if (find(p.getName()) == NO_ID) {
		properties.push_back(p);

		Property &end = properties.back();
		if (end.onUpdate == Fn<void>()) {
			end.onUpdate = onUpdateFn;
		}
	} else {
		assert(FALSE);
	}
}

void Properties::add(const Properties &p) {
	for (nat i = 0; i < p.properties.size(); i++) {
		add(p.properties[i]);
	}
}

void Properties::add(const String &prefix, const Properties &p) {
	for (nat i = 0; i < p.properties.size(); i++) {
		Property prop = p.properties[i];
		prop.setName(prefix + prop.getName());
		add(prop);
	}
}

void Properties::add(const Properties &p, const String &suffix) {
	for (nat i = 0; i < p.properties.size(); i++) {
		Property prop = p.properties[i];
		prop.setName(prop.getName() + suffix);
		add(prop);
	}
}

void Properties::setOnUpdate(const Fn<void> &fn) {
	for (nat i = 0; i < properties.size(); i++) {
		if (properties[i].onUpdate == onUpdateFn) {
			properties[i].onUpdate = fn;
		}
	}
	onUpdateFn = fn;
}

nat Properties::find(const String &name) const {
	for (nat i = 0; i < properties.size(); i++) {
		if (properties[i].getName() == name) return i;
	}
	return NO_ID;
}

void Properties::load(Stream &from, Property *p) {
	Property::Type type = (Property::Type)from.read<int>();
	if (p && type != p->getType()) p = null;

	bool b; int i; nat n; float f; String s; Color c;
	switch (type) {
	case Property::tBool:
		b = from.read<bool>();
		if (p) p->setBool(b);
		break;
	case Property::tInt:
		i = from.read<int>();
		if (p) p->setInt(i);
		break;
	case Property::tNat:
		n = from.read<nat>();
		if (p) p->setNat(n);
		break;
	case Property::tFloat:
		f = from.read<float>();
		if (p) p->setFloat(f);
		break;
	case Property::tString:
		s = from.read<String>();
		if (p) p->setString(s);
		break;
	case Property::tColor:
		c = from.read<Color>();
		if (p) p->setColor(c);
	default:
		TODO("Implement the type " << type << "!");
		break;
	}
}

void Properties::save(Stream &to, const Property &p) {
	to.write<String>(p.getName());
	to.write<int>(p.getType());
	switch (p.getType()) {
	case Property::tBool:
		to.write<bool>(p.getBool());
		break;
	case Property::tInt:
		to.write<int>(p.getInt());
		break;
	case Property::tNat:
		to.write<nat>(p.getNat());
		break;
	case Property::tFloat:
		to.write<float>(p.getFloat());
		break;
	case Property::tString:
		to.write<String>(p.getString());
		break;
	case Property::tColor:
		to.write<Color>(p.getColor());
		break;
	default:
		TODO("Implement the type " << p.getType() << "!");
		break;
	}
}

void Properties::save(Stream &to) const {
	nat numProperties = properties.size();
	for (nat i = 0; i < properties.size(); i++) {
		if (properties[i].getType() == Property::tNone) numProperties--;
	}

	to.write<nat>(numProperties);
	for (nat i = 0; i < properties.size(); i++) {
		const Property &p = properties[i];
		if (p.getType() != Property::tNone) {
			save(to, p);
		}
	}
}

void Properties::load(Stream &from) {
	nat size = from.read<nat>();
	if (from.error()) return;

	for (nat i = 0; i < size; i++) {
		String name = from.read<String>();
		nat id = find(name);
		if (id != NO_ID) {
			load(from, &properties[id]);
		} else {
			load(from, null);
		}
	}
}


