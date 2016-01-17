#include "stdafx.h"
#include "Frame.h"
#include "Exception.h"

#include "Utils/Indent.h"
#include "Utils/Bitwise.h"

namespace code {

	String name(FreeOpt f) {
		wchar *first = L"never";
		wchar *second = L"by value";

		if ((f & freeOnBoth) == freeOnBoth)
			first = L"always";
		else if (f & freeOnBlockExit)
			first = L"on exit block";
		else if (f & freeOnException)
			first = L"on exception";

		if (f & freePtr)
			second = L"by ptr";

		return String(first) + L", " + String(second);
	}

	Frame::Frame() : nextPartId(1), nextVariableId(0), anyDestructors(false) {
		// Create the root block as ID 0.
		blocks.insert(make_pair(0, InternalBlock()));
		parts.insert(make_pair(0, InternalPart(0)));
	}

	void Frame::output(wostream &to) const {
		outputBlock(to, root());
	}

	void Frame::outputBlock(wostream &to, Block block) const {
		to << endl << "Block" << block.id << L":";

		for (Part p = block; p != Part::invalid; p = next(p)) {
			if (asBlock(p) == Block::invalid)
				to << endl << L" " << Value(p) << L":";

			Indent indent(to);

			vector<Block> c = children(p);
			vector<Variable> v = variables(p);
			for (nat i = 0; i < v.size(); i++) {
				to << endl << Value(v[i]);
				Value free = freeFn(v[i]);
				if (free != Value())
					to << L" (free: " << free << L", " << name(freeOpt(v[i])) << L")";
				if (isParam(v[i]))
					to << L" (param)";
			}

			for (nat i = 0; i < c.size(); i++)
				outputBlock(to, c[i]);

			if (v.size() + c.size() == 0)
				to << endl << "(empty)";
		}
	}

	bool Frame::exceptionHandlerNeeded() const {
		return anyDestructors;
	}

	Block Frame::root() {
		return Block(0);
	}

	vector<Block> Frame::allBlocks() const {
		vector<Block> r;
		r.reserve(blocks.size());
		for (BlockMap::const_iterator i = blocks.begin(), end = blocks.end(); i != end; ++i) {
			r.push_back(Block(i->first));
		}
		return r;
	}

	vector<Part> Frame::allParts() const {
		vector<Part> r(nextPartId);
		for (nat i = 0; i < nextPartId; i++)
			r[i] = Part(i);
		return r;
	}

	vector<Block> Frame::children(Part b) const {
		vector<Block> r;
		for (BlockMap::const_iterator i = blocks.begin(); i != blocks.end(); ++i) {
			if (i->second.parent == b.id)
				r.push_back(Block(i->first));
		}
		return r;
	}

	vector<Variable> Frame::allVariables() const {
		vector<Variable> r(nextVariableId);
		for (nat i = 0; i < nextVariableId; i++) {
			r[i] = Variable(i, size(i));
		}
		return r;
	}

	Part Frame::prev(Part p) const {
		if (p == Part::invalid)
			return p;

		Block b = asBlock(p);
		if (b != Block::invalid)
			return parent(b);
		return Part(parts.find(p.id)->second.prev);
	}

	Part Frame::prevStored(Part p) const {
		if (p == Part::invalid)
			return p;

		Block b = asBlock(p);
		if (b != Block::invalid)
			return last(parent(b));
		return prev(p);
	}

	Variable Frame::prev(Variable v) const {
		if (isParam(v)) {
			const Param &p = parameters.find(v.id)->second;
			if (p.index == 0)
				return Variable::invalid;

			nat prevId = parameterOrder[p.index - 1];
			const Param &prev = parameters.find(prevId)->second;
			return Variable(prevId, prev.size);
		} else {
			Part z = parent(v);
			const InternalPart &p = parts.find(z.id)->second;

			vector<nat>::const_iterator pos = std::find(p.variables.begin(), p.variables.end(), v.id);
			nat id = Variable::invalid.id;

			if (pos == p.variables.begin()) {
				// Find a new variable in any parent blocks!
				while (id == Variable::invalid.id) {
					// ...
					z = prevStored(z);
					// TODO? Return the last parameter here?
					if (z == Block::invalid)
						return Variable::invalid;

					const InternalPart &prev = parts.find(z.id)->second;
					nat numVars = prev.variables.size();
					if (numVars > 0)
						id = prev.variables[numVars - 1];
				}
			} else {
				--pos;
				id = *pos;
			}

			if (id == Variable::invalid.id)
				return Variable::invalid;
			return Variable(id, size(id));
		}
	}

	bool Frame::isParam(Variable v) const {
		return parameters.count(v.id) == 1;
	}

	Part Frame::parent(Variable v) const {
		if (parameters.count(v.id) == 1) {
			return root();
		} else if (vars.count(v.id) == 1) {
			return Part(vars.find(v.id)->second.part);
		} else {
			assert(false);
			return root();
		}
	}

	Part Frame::parent(Block b) const {
		if (b == Block::invalid)
			throw FrameError();

		return Part(blocks.find(b.id)->second.parent);
	}

	bool Frame::indirectParent(Block parent, Block child) const {
		for (Block c = child; c != Block::invalid; c = first(this->parent(c))) {
			if (c == parent)
				return true;
		}
		return false;
	}

	bool Frame::accessible(Part b, Variable v) const {
		return indirectParent(first(parent(v)), first(b));
	}

	Part Frame::next(Part p) const {
		if (p == Part::invalid)
			return Part::invalid;
		return Part(parts.find(p.id)->second.next);
	}

