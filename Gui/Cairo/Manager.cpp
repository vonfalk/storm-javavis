#include "stdafx.h"
#include "Manager.h"
#include "Exception.h"

namespace gui {

#ifdef GUI_GTK

	// A value we use for dummy Cairo objects. It is not null, so that it does not trigger
	// re-generation of the resource (which costs a few cycles, even if we don't do anything), but
	// not large enough so that the GC believes it to be something significant.
	static void *const CAIRO_NOTHING = (void *)0x1;

	// A value we use for solid color brushes that is not null, yet not a valid pointer.
	static void *const COLOR_BRUSH = CAIRO_NOTHING;

	static void destroySurface(void *surface) {
		if (surface)
			cairo_surface_destroy((cairo_surface_t *)surface);
	}

	static void destroyPattern(void *pattern) {
		if (pattern)
			cairo_pattern_destroy((cairo_pattern_t *)pattern);
	}

	CairoManager::CairoManager(Graphics *owner, CairoSurface *surface) : owner(owner), surface(surface) {}

	void CairoManager::applyBrush(cairo_t *to, Brush *brush, void *data) {
		if (data == COLOR_BRUSH) {
			SolidBrush *b = static_cast<SolidBrush *>(brush);
			Color c = b->color();
			cairo_set_source_rgba(to, c.r, c.g, c.b, c.a * b->opacity());
		} else {
			cairo_set_source(to, (cairo_pattern_t *)data);
		}
	}

	void CairoManager::create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		// We don't need any data for these, but mark it as non-null.
		result = COLOR_BRUSH;
		cleanup = null;
		(void)brush;
	}

	void CairoManager::update(SolidBrush *brush, void *resource) {
		// We don't need to do anything.
		(void)brush;
		(void)resource;
	}

	void CairoManager::create(BitmapBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		cairo_surface_t *surface = (cairo_surface_t *)brush->bitmap()->forGraphicsRaw(owner);
		cairo_pattern_t *p = cairo_pattern_create_for_surface(surface);
		cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);

		cairo_matrix_t tfm = cairo(brush->transform());
		cairo_matrix_invert(&tfm);
		cairo_pattern_set_matrix(p, &tfm);

		result = p;
		cleanup = &destroyPattern;
	}

	void CairoManager::update(BitmapBrush *brush, void *resource) {
		cairo_pattern_t *p = (cairo_pattern_t *)resource;
		cairo_matrix_t tfm = cairo(brush->transform());
		cairo_matrix_invert(&tfm);
		cairo_pattern_set_matrix(p, &tfm);
	}

	static void addStops(cairo_pattern_t *to, Gradient *from) {
		Array<GradientStop> *stops = from->peekStops();
		for (Nat i = 0; i < stops->count(); i++) {
			GradientStop &at = stops->at(i);
			cairo_pattern_add_color_stop_rgba(to, at.pos, at.color.r, at.color.g, at.color.b, at.color.a);
		}
	}

	// Update the transform of a linear gradient to match the specified points.
	static void setLinearTfm(cairo_pattern_t *to, Point start, Point end) {
		Point delta = end - start;
		Angle a = angle(delta);
		Float scale = 1.0f / delta.length();

		// Compute a matrix so that 'start' and 'end' map to the points (0, 0) and (0, 1).
		cairo_matrix_t tfm;
		// Note: transformations are applied in reverse.
		cairo_matrix_init_rotate(&tfm, -a.rad());
		cairo_matrix_scale(&tfm, scale, scale);
		cairo_matrix_translate(&tfm, -start.x, -start.y);
		cairo_pattern_set_matrix(to, &tfm);
	}

	void CairoManager::create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		// We're using the points (0, 0) - (0, -1) so that we can easily transform them later (= 0 deg).
		cairo_pattern_t *p = cairo_pattern_create_linear(0, 0, 0, -1);
		addStops(p, brush);
		setLinearTfm(p, brush->start(), brush->end());

		result = p;
		cleanup = &destroyPattern;
	}

	void CairoManager::update(LinearGradient *brush, void *resource) {
		cairo_pattern_t *p = (cairo_pattern_t *)resource;
		setLinearTfm(p, brush->start(), brush->end());
	}

	// Update the transform of a radial gradient to match its settings.
	static void setRadialTfm(cairo_pattern_t *to, RadialGradient *brush) {
		Float r = 1.0f / brush->radius();

		cairo_matrix_t tfm;
		cairo_matrix_init_scale(&tfm, r, r);
		cairo_matrix_translate(&tfm, -brush->center().x, -brush->center().y);

		cairo_matrix_t our = cairo(brush->transform());
		cairo_matrix_invert(&our);
		cairo_matrix_multiply(&tfm, &our, &tfm);

		cairo_pattern_set_matrix(to, &tfm);
	}

	void CairoManager::create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		// We're using the point (0, 0) and a radius of 1 so that we can easily transform it later.
		cairo_pattern_t *p = cairo_pattern_create_radial(0, 0, 0, 0, 0, 1);
		addStops(p, brush);
		setRadialTfm(p, brush);

		result = p;
		cleanup = &destroyPattern;
	}

	void CairoManager::update(RadialGradient *brush, void *resource) {
		cairo_pattern_t *r = (cairo_pattern_t *)resource;
		setRadialTfm(r, brush);
	}

	void CairoManager::create(Bitmap *bitmap, void *&result, Resource::Cleanup &cleanup) {
		Image *img = bitmap->image();
		Nat w = img->width();
		Nat h = img->height();

		cairo_surface_t *texture = cairo_surface_create_similar(surface->surface, CAIRO_CONTENT_COLOR_ALPHA, w, h);
		cairo_surface_t *image = cairo_surface_map_to_image(texture, NULL);

		byte *src = img->buffer();
		Nat srcStride = img->stride();
		byte *dest = cairo_image_surface_get_data(image);
		Nat destStride = cairo_image_surface_get_stride(image);
		cairo_format_t fmt = cairo_image_surface_get_format(image);
		if (fmt != CAIRO_FORMAT_ARGB32)
			throw new (this) GuiError(S("Invalid format from Cairo. Expected ARGB32."));

		for (Nat y = 0; y < h; y++) {
			for (Nat x = 0; x < w; x++) {
				Nat *d = (Nat *)(dest + destStride*y + x*4);
				byte *s = src + srcStride*y + x*4;

				// Convert to premultiplied alpha.
				float a = float(s[3]) / 255.0f;
				Nat pixel = 0;
				pixel |= (Nat(s[3] * 1) & 0xFF) << 24; // A
				pixel |= (Nat(s[0] * a) & 0xFF) << 16; // R
				pixel |= (Nat(s[1] * a) & 0xFF) <<  8; // G
				pixel |= (Nat(s[2] * a) & 0xFF) <<  0; // B
				*d = pixel;
			}
		}
		cairo_surface_mark_dirty(image);
		cairo_surface_unmap_image(texture, image);
		cairo_surface_mark_dirty(texture);

		result = texture;
		cleanup = &destroySurface;
	}

	void CairoManager::update(Bitmap *, void *) {
		// Should never be called.
	}

	void CairoManager::create(Path *path, void *&result, Resource::Cleanup &cleanup) {
		result = CAIRO_NOTHING;
		cleanup = null;
		(void)path;
	}

	void CairoManager::update(Path *, void *) {
		// Should never be called.
	}

#else

	DEFINE_GRAPHICS_MGR_FNS(CairoManager);

#endif

}
