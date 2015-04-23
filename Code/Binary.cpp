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
			clear(parts);
			arena.codeFree(memory);
			throw;
		}
	}

	Binary::~Binary() {
		clear(references);
		clear(parts);
		arena.codeFree(memory);
	}

	void Binary::update(RefSource &ref) {
		ref.set(memory, size);
	}

	Binary::Part *Binary::Part::create(const Frame &frame, const code::Part &p) {
		vector<Variable> vars = frame.variables(p);

		nat size = sizeof(Part) + sizeof(Var)*(vars.size() - 1);
		Part *c = (Part *)::operator new (size);
		// Part *c = (Part *)new byte[size];

		c->parent = frame.prev(p).getId();
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

	void Binary::updateParts(const Frame &from) {
		vector<code::Part> parts = from.allParts();
		vector<Part *> r(parts.size());

		for (nat i = 0; i < parts.size(); i++) {
			code::Part p = parts[i];
			r[i] = Part::create(from, p);
		}

		std::swap(r, this->parts);
		clear(r);
	}

	void Binary::dbg_dump() {
		wostream &to = std::wcout;

		for (nat i = 0; i < parts.size(); i++) {
			to << L"Part " << i << L":" << endl;
			Part *p = parts[i];
			to << L"parent=" << p->parent << endl;

			Indent z(to);
			for (nat i = 0; i < p->variables; i++) {
				to << L"variable" << i << L"=" << p->variable[i].id << L", " << p->variable[i].size << endl;
			}
		}
	}

	void Binary::dbg_clearReferences() {
		// For use when delaying removal of code, can be dangerous!
		clear(references);
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
			updateParts(transformed.frame);
			metadata = (machine::FnMeta *)memOutput.lookup(transformed.metadata());
		} catch (...) {
			arena.codeFree(memory);
			memory = oldMem;
			size = oldSize;
			throw;
		}

		arena.codeFree(oldMem);
	}

	const void *Binary::getData() const {
		return memory;
	}

	void Binary::destroyFrame(const machine::StackFrame &frame) const {
		nat part = machine::activePart(frame, metadata);
		assert(part < parts.size(), "Invalid block id");

		while (part < parts.size()) {
			Part *b = parts[part];

			for (nat i = 0; i < b->variables; i++) {
				destroyVariable(frame, b->variable[i]);
			}

			part = b->parent;
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

		if ((info.freeOpt & freeOnException) == 0)
			return;

		if (info.freeOpt & freePtr) {
			typedef void (CODECALL *FreeFn)(void *);
			FreeFn f = (FreeFn)info.freeFn;
			(*f)(info.ptr);
		} else {
			switch (var.size.current()) {
			case 1:
				callFn<Byte>(info.freeFn, info.ptr);
				break;
			case 4:
				callFn<Int>(info.freeFn, info.ptr);
				break;
			case 8:
				callFn<Long>(info.freeFn, info.ptr);
				break;
			default:
				assert(false, "By-value destruction of values larger than 8 bytes is not supported.");
				break;
			}
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


	BinaryUpdater::BinaryUpdater(const Ref &ref, const String &title, cpuNat *at, bool relative)
		: Reference(ref, title), at(at), relative(relative) {}

	void BinaryUpdater::onAddressChanged(void *newAddress) {
		Reference::onAddressChanged(newAddress);

		cpuNat addr = (cpuNat)newAddress;
		if (relative)
			addr -= cpuNat(at) + sizeof(cpuNat);
		unalignedAtomicWrite(*at, addr);
	}
}
