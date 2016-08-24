#include "stdafx.h"
#include "Listing.h"
#include "Core/StrBuf.h"

namespace code {

	static const nat invalid = -1;

	wostream &operator <<(wostream &to, FreeOpt o) {
		if ((o & freeOnBoth) == freeOnBoth)
			to << L"always";
		else if (o & freeOnBlockExit)
			to << L"on block exit";
		else if (o & freeOnException)
			to << L"on exception";

		if (o & freePtr)
			to << L", by ptr";
		else
			to << L", by value";

		return to;
	}

	StrBuf &operator <<(StrBuf &to, FreeOpt o) {
		if ((o & freeOnBoth) == freeOnBoth)
			to << L"always";
		else if (o & freeOnBlockExit)
			to << L"on block exit";
		else if (o & freeOnException)
			to << L"on exception";

		if (o & freePtr)
			to << L", by ptr";
		else
			to << L", by value";

		return to;
	}

	Listing::Entry::Entry() : instr(null), labels(null) {}

	Listing::Entry::Entry(const Entry &o) {
		instr = o.instr;
		labels = o.labels;
	}

	Listing::Entry::Entry(Instr *i) : instr(i), labels(null) {}

	Listing::Entry::Entry(Engine &e, Label l) : instr(null) {
		labels = new (e) Array<Label>(l);
	}

	void Listing::Entry::deepCopy(CloneEnv *env) {
		// Instr is immutable. No need to do anything with that!

		if (labels)
			// No need to call 'deepCopy' here, as labels are simple enough.
			labels = new (labels) Array<Label>(labels);
	}

	void Listing::Entry::add(Engine &e, Label l) {
		if (!labels)
			labels = new (e) Array<Label>(l);
		else
			labels->push(l);
	}

	StrBuf &STORM_FN operator <<(StrBuf &to, Listing::Entry e) {
		if (e.labels != null && e.labels->any()) {
			StrBuf *tmp = new (&to) StrBuf();
			*tmp << e.labels->at(0);

			for (nat i = 1; i < e.labels->count(); i++)
				*tmp << L", " << width(3) << e.labels->at(i);

			*tmp << L": ";

			to << width(20) << tmp->toS();
		} else {
			to << width(20) << L" ";
		}

		if (e.instr)
			to << e.instr;

		return to;
	}


	/**
	 * Internal storage.
	 */

	Listing::IVar::IVar(Nat parent, Size size, Bool isFloat, Operand freeFn, FreeOpt opt) :
		parent(parent), size(size), isFloat(isFloat), freeFn(freeFn), freeOpt(opt) {}

	Listing::IBlock::IBlock(Engine &e) : parts(new (e) Array<Nat>()), parent(Block().id) {}

	Listing::IBlock::IBlock(Engine &e, Nat parent) : parts(new (e) Array<Nat>()), parent(parent) {}

	void Listing::IBlock::deepCopy(CloneEnv *env) {
		parts = new (parts) Array<Nat>(parts);
	}

	Listing::IPart::IPart(Engine &e, Nat block, Nat id) : vars(new (e) Array<Nat>()), block(block), index(id) {}

	void Listing::IPart::deepCopy(CloneEnv *env) {
		vars = new (vars) Array<Nat>(vars);
	}

	/**
	 * Listing.
	 */

	Listing::Listing() :
		code(new (engine()) Array<Entry>()),
		nextLabel(1),
		vars(new (engine()) Array<IVar>()),
		blocks(new (engine()) Array<IBlock>()),
		parts(new (engine()) Array<IPart>()) {

		// Create the root block.
		IBlock root(engine());
		root.parts->push(0);
		blocks->push(root);
		parts->push(IPart(engine(), 0, 0));
	}

	void Listing::deepCopy(CloneEnv *env) {
		code = new (this) Array<Entry>(code);
		code->deepCopy(env);
		vars = new (this) Array<IVar>(vars);
		vars->deepCopy(env);
		blocks = new (this) Array<IBlock>(blocks);
		blocks->deepCopy(env);
		parts = new (this) Array<IPart>(parts);
		parts->deepCopy(env);
	}

	Listing &Listing::operator <<(Instr *i) {
		if (code->empty()) {
			*code << Entry(i);
		} else if (code->last().instr) {
			*code << Entry(i);
		} else {
			code->last().instr = i;
		}
		return *this;
	}

