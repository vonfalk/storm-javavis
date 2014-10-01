#pragma once

namespace storm {

	/**
	 * Helper for representing package 'paths'.
	 * Note: all package names are always lowercase. This is enforced
	 * by this class.
	 */
	class PkgPath : public Printable {
	public:
		// Path to the root package.
		PkgPath();

		// Path to a package in the form 'a.b.c...'
		PkgPath(const String &path);

		// Concat paths.
		PkgPath operator +(const PkgPath &o) const;
		PkgPath &operator +=(const PkgPath &o);

		// Equality.
		inline bool operator ==(const PkgPath &o) const { return parts == o.parts; }
		inline bool operator !=(const PkgPath &o) const { return parts != o.parts; }

		// Get the parent.
		PkgPath parent() const;

		// Is this the root package?
		inline bool root() const { return size() == 0; }

		// Access to individual elements.
		inline nat size() const { return parts.size(); }
		inline const String &operator [](nat id) const { return parts[id]; }

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Store each part.
		vector<String> parts;
	};

}
