#include "stdafx.h"
#include "Binary.h"

namespace code {

	Binary::Binary(Arena *arena, Listing *listing) {
		Listing *tfm = arena->transform(listing, this);
		// PVAR(tfm);

		fillParts(tfm);

		LabelOutput *labels = arena->labelOutput();
		arena->output(tfm, labels);

		if (tfm->meta().id < labels->offsets->count()) {
			metaOffset = labels->offsets->at(tfm->meta().id);
		} else {
			metaOffset = 0;
			WARNING(L"No metadata seems to have been generated by the backend.");
			WARNING(L"Exception cleanup will not work!");
		}

		CodeOutput *output = arena->codeOutput(this, labels);
		arena->output(tfm, output);

		runtime::codeUpdatePtrs(output->codePtr());
		set(output->codePtr(), output->tell());
	}

	const GcType Binary::partArrayType = {
		GcType::tArray,
		null,
		null,
		sizeof(void *),
		1,
		{ 0 },
	};

	const GcType Binary::partType = {
		GcType::tArray,
		null,
		null,
		sizeof(Variable),
		0,
		{},
	};

	void Binary::fillParts(Listing *src) {
		Array<code::Part> *srcParts = src->allParts();

		parts = runtime::allocArray<Part *>(engine(), &partArrayType, srcParts->count());

		for (nat i = 0; i < srcParts->count(); i++) {
			code::Part part = srcParts->at(i);
			Array<Var> *vars = src->partVars(part);

			Part *p = (Part *)runtime::allocArray(engine(), &partType, vars->count());
			parts->v[i] = p;
			p->prev = src->prev(part).key();

			for (nat j = 0; j < vars->count(); j++) {
				const Var &v = vars->at(j);
				p->vars[j].id = v.key();
				p->vars[j].freeOpt = src->freeOpt(v);
				p->vars[j].size = v.size();
			}
		}
	}

	void Binary::cleanup(StackFrame &frame) {
		for (Nat i = frame.part; i != code::Part().key(); i = parts->v[i]->prev) {
			Part *p = parts->v[i];

			// Reverse order is common.
			for (Nat j = p->count; j > 0; j--) {
				cleanup(frame, p->vars[j - 1]);
			}
		}
	}

	void Binary::cleanup(StackFrame &frame, Variable &v) {
		if ((v.freeOpt & freeOnException) != 0) {
			byte *data = (byte *)address();
			size_t *table = (size_t *)(data + metaOffset);

			void *freeFn = (void *)table[v.id*2];
			size_t offset = table[v.id*2 + 1];

			void *ptr = frame.toPtr(offset);

			typedef void (*FPtr)(void *v);
			typedef void (*FByte)(Byte v);
			typedef void (*FInt)(Int v);
			typedef void (*FLong)(Long v);

			if (v.freeOpt & freePtr) {
				FPtr p = (FPtr)freeFn;
				(*p)(ptr);
			} else if (v.size == Size::sPtr) {
				FPtr p = (FPtr)freeFn;
				(*p)(*(void **)ptr);
			} else if (v.size == Size::sByte) {
				FByte p = (FByte)freeFn;
				(*p)(*(Byte *)ptr);
			} else if (v.size == Size::sInt) {
				FInt p = (FInt)freeFn;
				(*p)(*(Int *)ptr);
			} else if (v.size == Size::sLong) {
				FLong p = (FLong)freeFn;
				(*p)(*(Long *)ptr);
			} else {
				WARNING(L"Unsupported size for destruction (" + ::toS(v.size) + L". Use 'freePtr' instead!");
			}
		}
	}

}
