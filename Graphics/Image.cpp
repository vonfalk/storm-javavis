#include "stdafx.h"
#include "Image.h"
#include "StreamWrapper.h"

#include <wincodec.h>
#pragma comment (lib, "Windowscodecs.lib")


namespace graphics {

	namespace wic {
		Image *loadImage(storm::IStream *from, const wchar *&error);
	}


	// check the header in the stream
	// zeroTerm - Zero-terminated string in the file?
	bool checkHeader(storm::IStream *file, char *header, bool zeroTerm) {
		nat len = strlen(header);
		if (zeroTerm)
			len++;

		Buffer buffer = file->peek(storm::buffer(file->engine(), len));
		if (!buffer.full()) {
			return false;
		}

		for (nat i = 0; i < len; i++) {
			if (byte(buffer[i]) != byte(header[i])) {
				return false;
			}
		}

		return true;
	}

	Image *loadImage(Url *file) {
		return loadImage(file->read());
	}

	Image *loadImage(storm::IStream *from) {
		Image *loaded = null;
		const wchar *error = L"The image file type was not recognized.";

		if (checkHeader(from, "\xff\xd8", false)) {
			// JPEG file
			loaded = wic::loadImage(from, error);
		} else if (checkHeader(from, "JFIF", false)) {
			// JFIF file
			loaded = wic::loadImage(from, error);
		} else if (checkHeader(from, "Exif", false)) {
			// Exif file
			loaded = wic::loadImage(from, error);
		} else if (checkHeader(from, "\x89PNG", false)) {
			// PNG file
			loaded = wic::loadImage(from, error);
		} else if (checkHeader(from, "BM", false)) {
			// BMP file
			loaded = wic::loadImage(from, error);
		}

		if (!loaded)
			throw ImageLoadError(error);

		return loaded;
	}


	//////////////////////////////////////////////////////////////////////////
	// Format-specific decoders
	//////////////////////////////////////////////////////////////////////////
	// WIC decoder, supports many formats

	namespace wic {

		HRESULT convertTexture(IWICImagingFactory *factory, IWICBitmapSource *src, IWICBitmapSource **dest, const wchar *&error) {
			IWICFormatConverter *converter = null;
			HRESULT r = factory->CreateFormatConverter(&converter);
			const wchar_t *err = L"Failed to create a format converter";

			if (SUCCEEDED(r)) {
				r = converter->Initialize(src, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
				err = L"Failed to convert the picture";
			}

			if (SUCCEEDED(r)) {
				r = converter->QueryInterface(IID_IWICBitmapSource, (void **)dest);
				err = L"Failed to get the converted picture";
			}

			release(converter);

			if (FAILED(r))
				error = err;

			return r;
		}

		Image *createTexture(Engine &e, IWICImagingFactory *factory, IWICBitmapSource *from, const wchar *&error) {
			IWICBitmapSource *converted = null;
			HRESULT r = convertTexture(factory, from, &converted, error);
			if (FAILED(r))
				return null;

			//NOTE: From here it is assumed that the format of "converted" is 32bpp, BGRA

			nat width, height;
			converted->GetSize(&width, &height);

			Image *image = new (e) Image(width, height);
			r = converted->CopyPixels(NULL, image->stride(), image->bufferSize(), image->buffer());

			// Swap red and blue...
			for (nat y = 0; y < height; y++) {
				for (nat x = 0; x < width; x++) {
					byte *px = image->buffer(x, y);
					swap(px[0], px[2]);
				}
			}

			if (FAILED(r))
				error = L"Failed to copy bitmap data";

			release(converted);
			return image;
		}

		Image *loadImage(storm::IStream *from, const wchar *&err) {
			IWICImagingFactory *wicFactory = null;
			HRESULT r = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
			err = L"Failed to initialize Windows Imaging Components";

			IWICStream *wicStream = null;
			storm::RIStream *src = from->randomAccess();
			StreamWrapper *fromStream = StreamWrapper::create(&src);
			if (SUCCEEDED(r)) {
				r = wicFactory->CreateStream(&wicStream);
				err = L"Failed to create a WIC stream";
			}

			if (SUCCEEDED(r)) {
				r = wicStream->InitializeFromIStream(fromStream);
				err = L"Failed to initialize the WIC stream";
			}

			IWICBitmapDecoder *decoder = null;
			if (SUCCEEDED(r)) {
				r = wicFactory->CreateDecoderFromStream(wicStream, NULL, WICDecodeMetadataCacheOnLoad, &decoder);
				err = L"Failed to create a WIC decoder (possibly corrupt image)";
			}

			IWICBitmapFrameDecode *frame = null;
			if (SUCCEEDED(r)) {
				r = decoder->GetFrame(0, &frame);
				err = L"Failed to get bitmap data";
			}

			IWICBitmapSource *bitmapSource = null;
			if (SUCCEEDED(r)) {
				r = frame->QueryInterface(IID_IWICBitmapSource, (void **)&bitmapSource);
				err = L"The object was not a BitmapSource object";
			}

			Image *result = null;
			// Convert the data and add it to the texture
			if (SUCCEEDED(r)) {
				result = createTexture(from->engine(), wicFactory, bitmapSource, err);
			}

			release(bitmapSource);
			release(frame);
			release(decoder);
			release(wicStream);
			release(fromStream);
			release(wicFactory);

			return result;
		}

	}

}
