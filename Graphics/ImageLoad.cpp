#include "stdafx.h"
#include "ImageLoad.h"

#ifndef WINDOWS

// TODO: We should probably add a license for libpng, even though we tecnically do not distribute it.
#include <png.h>

// #include <jpeglib.h>


namespace graphics {

	/**
	 * Image loading for platforms other than Windows.
	 */


	// IO callbacks from libpng.
	static void pngRead(png_structp png, byte *to, size_t length) {
		IStream *src = (IStream*)png_get_io_ptr(png);
		Buffer out = src->read(length);
		if (out.count() > 0)
			memcpy(to, out.dataPtr(), out.count());
	}

	Image *loadPng(IStream *from, const wchar *&error) {
		// Anchor 'from' on the stack so that the GC does not move it.
		IStream *volatile anchor = null;
		atomicWrite(anchor, from);

		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		png_infop info = null;
		error = S("Failed to initialize libpng");

		// Custom error handling:
		if (setjmp(png_jmpbuf(png))) {
			// Error. Clean up and exit.
			png_destroy_read_struct(&png, &info, NULL);
			error = S("Internal error in libpng.");
			return null;
		}

		png_set_read_fn(png, (void *)anchor, &pngRead);

		if (png) {
			info = png_create_info_struct(png);
			error = S("Failed to create info struct.");
		}

		png_uint_32 ok = 0;
		png_uint_32 width = 0, height = 0;
		int depth = 0, type = 0;
		if (info) {
			png_read_info(png, info);
			ok = png_get_IHDR(png, info, &width, &height, &depth, &type, NULL, NULL, NULL);
			error = S("Failed to read PNG header.");
		}

		Image *output = null;
		if (ok) {
			// Fix the format.
			if (type == PNG_COLOR_TYPE_PALETTE)
				png_set_palette_to_rgb(png);
			if (type == PNG_COLOR_TYPE_GRAY && depth < 8)
				png_set_expand_gray_1_2_4_to_8(png);
			if (png_get_valid(png, info, PNG_INFO_tRNS))
				png_set_tRNS_to_alpha(png);
			if (depth == 16)
				png_set_strip_16(png);
			if (depth < 8) {
				png_color_8p bit;
				if (png_get_sBIT(png, info, &bit))
					png_set_shift(png, bit);
			}
			if (!(type & PNG_COLOR_MASK_ALPHA))
				png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);

			png_read_update_info(png, info);

			// Read the PNG file itself!
			output = new (from) Image(Nat(width), Nat(height));

			// Make sure to pin the internal array by storing a pointer to it on the stack.
			byte *volatile buffer = null;
			atomicWrite(buffer, output->buffer());
			Nat stride = output->stride();

			// Create a GcArray where we can store pointers to inside 'buffer'. Note: it is not
			// scanned, as that would lead to pointers to the middle of objects. Since the buffer is
			// pinned on the stack, it can not move so this should be fine anyway.
			GcArray<byte *> *rows = runtime::allocArray<byte *>(output->engine(), &sizeArrayType, height);
			for (Nat i = 0; i < height; i++)
				rows->v[i] = buffer + stride*i;

			png_read_image(png, rows->v);
		}

		// Clean up any structures we allocated.
		png_destroy_read_struct(&png, &info, NULL);

		return output;
	}

	Image *loadJpeg(IStream *from, const wchar *&error) {
		error = S("JPEG files are not supported yet.");
		return null;
	}

	Image *loadBmp(IStream *from, const wchar *&error) {
		error = S("BMP files are not supported yet.");
		return null;
	}

}

#endif
