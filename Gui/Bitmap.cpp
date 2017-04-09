#include "stdafx.h"
#include "Bitmap.h"
#include "Painter.h"

namespace gui {

	Bitmap::Bitmap(Image *img) : src(img) {}

	void Bitmap::create(Painter *owner, ID2D1Resource **out) {
		nat w = src->width();
		nat h = src->height();

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
		for (nat y = 0; y < h; y++) {
			nat base = stride * y;
			for (nat x = 0; x < w; x++) {
				nat p = base + x * 4;
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

	Size Bitmap::size() {
		// TODO: Respect screen vs image dpi?
		return src->size();
	}

}
