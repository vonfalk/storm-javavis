#pragma once
#include "Template.h"

namespace storm {
	namespace bs {

		/**
		 * Resolve template functions in Basic Storm. Allows multiple templates with the same name to
		 * exist, as long as both do not apply for the actual parameter types.
		 */
		class TemplateAdapter : public Template {
			STORM_CLASS;
		public:
			// Create, set name.
			STORM_CTOR TemplateAdapter(Auto<Str> name);
			TemplateAdapter(const String &name);

			// Generate things.
			virtual Named *STORM_FN generate(Par<SimplePart> par);

			// Add a new template variant with this name.
			virtual void STORM_FN add(Par<Template> t);

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// Contents.
			Auto<ArrayP<Template>> data;
		};

	}
}
