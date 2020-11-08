#pragma once

namespace gui {

	/**
	 * An object that keeps track of all identifiers in the system.
	 *
	 * This class is managed through the RenderMgr class. It is therefore written in a C++ style.
	 *
	 * Due to how Resource objects keep track of any native resources, this allocation attempts to
	 * keep indices low and lumped together whenever possible. This property is prioritized above
	 * allocation speed, as creating Graphics objects is expected to be fairly expensive anyway.
	 */
	class IdMgr {
	public:
		// Create.
		IdMgr();

		// Destroy.
		~IdMgr();

		// Allocate an identifier. Identifier 0 is reserved.
		Nat alloc();

		// Free an identifier.
		void free(Nat id);

	private:
		// Data. We use one bit for each identity.
		Nat *data;

		// Number of elements in 'data'.
		Nat count;

		// Allocate inside a single Nat.
		Nat allocate(Nat &in);

		// Grow the data.
		void grow();
	};

}
