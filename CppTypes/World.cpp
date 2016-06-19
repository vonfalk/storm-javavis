#include "stdafx.h"
#include "World.h"
#include "Tokenizer.h"

void parseFile(nat id, World &world) {
	Tokenizer tok(id, 0);

	while (tok.more()) {
		PVAR(tok.next());
	}
}

World parseWorld() {
	World world;

	for (nat i = 0; i < SrcPos::files.size(); i++) {
		parseFile(i, world);
	}

	return world;
}