	Listing &Listing::operator <<(Label l) {
		if (code->empty()) {
			*code << Entry(engine(), l);
		} else if (code->last().instr) {
			*code << Entry(engine(), l);
		} else {
			code->last().add(engine(), l);
		}
		return *this;
	}

	Label Listing::meta() {
		return Label(0);
	}

	Label Listing::label() {
		return Label(nextLabel++);
	}

	Block Listing::root() {
		return Block(0);
	}

	Block Listing::createBlock(Part parent) {
		if (parent.id >= parts->count())
			return Block();

		Nat blockId = blocks->count();

		IBlock b(engine(), parent.id);
		b.parts->push(parts->count());
		blocks->push(b);

		Nat partId = parts->count();
		parts->push(IPart(engine(), blockId, 0));

		return Block(partId);
	}

	Part Listing::createPart(Part in) {
		if (in.id >= parts->count())
			return Part(invalid);

		Nat blockId = findBlock(in.id);
		IBlock &block = blocks->at(blockId);
		Nat id = parts->count();
		parts->push(IPart(engine(), blockId, block.parts->count()));
		block.parts->push(id);
		return Part(id);
	}

	Part Listing::next(Part p) const {
		if (p.id >= parts->count())
			return Part(invalid);

		IPart &part = parts->at(p.id);
		IBlock &block = blocks->at(part.block);
		if (part.index + 1 >= block.parts->count())
			return Part(invalid);
		return Part(block.parts->at(part.index + 1));
	}

	Part Listing::prev(Part p) const {
		if (p.id >= parts->count())
			return Part(invalid);

		IPart &part = parts->at(p.id);
		IBlock &block = blocks->at(part.block);
		if (part.index < 1)
			return Part(invalid);
		return Part(block.parts->at(part.index - 1));
	}

	Part Listing::last(Part p) const {
		if (p.id >= parts->count())
			return Part(invalid);

		IBlock &block = blocks->at(findBlock(p.id));
		return Part(block.parts->last());
	}

	Block Listing::first(Part p) const {
		if (p.id >= parts->count())
			return Block(invalid);

		IBlock &block = blocks->at(findBlock(p.id));
		return Block(block.parts->first());
	}

	void Listing::delay(Variable v, Part to) {
		TODO(L"Implement me!");
	}

	Variable Listing::createVar(Part in, Size size, Operand free, FreeOpt when) {
		assert(in.id != invalid, L"No such part!");

		PLN(L"Adding var to " << in.id);
		Nat id = vars->count();
		vars->push(IVar(in.id, size, false, free, when));

		IPart &part = parts->at(in.id);
		part.vars->push(id);

		return Variable(id, size);
	}

	Nat Listing::findBlock(Nat partId) const {
		IPart &p = parts->at(partId);
		return p.block;
	}

	void Listing::toS(StrBuf *to) const {
		// TODO: Output parameters.
		putBlock(*to, 0);

		for (nat i = 0; i < code->count(); i++) {
			*to << L"\n" << code->at(i);
		}
	}

	void Listing::putBlock(StrBuf &to, Nat block) const {
		to << L"Block " << block << L":\n";
		storm::Indent z(&to);

		Nat blockId = findBlock(block);
		IBlock &b = blocks->at(blockId);
		for (nat i = 0; i < b.parts->count(); i++) {
			putPart(to, b.parts->at(i), i != 0);
		}

		// Put all child blocks as well.
		for (nat i = 0; i < blocks->count(); i++) {
			IBlock &block = blocks->at(i);
			if (block.parent == blockId)
				putBlock(to, block.parts->at(0));
		}
	}

	void Listing::putPart(StrBuf &to, Nat part, Bool header) const {
		if (header) {
			to << L"--- Part " << part << L" ---\n";
		}

		IPart &p = parts->at(part);
		for (nat i = 0; i < p.vars->count(); i++) {
			putVar(to, p.vars->at(i));
		}
	}

	void Listing::putVar(StrBuf &to, Nat var) const {
		IVar &v = vars->at(var);
		to << width(3) << var << L": " << v.size << L" " << v.freeOpt << L"\n";
	}

}
