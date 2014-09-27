#include "StdAfx.h"
#include "Listing.h"

#include "UsedRegisters.h"

#include <map>
#include <iomanip>

namespace code {

	// Note: label id 0 is reserved for the metadata label.
	Listing::Listing() : nextLabelId(1) {}

	Listing Listing::createShell(const Listing &other) {
		Listing n;

		n.nextLabelId = other.nextLabelId;
		n.frame = other.frame;

		return n;
	}

	Label Listing::label() {
		return Label(nextLabelId++);
	}

	Label Listing::metadata() {
		return Label(0);
	}

	Listing &Listing::operator <<(const Instruction &instr) {
		code.push_back(instr);
		return *this;
	}

	Listing &Listing::operator <<(const Label &label) {
		assert(labels.count(label.id) == 0);
		labels[label.id] = code.size();
		return *this;
	}

	Listing &Listing::operator <<(const Listing &listing) {
		for (nat i = 0; i < listing.size(); i++) {
			*this << listing[i];
		}
		return *this;
	}

	std::multimap<nat, Label> Listing::getLabels() const {
		std::multimap<nat, Label> labels;
		for (Labels::const_iterator i = this->labels.begin(); i != this->labels.end(); ++i) {
			labels.insert(std::make_pair(i->second, Label(i->first)));
		}
		return labels;
	}

	static String genLabels(std::multimap<nat, Label> &all, nat line) {
		std::multimap<nat, Label>::iterator first = all.lower_bound(line);
		std::multimap<nat, Label>::iterator last = all.upper_bound(line);

		std::wostringstream to;

		for (std::multimap<nat, Label>::iterator i = first; i != last; ++i) {
			if (i != first)
				to << " ";
			to << i->second.toS();
		}

		return to.str();
	}

	void Listing::output(std::wostream &to) const {
		std::multimap<nat, Label> labels = getLabels();

		const nat indent = 6;

		UsedRegisters registers(*this);

		for (nat i = 0; i < code.size(); i++) {
			to << std::endl;

			String lbl = genLabels(labels, i);
			nat spaces = indent;
			if (lbl != L"") {
				to << lbl << L": ";
				if (lbl.size() + 2 < indent)
					spaces = indent - 2 - lbl.size();
				else
					to << std::endl;
			}
			to << std::setw(spaces) << ' ';
			to << std::setw(25) << registers[i].toS() << ' ';
			to << code[i];
		}
	}

	void Transformer::before(Listing &to) {}
	void Transformer::after(Listing &to) {}

	Listing Transformer::transformed() {
		Listing to = Listing::createShell(from);
		std::multimap<nat, Label> labels = from.getLabels();

		std::multimap<nat, Label>::iterator at = labels.begin(), end = labels.end();

		before(to);

		for (nat line = 0; line < from.size(); line++) {
			while (at != end && at->first <= line) {
				if (at->first == line)
					to << at->second;

				++at;
			}

			transform(to, line);
		}

		// Any remaining labels shall also be added!
		while (at != end) {
			to << at->second;
			++at;
		}

		after(to);

		return to;
	}
}