	Part Frame::last(Part p) const {
		for (Part n = next(p); n != Part::invalid; n = next(n))
			p = n;
		return p;
	}

	Block Frame::first(Part p) const {
		if (p == Part::invalid)
			return Block::invalid;
		return Block(parts.find(p.id)->second.block);
	}

	Block Frame::asBlock(Part p) const {
		if (blocks.count(p.id))
			return Block(p.id);
		return Block::invalid;
	}

	Block Frame::createChild(Part parent) {
		if (parent == Part::invalid)
			throw FrameError();
		assert(parts.count(parent.id) == 1);

		InternalBlock block(parent.id);
		blocks.insert(make_pair(nextPartId, block));

		InternalPart part(nextPartId);
		parts.insert(make_pair(nextPartId, part));

		return Block(nextPartId++);
	}

	Block Frame::createChildLast(Part parent) {
		return createChild(last(parent));
	}

	Part Frame::createPart(Part before) {
		if (before == Part::invalid)
			throw FrameError();
		assert(parts.count(before.id) == 1);

		before = last(before);

		InternalPart &b = parts.find(before.id)->second;
		b.next = nextPartId;

		InternalPart part(b.block, before.id);
		parts.insert(make_pair(nextPartId, part));
		return Part(nextPartId++);
	}

	static bool checkFree(const Value &free, FreeOpt on) {
		if ((on & freePtr) != freePtr)
			if (free.size() > Size(8))
				throw InvalidValue(L"Can not destroy values larger than 8 bytes by value.");

		if (!free.empty())
			if (on & freeOnException)
				return true;
		return false;
	}

	Variable Frame::createVariable(Part in, Size size, Value free, FreeOpt on) {
		if (in == Part::invalid)
			throw FrameError();
		assert(parts.count(in.id));

		if (checkFree(free, on))
			anyDestructors = true;

		InternalPart &part = parts.find(in.id)->second;
		part.variables.push_back(nextVariableId);

		Var v = { in.id, size, free, on };
		vars.insert(std::make_pair(nextVariableId, v));

		return Variable(nextVariableId++, size);
	}

	Variable Frame::createParameter(Size size, bool isFloat, Value free, FreeOpt on) {
		if (checkFree(free, on))
			anyDestructors = true;

		Param p = { parameterOrder.size(), size, isFloat, free, on };
		parameters.insert(std::make_pair(nextVariableId, p));
		parameterOrder.push_back(nextVariableId);

		return Variable(nextVariableId++, size);
	}

	void Frame::moveParam(Variable param, nat to) {
		assert(isParam(param), L"Trying to move a non-parameter variable!");
		Param &p = parameters[param.id];
		parameterOrder.erase(parameterOrder.begin() + p.index);
		parameterOrder.insert(parameterOrder.begin() + to, param.id);

		// Update all indices.
		for (nat i = 0; i < parameterOrder.size(); i++) {
			nat id = parameterOrder[i];
			parameters[id].index = i;
		}
	}

	void Frame::delay(Variable v, Part to) {
		if (isParam(v))
			throw InvalidValue(L"Can not delay parameters.");

		Part from = parent(v);
		if (first(from) != first(to))
			throw InvalidValue(L"Can only delay within the same block.");

		InternalPart &p = parts.find(from.id)->second;
		vector<nat>::iterator i = find(p.variables.begin(), p.variables.end(), v.id);
		assert(i != p.variables.end());
		p.variables.erase(i);

		InternalPart &q = parts.find(to.id)->second;
		q.variables.push_back(v.id);

		Var &z = vars.find(v.id)->second;
		z.part = to.id;
	}

	Size Frame::size(nat v) const {
		if (parameters.count(v)) {
			return parameters.find(v)->second.size;
		} else if (vars.count(v)) {
			return vars.find(v)->second.size;
		} else {
			throw FrameError();
		}
	}

	Value Frame::freeFn(Variable v) const {
		if (parameters.count(v.id)) {
			return parameters.find(v.id)->second.freeFn;
		} else if (vars.count(v.id)) {
			return vars.find(v.id)->second.freeFn;
		} else {
			throw FrameError();
		}
	}

	FreeOpt Frame::freeOpt(Variable v) const {
		if (parameters.count(v.id)) {
			return parameters.find(v.id)->second.freeOpt;
		} else if (vars.count(v.id)) {
			return vars.find(v.id)->second.freeOpt;
		} else {
			throw FrameError();
		}
	}

	vector<Variable> Frame::allVariables(Block b) const {
		vector<Variable> r;

		for (Part c = b; c != Part::invalid; c = next(c))
			variables(c, r);

		return r;
	}

	vector<Variable> Frame::variables(Part p) const {
		vector<Variable> r;
		variables(p, r);
		return r;
	}

	void Frame::variables(Part p, vector<Variable> &r) const {
		if (p == Block::invalid)
			throw FrameError();

		const InternalPart &part = parts.find(p.id)->second;
		r.reserve(r.size() + part.variables.size());

		for (nat i = 0; i < part.variables.size(); i++) {
			nat id = part.variables[i];
			r.push_back(Variable(id, size(id)));
		}

		// Root part?
		if (p.id == 0) {
			for (nat i = 0; i < parameterOrder.size(); i++) {
				nat id = parameterOrder[i];
				r.push_back(Variable(id, size(id)));
			}
		}
	}

	nat Frame::parameterCount() const {
		return parameters.size();
	}

}
