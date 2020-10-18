#include "stdafx.h"
#include "Bitmap.h"
#include "Painter.h"
#include "Exception.h"

namespace gui {

	Bitmap::Bitmap(Image *img) : src(img) {}

#ifdef GUI_WIN32

	void Bitmap::create(Painter *owner, ID2D1Resource **out) {
		Nat w = src->width();
		Nat h = src->height();

		D2D1_SIZE_U s = { w, h };
		D2D1_BITMAP_PROPERTIES props = {
			{ DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
			0, 0
		};
		owner->renderTarget()->GetDpi(&props.dpiX, &props.dpiY);

		// Non-premultiplied alpha is not supported. We need to convert our source bitmap...
		byte *src = this->src->buffer();
		nat stride = this->src->stride();
		// byte *img = runtime::allocArray<Byte>(engine(), &byteArrayType, stride * h)->v;
		// We'll see if this crashes less than the above... (which was already very rare).
		byte *img = runtime::allocBuffer(engine(), stride * h)->v;
		for (Nat y = 0; y < h; y++) {
			Nat base = stride * y;
			for (Nat x = 0; x < w; x++) {
				Nat p = base + x * 4;
				float a = float(src[p + 3]) / 255.0f;
				img[p+3] = src[p+3];
				img[p+0] = byte(src[p+0] * a);
				img[p+1] = byte(src[p+1] * a);
				img[p+2] = byte(src[p+2] * a);
			}
		}

		HRESULT r = owner->renderTarget()->CreateBitmap(s, img, stride, props, (ID2D1Bitmap **)out);
		if (FAILED(r))
			PLN(L"Error: " + ::toS(r));
	}

#endif
#ifdef GUI_GTK

	OsResource *Bitmap::create(Painter *owner) {
		Nat w = src->width();
		Nat h = src->height();

		printf("Created bitmap: %d, %d\n", w, h);

		CairoSurface *surface = owner->surface();
		cairo_surface_t *texture = cairo_surface_create_similar(surface->surface, CAIRO_CONTENT_COLOR_ALPHA, w, h);
		// cairo_surface_t *image = cairo_surface_map_to_image(texture, NULL);

		// byte *src = this->src->buffer();
		// Nat srcStride = this->src->stride();
		// byte *dest = cairo_image_surface_get_data(image);
		// Nat destStride = cairo_image_surface_get_stride(image);
		// cairo_format_t fmt = cairo_image_surface_get_format(image);
		// if (fmt != CAIRO_FORMAT_ARGB32)
		// 	throw new (this) GuiError(S("Invalid format from Cairo. Expected ARGB32."));

		// for (Nat y = 0; y < h; y++) {
		// 	for (Nat x = 0; x < w; x++) {
		// 		nat *d = (nat *)(dest + destStride*y + x*4);
		// 		byte *s = src + srcStride*y + x*4;

		// 		// Convert to premultiplied alpha.
		// 		float a = float(s[3]) / 255.0f;
		// 		nat pixel = 0;
		// 		pixel |= (nat(s[3] * 1) & 0xFF) << 24; // A
		// 		pixel |= (nat(s[0] * a) & 0xFF) << 16; // R
		// 		pixel |= (nat(s[1] * a) & 0xFF) << 8; // G
		// 		pixel |= (nat(s[2] * a) & 0xFF) << 0; // B
		// 		*d = pixel;
		// 	}
		// }
		// cairo_surface_mark_dirty(image);
		// cairo_surface_unmap_image(texture, image);
		cairo_surface_mark_dirty(texture);
		return texture;
	}

#endif

	Size Bitmap::size() {
		// TODO: Respect screen vs image dpi?
		return src->size();
	}

}
