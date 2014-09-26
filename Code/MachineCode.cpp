#include "stdafx.h"
#include "MachineCode.h"
#include "Listing.h"

namespace code {
	namespace machine {

		void output(Output &to, Arena &arena, const Listing &listing) {
			std::multimap<nat, Label> labels = listing.getLabels();

			State state;
			std::multimap<nat, Label>::iterator at = labels.begin(), end = labels.end();

			for (nat line = 0; line < listing.size(); line++) {
				while (at != end && at->first <= line) {
					if (at->first == line) {
						to.markLabel(at->second);
					}
					++at;
				}

				output(to, arena, listing.frame, state, listing[line]);
			}

			// Any remaining labels shall also be added!
			while (at != end) {
				to.markLabel(at->second);
				++at;
			}
		}

		VarInfo variableInfo(const StackFrame &frame, const FnMeta *data, Variable variable) {
			return variableInfo(frame, data, variable.getId());
		}
	}
}