#include "stdafx.h"
#include "Listing.h"
#include "Exception.h"
#include "UsedRegs.h"
#include "Arena.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/CloneEnv.h"

namespace code {

	static const nat invalid = -1;

	wostream &operator <<(wostream &to, FreeOpt o) {
		if ((o & freeOnBoth) == freeOnBoth) {
			to << L"always";
		} else if (o & freeOnBlockExit) {
			to << L"on block exit";
		} else if (o & freeOnException) {
			to << L"on exception";
		} else {
			to << L"never";
			return to;
		}

		if (o & freePtr)
			to << L", by ptr";
		else
			to << L", by value";

		return to;
	}

	StrBuf &operator <<(StrBuf &to, FreeOpt o) {
		if ((o & freeOnBoth) == freeOnBoth) {
			to << L"always";
		} else if (o & freeOnBlockExit) {
			to << L"on block exit";
		} else if (o & freeOnException) {
			to << L"on exception";
		} else {
			to << L"never";
			return to;
		}

		if (o & freePtr)
			to << L", by ptr";
		else
			to << L", by value";

		return to;
	}

	Listing::Entry::Entry() : instr(null), labels(null) {}

	Listing::Entry::Entry(Instr *i) : instr(i), labels(null) {}

	Listing::Entry::Entry(Instr *i, Array<Label> *l) : instr(i), labels(l) {}

	void Listing::Entry::deepCopy(CloneEnv *env) {
		// Instr is immutable. No need to do anything with that!

		if (labels)
			// No need to call 'deepCopy' here, as labels are simple enough.
			labels = new (labels) Array<Label>(*labels);
	}

	void Listing::Entry::add(Engine &e, Label l) {
		if (l == Label())
			return;

		if (!labels)
			labels = new (e) Array<Label>(l);
		else
			labels->push(l);
	}

