#include "stdafx.h"
#include "BSBlock.h"

namespace storm {

	bs::Block::Block() : parent(null) {}

	bs::Block::Block(Auto<Block> parent) : parent(parent.borrow()) {}

}
