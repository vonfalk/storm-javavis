#include "StdAfx.h"
#include "Output.h"
#include "Exception.h"

namespace code {

	void Output::putSize(Word w, nat size) {
		switch (size) {
			case 0:
				putPtr(cpuNat(w));
				break;
			case 1:
				putByte(Byte(w));
				break;
			case 4:
				putInt(Nat(w));
				break;
			case 8:
				putLong(Word(w));
				break;
			default:
				assert(false);
				break;
		}
	}

	void Output::putAddress(const Label &lbl) {
		cpuNat addr = lookupLabel(lbl.id);
		putPtr(addr);
	}

	void Output::putRelative(cpuNat addr) {
		putPtr(toRelative(addr));
	}

	void Output::putRelative(const Label &lbl) {
		cpuNat addr = lookupLabel(lbl.id);
		putPtr(toRelative(addr));
	}

	void Output::putAddress(const Ref &ref) {
		nat offset = tell();

		cpuNat addr = (cpuNat)ref.address();
		putPtr(addr);

		UsedRef r = { ref, offset };
		absoluteRefs.push_back(r);
	}

	void Output::putRelative(const Ref &ref) {
		nat offset = tell();

		cpuNat addr = (cpuNat)ref.address();
		putPtr(toRelative(addr));

		UsedRef r = { ref, offset };
		relativeRefs.push_back(r);
	}

	void Output::markLabel(Label lbl) {
		markLabelId(lbl.id);
	}

	//////////////////////////////////////////////////////////////////////////
	// Size specialization
	//////////////////////////////////////////////////////////////////////////

	SizeOutput::SizeOutput(nat numLabels) : labelOffsets(numLabels, invalidOffset), size(0) {}

	void SizeOutput::putByte(Byte b) {
		size += 1;
	}

	void SizeOutput::putInt(Nat w) {
		size += sizeof(Nat);
	}

	void SizeOutput::putLong(Word w) {
		size += sizeof(Word);
	}

	void SizeOutput::putPtr(cpuNat v) {
		size += sizeof(cpuNat);
	}

	nat SizeOutput::tell() const {
		return size;
	}

	cpuNat SizeOutput::lookupLabel(nat id) {
		return 0;
	}

	cpuNat SizeOutput::toRelative(cpuNat addr) {
		return 0;
	}

	void SizeOutput::markLabelId(nat id) {
		assert(labelOffsets.size() > id);
		assert(labelOffsets[id] == invalidOffset);
		if (labelOffsets[id] != invalidOffset)
			throw DuplicateLabelError(id);
		labelOffsets[id] = size;
	}

	//////////////////////////////////////////////////////////////////////////
	// Memory specialization
	//////////////////////////////////////////////////////////////////////////

	MemoryOutput::MemoryOutput(void *to, SizeOutput &metrics) : labelOffsets(metrics.labelOffsets), dest((byte *)to), size(metrics.tell()), at(0) {}

	void MemoryOutput::putByte(Byte b) {
		assert(at + 1 <= size);
		dest[at++] = b;
	}

	void MemoryOutput::putInt(Nat w) {
		assert(at + sizeof(w) <= size);
		Nat *to = (Nat *)(dest + at);
		*to = w;
		at += sizeof(w);
	}

	void MemoryOutput::putLong(Word w) {
		assert(at + sizeof(w) <= size);
		Word *to = (Word *)(dest + at);
		*to = w;
		at += sizeof(w);
	}

	void MemoryOutput::putPtr(cpuNat w) {
		assert(at + sizeof(w) <= size);
		cpuNat *to = (cpuNat *)(dest + at);
		*to = w;
		at += sizeof(w);
	}

	nat MemoryOutput::tell() const {
		return at;
	}

	cpuNat MemoryOutput::lookupLabel(nat id) {
		if (labelOffsets[id] == SizeOutput::invalidOffset)
			throw UnusedLabelError(id);

		return (cpuNat)dest + labelOffsets[id];
	}

	cpuNat MemoryOutput::toRelative(cpuNat addr) {
		return addr - (cpuInt)(dest + at + sizeof(cpuNat));
	}

	// Compute the address of 'lbl'. Returns null if the label has not been defined yet.
	void *MemoryOutput::lookup(Label lbl) {
		nat id = labelId(lbl);

		if (labelOffsets[id] == SizeOutput::invalidOffset)
			return null;

		return (void *)lookupLabel(id);
	}


	void MemoryOutput::markLabelId(nat id) {
		assert(labelOffsets[id] == at);
	}
}
