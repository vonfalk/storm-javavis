#include "stdafx.h"
#include "GtkCommon.h"

#ifdef GUI_GTK
namespace gui {

	cairo_matrix_t cairoMultiply(const cairo_matrix_t &a, const cairo_matrix_t &b) {
		cairo_matrix_t result;
		cairo_matrix_multiply(&result, &a, &b);
		return result;
	}

	cairo_matrix_t cairo(Transform *tfm) {
		cairo_matrix_t m = {
			tfm->at(0, 0), tfm->at(0, 1),
			tfm->at(1, 0), tfm->at(1, 1),
			tfm->at(3, 0), tfm->at(3, 1),
		};
		return m;
	}

}
#endif
