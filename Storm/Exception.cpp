#include "stdafx.h"
#include "Exception.h"

namespace storm {

	void CodeError::output(wostream &to) const {
		to << what();
	}

}
