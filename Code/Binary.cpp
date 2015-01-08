#include "StdAfx.h"
#include "Binary.h"
#include "MachineCode.h"

namespace code {

	Binary::Binary(Arena &arena, const String &title, const Listing &listing)
		: title(title), arena(arena), memory(null), size(0) {
		try {
			set(listing);
		} catch (...) {
			clear(references);
			clear(blocks);
			arena.codeFree(memory);
			throw;
		}
	}

	Binary::~Binary() {
		clear(references);
		clear(blocks);
		arena.codeFree(memory);
	}

	void Binary::update(RefSource &ref) {
		ref.set(memory, size);
	}

	Binary::Block *Binary::Block::create(const Frame &frame, const code::Block &b) {
		vector<Variable> vars = frame.variables(b);

		nat size = sizeof(Block) + sizeof(Var)*(vars.size() - 1);
		Block *c = (Block *)::operator new (size);
		// Block *c = (Block *)new byte[size];

		c->parent = frame.parent(b).getId();
		c->variables = vars.size();

		for (nat i = 0; i < vars.size(); i++) {
			Var v = { vars[i].getId(), vars[i].size() };
			c->variable[i] = v;
		}

		return c;
	}

	void Binary::updateReferences(void *addr, const Output &from) {
		vector<Reference *> references;

		byte *baseAddr = (byte *)addr;

		for (nat i = 0; i < from.absoluteRefs.size(); i++) {
			const Output::UsedRef &r = from.absoluteRefs[i];
			references.push_back(new BinaryUpdater(r.reference, title, (cpuNat *)(baseAddr + r.offset), false));
		}

		for (nat i = 0; i < from.relativeRefs.size(); i++) {
			const Output::UsedRef &r = from.relativeRefs[i];
			references.push_back(new BinaryUpdater(r.reference, title, (cpuNat *)(baseAddr + r.offset), true));
		}

		std::swap(references, this->references);
		clear(references);
	}

	void Binary::updateBlocks(const Frame &from) {
		vector<code::Block> blocks = from.allBlocks();
		vector<Block *> r(blocks.size());

		for (nat i = 0; i < blocks.size(); i++) {
			code::Block b = blocks[i];

			r[i] = Block::create(from, b);
		}

		std::swap(r, this->blocks);
		clear(r);
	}

	void Binary::set(const Listing &listing) {
		void *oldMem = memory;
		nat oldSize = size;

		Listing transformed = machine::transform(listing, this);
		// PLN("After transformation:" << transformed);

		// Calculate size and positions of labels and other important positions...
		SizeOutput sizeOutput(transformed.getLabelCount());
		machine::output(sizeOutput, arena, transformed);

		size = sizeOutput.tell();
		memory = arena.codeAlloc(size);

		try {
			MemoryOutput memOutput(memory, sizeOutput);
			machine::output(memOutput, arena, transformed);

			// Update our list of references
			updateReferences(memory, memOutput);
			updateBlocks(transformed.frame);
			metadata = (machine::FnMeta *)memOutput.lookup(transformed.metadata());
		} catch (...) {
			arena.codeFree(memory);
			memory = oldMem;
			size = oldSize;
			PLN("WERE FUCKED");
			throw;
		}

		arena.codeFree(oldMem);
	}

	const void *Binary::getData() const {
		return memory;
	}

	void Binary::destroyFrame(const machine::StackFrame &frame) const {
		nat block = machine::activeBlock(frame, metadata);
		assert(block < blocks.size());

		while (block < blocks.size()) {
			Block *b = blocks[block];

			for (nat i = 0; i < b->variables; i++) {
				destroyVariable(frame, b->variable[i]);
			}

			block = b->parent;
		}
	}

	template <class Param>
	static inline void callFn(void *fn, void *param) {
		typedef void (CODECALL *Fn)(Param);
		Fn ptr = (Fn)fn;
		Param *p = (Param *)param;
		(*ptr)(*p);
	}

	void Binary::destroyVariable(const machine::StackFrame &frame, Var var) const {
		machine::VarInfo info = machine::variableInfo(frame, metadata, var.id);

		if (info.freeFn == null)
			return;

		switch (var.size.current()) {
			case 1:
				callFn<Byte>(info.freeFn, info.ptr);
				break;
			case 4:
				callFn<Int>(info.freeFn, info.ptr);
				break;
			case 8:
				assert(false); // Not implemented yet!
				break;
		}
	}

	template <class T>
	static inline T read(machine::StackFrame &frame, machine::FnMeta *meta, Variable v) {
		T *addr = (T *)machine::variableInfo(frame, meta, v).ptr;
		return *addr;
	}

	Byte Binary::readByte(machine::StackFrame &frame, Variable v) {
		return read<Byte>(frame, metadata, v);
	}

	Int Binary::readInt(machine::StackFrame &frame, Variable v) {
		return read<Int>(frame, metadata, v);
	}

	Long Binary::readLong(machine::StackFrame &frame, Variable v) {
		return read<Long>(frame, metadata, v);
	}

	void *Binary::readPtr(machine::StackFrame &frame, Variable v) {
		return read<void *>(frame, metadata, v);
	}


	BinaryUpdater::BinaryUpdater(const Ref &ref, const String &title, cpuNat *at, bool relative) : Reference(ref, title), at(at), relative(relative) {}

	void BinaryUpdater::onAddressChanged(void *newAddress) {
		Reference::onAddressChanged(newAddress);

		cpuNat addr = (cpuNat)newAddress;
		if (relative) {
			addr -= cpuNat(at) + sizeof(cpuNat);
			*at = addr;
		} else {
			*at = addr;
		}
	}
}
