#include "StdAfx.h"
#include "Frame.h"
#include "Exception.h"

#include "Utils/Indent.h"
#include "Utils/Math.h"

namespace code {

	Frame::Frame() : nextBlockId(1), nextVariableId(0), anyDestructors(false) {
		// Create the root block as ID 0.
		blocks.insert(std::make_pair(0, InternalBlock()));
	}

	void Frame::output(wostream &to) const {
		outputBlock(to, root());
	}

	void Frame::outputBlock(wostream &to, Block block) const {
		to << endl << Value(block) << L":";

		Indent indent(to);
		vector<Block> c = children(block);
		vector<Variable> v = variables(block);

		for (nat i = 0; i < v.size(); i++) {
			to << endl << Value(v[i]);
			if (isParam(v[i]))
				to << " (param)";
		}

		for (nat i = 0; i < c.size(); i++)
			outputBlock(to, c[i]);
	}

	bool Frame::exceptionHandlerNeeded() const {
		return anyDestructors;
	}

	Block Frame::root() {
		return Block(0);
	}

	vector<Block> Frame::allBlocks() const {
		vector<Block> r(nextBlockId);
		for (nat i = 0; i < nextBlockId; i++) {
			r[i] = Block(i);
		}
		return r;
	}

	vector<Block> Frame::children(Block b) const {
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

	Variable Frame::prev(Variable v) const {
		if (isParam(v)) {
			ParamMap::const_iterator i = parameters.find(v.id);
			if (i == parameters.begin())
				return Variable::invalid;

			--i;
			const Param &p = i->second;
			return Variable(i->first, p.size);
		} else {
			Block p = parent(v);
			const InternalBlock &b = blocks.find(p.id)->second;

			vector<nat>::const_iterator pos = std::find(b.variables.begin(), b.variables.end(), v.id);
			nat id = Variable::invalid.id;

			if (pos == b.variables.begin()) {
				// Find a new variable in any parent blocks!

				while (id == Variable::invalid.id) {
					p = parent(p);
					if (p == Block::invalid)
						return Variable::invalid;

					const InternalBlock &parent = blocks.find(p.id)->second;
					nat numVars = parent.variables.size();
					if (numVars > 0)
						id = parent.variables[numVars - 1];
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

	Block Frame::parent(Variable v) const {
		if (parameters.count(v.id) == 1) {
			return root();
		} else if (vars.count(v.id) == 1) {
			return Block(vars.find(v.id)->second.block);
		} else {
			assert(false);
			return root();
		}
	}

	Block Frame::parent(Block b) const {
		if (b == Block::invalid)
			throw FrameError();

		return Block(blocks.find(b.id)->second.parent);
	}

	Block Frame::createChild(Block parent) {
		if (parent == Block::invalid)
			throw FrameError();

		assert(blocks.count(parent.id) == 1);
		InternalBlock block;
		block.parent = parent.id;

		blocks.insert(std::make_pair(nextBlockId, block));

		return Block(nextBlockId++);
	}

	Variable Frame::createVariable(Block in, nat size, Value free) {
		if (in == Block::invalid)
			throw FrameError();

		if (!free.empty())
			anyDestructors = true;

		assert(blocks.count(in.id));
		InternalBlock &block = blocks[in.id];
		block.variables.push_back(nextVariableId);

		Var v = { in.id, size, free };
		vars.insert(std::make_pair(nextVariableId, v));

		return Variable(nextVariableId++, size);
	}

	Variable Frame::createParameter(nat size, bool isFloat, Value free) {
		Param p = { size, isFloat, free };
		parameters.insert(std::make_pair(nextVariableId, p));
		return Variable(nextVariableId++, size);
	}

	nat Frame::size(nat v) const {
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

	vector<Variable> Frame::variables(Block b) const {
		if (b == Block::invalid)
			throw FrameError();

		const InternalBlock &block = blocks.find(b.id)->second;
		vector<Variable> r(block.variables.size());

		for (nat i = 0; i < r.size(); i++) {
			const Var &v = vars.find(block.variables[i])->second;
			r[i] = Variable(block.variables[i], v.size);
		}

		// Root block?
		if (b.id == 0) {
			ParamMap::const_iterator at = parameters.begin(), end = parameters.end();
			for (; at != end; ++at) {
				nat id = at->first;
				nat size = at->second.size;
				r.push_back(Variable(id, size));
			}
		}

		return r;
	}

	nat Frame::parameterCount() const {
		return parameters.size();
	}

}