	StrBuf &operator <<(StrBuf &to, Listing::Entry e) {
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

	Listing::IVar::IVar(Nat parent, Size size, TypeDesc *param, Operand freeFn, FreeOpt opt) :
		parent(parent), size(size), param(param), freeFn(freeFn), freeOpt(opt) {}

	void Listing::IVar::deepCopy(CloneEnv *env) {
		// No need.
	}

	Listing::IBlock::IBlock(Engine &e) : parts(new (e) Array<Nat>()), parent(Block().id) {}

	Listing::IBlock::IBlock(Engine &e, Nat parent) : parts(new (e) Array<Nat>()), parent(parent) {}

	void Listing::IBlock::deepCopy(CloneEnv *env) {
		parts = new (parts) Array<Nat>(*parts);
	}

	Listing::IPart::IPart(Engine &e, Nat block, Nat id) : vars(new (e) Array<Nat>()), block(block), index(id) {}

	void Listing::IPart::deepCopy(CloneEnv *env) {
		vars = new (vars) Array<Nat>(*vars);
	}

	/**
	 * Listing.
	 */

	Listing::Listing() : arena(null) {
		init(false, new (this) PrimitiveDesc(Primitive()));
	}

	Listing::Listing(Bool member, TypeDesc *result) : arena(null) {
		init(member, result);
	}

	Listing::Listing(const Arena *arena) : arena(arena) {
		init(false, new (this) PrimitiveDesc(Primitive()));
	}

	Listing::Listing(const Arena *arena, Bool member, TypeDesc *result) : arena(arena) {
		init(member, result);
	}

	void Listing::init(Bool member, TypeDesc *result) {
		this->code = new (this) Array<Entry>();
		this->nextLabels = null;
		this->nextLabel = 1;
		this->params = new (this) Array<Nat>();
		this->vars = new (this) Array<IVar>();
		this->blocks = new (this) Array<IBlock>();
		this->parts = new (this) Array<IPart>();
		this->needEH = false;
		this->member = member;
		this->result = result;

		// Create the root block.
		IBlock root(engine());
		root.parts->push(0);
		blocks->push(root);
		parts->push(IPart(engine(), 0, 0));
	}

	Listing::Listing(const Listing &o) :
		result(o.result),
		member(o.member),
		arena(o.arena),
		code(o.code),
		nextLabels(o.nextLabels),
		params(o.params),
		vars(o.vars),
		blocks(o.blocks),
		parts(o.parts),
		needEH(o.needEH) {

		deepCopy(new (this) CloneEnv());
	}

	void Listing::deepCopy(CloneEnv *env) {
		cloned(result, env);
		code = new (this) Array<Entry>(*code);
		code->deepCopy(env);
		nextLabels = new (this) Array<Label>(*nextLabels);
		nextLabels->deepCopy(env);
		params = new (this) Array<Nat>(*params);
		params->deepCopy(env);
		vars = new (this) Array<IVar>(*vars);
		vars->deepCopy(env);
		blocks = new (this) Array<IBlock>(*blocks);
		blocks->deepCopy(env);
		parts = new (this) Array<IPart>(*parts);
		parts->deepCopy(env);
	}

	Listing *Listing::createShell(const Arena *arena) const {
		Listing *shell = arena ? new (this) Listing(arena) : new (this) Listing();

		shell->result = result;
		shell->member = member;
		shell->nextLabel = nextLabel;
		if (nextLabels)
			shell->nextLabels = new (this) Array<Label>(*nextLabels);
		shell->params = new (this) Array<Nat>(*params);
		shell->vars = new (this) Array<IVar>(*vars);
		shell->blocks = new (this) Array<IBlock>(*blocks);
		shell->parts = new (this) Array<IPart>(*parts);
		shell->needEH = needEH;

		// Note: we're doing this the hard way since deepCopy did not work properly at the time this was written.
		CloneEnv *env = new (this) CloneEnv();
		for (nat i = 0; i < shell->vars->count(); i++)
			shell->vars->at(i).deepCopy(env);

		for (nat i = 0; i < shell->blocks->count(); i++)
			shell->blocks->at(i).deepCopy(env);

		for (nat i = 0; i < shell->parts->count(); i++)
			shell->parts->at(i).deepCopy(env);

		return shell;
	}

	Listing &Listing::operator <<(Instr *i) {
		code->push(Entry(i, nextLabels));
		nextLabels = null;
		return *this;
	}

	Listing &Listing::operator <<(Label l) {
		if (!nextLabels)
			nextLabels = new (this) Array<Label>();
		nextLabels->push(l);
		return *this;
	}

	MAYBE(Array<Label> *) Listing::labels(Nat id) {
		if (id == count())
			return nextLabels;
		else
			return code->at(id).labels;
	}

	Label Listing::meta() {
		return Label(0);
	}

	Label Listing::label() {
		return Label(nextLabel++);
	}

	Block Listing::root() const {
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

	Nat Listing::findId(Array<Nat> *in, Nat val) {
		for (nat i = 0; i < in->count(); i++)
			if (in->at(i) == val)
				return i;
		return in->count();
	}

	Var Listing::createVar(Nat index) const {
		return Var(index, vars->at(index).size);
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

	Var Listing::prev(Var v) const {
		if (v.id >= vars->count())
			return Var();

		if (isParam(v)) {
			// Find the previous parameter:
			Nat id = findId(params, v.id);
			if (id > 0 && id < params->count())
				return createVar(params->at(id - 1));
			else
				return Var();
		} else {
			// Must be a variable somewhere.
			IVar &var = vars->at(v.id);
			IPart &part = parts->at(var.parent);

			Nat id = findId(part.vars, v.id);
			if (id > 0) {
				// Previous variable in here!
				assert(id < part.vars->count());
				return createVar(part.vars->at(id - 1));
			} else {
				// We need to find a part with variables in it!
				for (Part at = prevStored(Part(var.parent)); at != Part(); at = prevStored(at)) {
					IPart &now = parts->at(at.id);
					if (now.vars->any())
						return createVar(now.vars->last());
				}

				// None found. Try parameters:
				if (params->any())
					return createVar(params->last());

				// Nope. No more candidates then!
				return Var();
			}
		}
	}

	Part Listing::prev(Part p) const {
		if (p.id >= parts->count())
			return Part(invalid);

		IPart &part = parts->at(p.id);
		IBlock &block = blocks->at(part.block);
		if (part.index < 1) {
			// Find the last one in our parent block (if any).
			if (block.parent >= parts->count())
				return Part(invalid);

			return Part(block.parent);
		}
		return Part(block.parts->at(part.index - 1));
	}

	Part Listing::prevStored(Part p) const {
		if (p.id >= parts->count())
			return Part(invalid);

		IPart &part = parts->at(p.id);
		IBlock &block = blocks->at(part.block);
		if (part.index < 1) {
			// Find the last one in our parent block (if any).
			if (block.parent >= parts->count())
				return Part(invalid);

			return last(Part(block.parent));
		}
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

	void Listing::delay(Var v, Part to) {
		if (v.id >= vars->count())
			return;
		if (to.id >= parts->count())
			return;

		// See if it is possible...
		Part from = parent(v);
		if (first(from) != first(to))
			throw InvalidValue(L"Can only delay within the same block.");

		IPart &fromI = parts->at(from.id);
		IPart &toI = parts->at(to.id);
		IVar &varI = vars->at(v.id);

		if (varI.param)
			throw InvalidValue(L"Can not delay parameters!");

		Nat index = findId(fromI.vars, v.id);
		assert(index < fromI.vars->count());
		fromI.vars->remove(index);

		varI.parent = to.id;
		toI.vars->push(v.id);
	}

	void Listing::moveParam(Var v, Nat to) {
		if (!isParam(v))
			return;

		Nat pos = 0;
		for (nat i = 0; i < params->count(); i++) {
			if (params->at(i) == v.id) {
				pos = i;
				break;
			}
		}

		params->remove(pos);
		params->insert(to, v.id);
	}

	Part Listing::parent(Part p) const {
		if (p.id >= parts->count())
			return Part(invalid);

		IPart &part = parts->at(p.id);
		IBlock &block = blocks->at(part.block);
		return Part(block.parent);
	}

	Part Listing::parent(Var v) const {
		if (v.id >= vars->count())
			return Part(invalid);

		return Part(vars->at(v.id).parent);
	}

	Bool Listing::accessible(Var v, Part p) const {
		if (v.id >= vars->count())
			return false;
		if (p.id >= parts->count())
			return false;

		// Parameters are always visible.
		if (isParam(v))
			return true;

		IVar &var = vars->at(v.id);
		return isParent(first(Part(var.parent)), p);
	}

	Bool Listing::isParent(Block parent, Part q) const {
		if (parent.id >= parts->count())
			return false;
		if (q.id >= parts->count())
			return false;

		Nat target = findBlock(parent.id);

		for (Nat current = findBlock(q.id); current != invalid; current = findBlock(blocks->at(current).parent)) {
			if (current == target)
				return true;
		}

		return false;
	}

	Bool Listing::isParam(Var v) const {
		if (v.id >= vars->count())
			return false;

		return vars->at(v.id).param != null;
	}

	MAYBE(TypeDesc *) Listing::paramDesc(Var v) const {
		if (v.id >= vars->count())
			return null;

		return vars->at(v.id).param;
	}

	Operand Listing::freeFn(Var v) const {
		if (v.id >= vars->count())
			return Operand();

		return vars->at(v.id).freeFn;
	}

	FreeOpt Listing::freeOpt(Var v) const {
		if (v.id >= vars->count())
			return freeOnNone;

		return vars->at(v.id).freeOpt;
	}

	void Listing::freeOpt(Var v, FreeOpt opt) {
		if (v.id >= vars->count())
			return;

		vars->at(v.id).freeOpt = opt;
	}

	static bool checkFree(const Operand &free, FreeOpt &when) {
		if (when & freePtr)
			if (max(free.size(), Size::sLong) != Size::sLong)
				throw InvalidValue(L"Can not destroy values larger than 8 bytes by value.");

		if (!free.empty())
			if (when & freeOnException)
				return true;

		when = freeOnNone;
		return false;
	}

	Var Listing::createVar(Part in, Size size, Operand free, FreeOpt when) {
		assert(in.id != invalid, L"No such part!");

		if (checkFree(free, when))
			needEH = true;

		Nat id = vars->count();
		vars->push(IVar(in.id, size, null, free, when));

		IPart &part = parts->at(in.id);
		part.vars->push(id);

		return Var(id, size);
	}

	Var Listing::createVar(Part in, TypeDesc *type) {
		return createVar(in, type, freeDef);
	}

	Var Listing::createVar(Part in, TypeDesc *type, FreeOpt when) {
		if (ComplexDesc *c = as<ComplexDesc>(type)) {
			return createVar(in, type->size(), c->dtor, when | freePtr);
		} else {
			return createVar(in, type->size(), Operand(), when);
		}
	}

	Var Listing::createParam(TypeDesc *type) {
		return createParam(type, freeDef);
	}

	Var Listing::createParam(TypeDesc *type, FreeOpt when) {
		if (ComplexDesc *c = as<ComplexDesc>(type)) {
			return createParam(type, c->dtor, when | freePtr);
		} else {
			return createParam(type, Operand(), freeDef);
		}
	}

	Var Listing::createParam(TypeDesc *type, Operand free) {
		return createParam(type, free, freeDef);
	}

	Var Listing::createParam(TypeDesc *type, Operand free, FreeOpt when) {
		if (checkFree(free, when))
			needEH = true;

		Nat id = vars->count();
		vars->push(IVar(invalid, type->size(), type, free, when));

		params->push(id);
		return Var(id, type->size());
	}

	Nat Listing::findBlock(Nat partId) const {
		if (partId == invalid)
			return invalid;

		IPart &p = parts->at(partId);
		return p.block;
	}

	Array<Block> *Listing::allBlocks() const {
		Array<Block> *r = new (this) Array<Block>();
		for (nat i = 0; i < blocks->count(); i++) {
			IBlock &b = blocks->at(i);
			r->push(Block(b.parts->first()));
		}
		return r;
	}

	Array<Part> *Listing::allParts() const {
		Array<Part> *r = new (this) Array<Part>();
		for (nat i = 0; i < parts->count(); i++)
			r->push(Part(i));
		return r;
	}

	Array<Var> *Listing::allVars() const {
		Array<Var> *r = new (this) Array<Var>();
		for (nat i = 0; i < vars->count(); i++)
			r->push(createVar(i));
		return r;
	}

	Array<Var> *Listing::allVars(Block b) const {
		Array<Var> *r = new (this) Array<Var>();
		if (b.id >= parts->count())
			return r;

		for (Part p = b; p != Part(); p = next(p)) {
			IPart &part = parts->at(p.id);
			for (nat i = 0; i < part.vars->count(); i++)
				r->push(createVar(part.vars->at(i)));
		}
		return r;
	}

	Array<Var> *Listing::partVars(Part p) const {
		Array<Var> *r = new (this) Array<Var>();
		if (p.id >= parts->count())
			return r;

		IPart &part = parts->at(p.id);
		for (nat i = 0; i < part.vars->count(); i++)
			r->push(createVar(part.vars->at(i)));

		if (p == root()) {
			for (nat i = 0; i < params->count(); i++)
				r->push(createVar(params->at(i)));
		}

		return r;
	}

	Array<Var> *Listing::allParams() const {
		Array<Var> *r = new (this) Array<Var>();
		for (nat i = 0; i < params->count(); i++)
			r->push(createVar(params->at(i)));
		return r;
	}

	static Str *toS(Engine &e, Array<Label> *l) {
		if (!l)
			return new (e) Str(L"");

		StrBuf *to = new (e) StrBuf();
		for (nat i = 0; i < l->count(); i++) {
			if (i > 0)
				*to << L", ";
			*to << l->at(i);
		}
		*to << L": ";
		return to->toS();
	}

	void Listing::toS(StrBuf *to) const {
		if (params->any())
			*to << L"Parameters:\n";

		for (nat i = 0; i < params->count(); i++) {
			Nat id = params->at(i);
			putVar(*to, id);
		}

		putBlock(*to, 0);

		UsedRegs r = usedRegs(arena, this);
		*to << L"Dirty registers: " << r.all;

		for (nat i = 0; i < code->count(); i++) {
			Entry &e = code->at(i);

			*to << L"\n";
			if (e.labels != null && e.labels->any()) {
				*to << code::toS(engine(), e.labels) << L"\n";
			}
			*to << width(20) << r.used->at(i)->toS();
			*to << L" | " << e.instr;
		}
	}

	void Listing::putBlock(StrBuf &to, Nat block) const {
		to << L"Block " << block << L" {\n";
		{
			storm::Indent z(&to);

			Nat blockId = findBlock(block);
			IBlock &b = blocks->at(blockId);
			for (nat i = 0; i < b.parts->count(); i++) {
				putPart(to, b.parts->at(i), i != 0);
			}
		}
		to << L"}\n";
	}

	void Listing::putPart(StrBuf &to, Nat part, Bool header) const {
		if (header) {
			to << L"Part " << part << L":\n";
		}

		IPart &p = parts->at(part);
		for (nat i = 0; i < p.vars->count(); i++) {
			putVar(to, p.vars->at(i));
		}

		// Output all blocks in here as well.
		for (nat i = 0; i < blocks->count(); i++) {
			IBlock &block = blocks->at(i);
			if (block.parent == part)
				putBlock(to, block.parts->at(0));
		}
	}

	void Listing::putVar(StrBuf &to, Nat var) const {
		IVar &v = vars->at(var);
		to << width(3) << var << L": size " << v.size << L" free " << v.freeOpt << L" using " << v.freeFn << L"\n";
	}

}
