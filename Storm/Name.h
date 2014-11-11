#pragma once

namespace storm {

	/**
	 * Representation of a name, either a relative name or an absolute
	 * name including the full package path.
	 */
	class Name : public Printable {
	public:
		// Path to the root package.
		Name();

		// Path to a named in the form 'a.b.c...'
		Name(const String &path);

		// Concat paths.
		Name operator +(const Name &o) const;
		Name &operator +=(const Name &o);

		// Equality.
		inline bool operator ==(const Name &o) const { return parts == o.parts; }
		inline bool operator !=(const Name &o) const { return parts != o.parts; }

		// Ordering.
		inline bool operator <(const Name &o) const { return parts < o.parts; }
		inline bool operator >(const Name &o) const { return parts > o.parts; }

		// Get the parent.
		Name parent() const;

		// Last element.
		String last() const;

		// All elements from 'n'.
		Name from(nat n) const;

		// Is this the root package?
		inline bool root() const { return size() == 0; }

		// Access to individual elements.
		inline nat size() const { return parts.size(); }
		inline const String &operator [](nat id) const { return parts[id]; }

		// empty/any
		inline bool any() const { return size() > 0; }
		inline bool empty() const { return size() == 0; }

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Store each part.
		vector<String> parts;
	};

}

namespace stdext {

	inline size_t hash_value(const storm::Name &n) {
		// djb2 hash
		size_t r = 5381;
		for (nat i = 0; i < n.size(); i++) {
			const String &s = n[i];
			for (nat j = 0; j < s.size(); j++)
				r = ((r << 5) + r) + s[j];
		}
		return r;
	}

}
