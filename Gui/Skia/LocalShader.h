#pragma once
#include "Skia.h"

#ifdef GUI_ENABLE_SKIA

namespace gui {

	/**
	 * A version of the SkLocalShader.
	 *
	 * This is so that we can freely modify the transform without re-creating objects all the
	 * time. While this is possible to some extent in Skia, there is seemingly no good way to simply
	 * set the shader's matrix to some value. We can only append the matrix to the previous one.
	 */
	class LocalShader : public SkShaderBase {
	public:
		// Create a local shader as a proxy for some other object.
		LocalShader(sk_sp<SkShader> proxy, const SkMatrix &matrix);

		// Overrides for some needed functions.
		GradientType asAGradient(GradientInfo *info) const override {
			return proxy->asAGradient(info);
		}

		// Fragment processing.
		std::unique_ptr<GrFragmentProcessor> asFragmentProcessor(const GrFPArgs &) const override;

		// Local matrix.
		SkMatrix matrix;

	protected:
		// Flatten.
		void flatten(SkWriteBuffer &to) const override;

		// To an image.
		SkImage *onIsAImage(SkMatrix *matrix, SkTileMode *mode) const override;

		// Append stages.
		bool onAppendStages(const SkStageRec &) const override;

		// (execute?) shader program.
		skvm::Color onProgram(skvm::Builder *, skvm::Coord device, skvm::Coord local, skvm::Color paint,
							const SkMatrixProvider &, const SkMatrix *localMatrix,
							SkFilterQuality quality, const SkColorInfo &dst,
							skvm::Uniforms *uniforms, SkArenaAlloc *arena) const override;

	private:
		SK_FLATTENABLE_HOOKS(LocalShader);

		// Shader we're wrapping.
		sk_sp<SkShader> proxy;

		// Get the proxy shader as a SkShaderBase
		SkShaderBase *base() const {
			return static_cast<SkShaderBase *>(proxy.get());
		}
	};

}

#endif
