#pragma once

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Represents a block of code in a listing. These blocks contain a set of local variables, along
	 * with parent-child relations. This information will be available in the final Binary object as
	 * well (but in another format).
	 */
	class Block {
		STORM_VALUE;
	public:
		// Create an invalid block.
		STORM_CTOR Block();

		inline Bool STORM_FN operator ==(Block o) const { return id == o.id; }
		inline Bool STORM_FN operator !=(Block o) const { return id != o.id; }

	private:
		friend class Part;
		friend class Listing;

		friend wostream &operator <<(wostream &to, Block l);
		friend StrBuf &operator <<(StrBuf &to, Block l);

		explicit Block(Nat id);

		Nat id;
	};

	wostream &operator <<(wostream &to, Block l);
	StrBuf &STORM_FN operator <<(StrBuf &to, Block l);

	/**
	 * Represents a part of a block. These parts are a simple way of telling the runtime when to
	 * enable certain error handlers on variables in the block.
	 */
	class Part {
		STORM_VALUE;
	public:
		// Block ids are also part ids!
		STORM_CAST_CTOR Part(Block b);

		// Create an invalid part.
		STORM_CTOR Part();

		inline Bool STORM_FN operator ==(Part o) const { return id == o.id; }
		inline Bool STORM_FN operator !=(Part o) const { return id != o.id; }

		// Key.
		inline Nat STORM_FN key() const { return id; }

	private:
		friend class Listing;
		friend class Operand;

		friend wostream &operator <<(wostream &to, Part l);
		friend StrBuf &operator <<(StrBuf &to, Part l);

		explicit Part(Nat id);

		Nat id;
	};

	wostream &operator <<(wostream &to, Part l);
	StrBuf &STORM_FN operator <<(StrBuf &to, Part l);

}
