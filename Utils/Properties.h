#pragma once

#include "Property.h"
#include "Stream.h"

#include <vector>

namespace util {

	// A collection of properties to be returned from an object.
	class Properties {
	public:
		// Add a property to this object. Asserts if duplicate name is added.
		void add(const Property &p);

		// Add a whole set of properties to this object. Asserts if duplicate names are added.
		void add(const Properties &p);
		void add(const String &prefix, const Properties &p);
		void add(const Properties &p, const String &suffix);

		// Set the callback on all unset properties, even not yet added ones.
		void setOnUpdate(const Function<void> &fn);
	
		// Save the properties to the stream.
		void save(util::Stream &to) const;

		// Load the properties from a stream. Missing properties will be silently ignored.
		void load(util::Stream &from);

		// Invalid id returned by find.
		static const nat NO_ID = -1;

		// Find a property by name.
		nat find(const String &name) const;

		// Get the number of elements.
		inline nat size() const { return properties.size(); }

		// Get property at index.
		inline Property &operator [](nat id) { return properties[id]; }
		inline const Property &operator [](nat id) const { return properties[id]; }
	private:
		// Helper for loading and saving a single property.
		static void load(util::Stream &from, Property *p);
		static void save(util::Stream &to, const Property &p);

		// All the properties.
		typedef vector<Property> Props;
		Props properties;

		// The current onUpdate function to use.
		Function<void> onUpdateFn;
	};
}