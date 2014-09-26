#include "StdAfx.h"
#include "Block.h"

namespace code {

	Variable::Variable() : id(-1), sz(0) {}

	Variable::Variable(nat id, nat size) : id(id), sz(size) {}

	const Variable Variable::invalid;

	Block::Block() : id(-1) {}

	Block::Block(nat id) : id(id) {}

	const Block Block::invalid;
}