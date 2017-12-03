#include "stdafx.h"
#include "GtkCommon.h"

namespace gui {

	cairo_matrix_t cairoMultiply(const cairo_matrix_t &a, const cairo_matrix_t &b) {
		cairo_matrix_t result;
		cairo_matrix_multiply(&result, &a, &b);
		return result;
	}

	cairo_matrix_t cairo(Transform *tfm) {
		// Note that we are taking the transpose of the matrix, since cairo uses a
		// row vector when multiplying while we're using a column vector.
		cairo_matrix_t m = {
			tfm->at(0, 0), tfm->at(1, 0),
			tfm->at(0, 1), tfm->at(1, 1),
			tfm->at(0, 3), tfm->at(1, 3),
		};
		return m;
	}

}
