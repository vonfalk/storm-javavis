#pragma once

namespace code {

	// A label in a listing. Created from the Listing object.
	class Label {
		friend class Listing;
		friend class Value;
		friend class Output;

	public:
		inline bool operator ==(const Label &o) const { return id == o.id; }
		inline bool operator !=(const Label &o) const { return id != o.id; }

		// To string.
		String toS() const;

	private:
		Label(nat id) : id(id) {}

		nat id;
	};

}
