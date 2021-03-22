#include "stdafx.h"
#include "LocalShader.h"

#ifdef GUI_ENABLE_SKIA

namespace gui {

	LocalShader::LocalShader(sk_sp<SkShader> proxy, const SkMatrix &matrix)
		: proxy(proxy), matrix(matrix) {}

	std::unique_ptr<GrFragmentProcessor> LocalShader::asFragmentProcessor(const GrFPArgs &args) const {
		return base()->asFragmentProcessor(GrFPArgs::WithPreLocalMatrix(args, matrix));
	}

	sk_sp<SkFlattenable> LocalShader::CreateProc(SkReadBuffer &buffer) {
		SkMatrix local;
		buffer.readMatrix(&local);
		auto base{buffer.readShader()};
		if (!base)
			return null;
		return sk_make_sp<LocalShader>(base, local);
	}

	void LocalShader::flatten(SkWriteBuffer &to) const {
		to.writeMatrix(matrix);
		to.writeFlattenable(proxy.get());
	}

	SkImage *LocalShader::onIsAImage(SkMatrix *matrix, SkTileMode *mode) const {
		SkMatrix imageMatrix;
		SkImage *image = proxy->isAImage(&imageMatrix, mode);
		if (image && matrix)
			*matrix = SkMatrix::Concat(imageMatrix, this->matrix);
		return image;
	}

	bool LocalShader::onAppendStages(const SkStageRec &rec) const {
		SkTCopyOnFirstWrite<SkMatrix> l(matrix);
		if (rec.fLocalM)
			l.writable()->preConcat(*rec.fLocalM);

		SkStageRec newRec = rec;
		newRec.fLocalM = l;
		return base()->appendStages(newRec);
	}

	skvm::Color LocalShader::onProgram(skvm::Builder *b, skvm::Coord device, skvm::Coord local, skvm::Color paint,
									const SkMatrixProvider &mp, const SkMatrix *localMatrix,
									SkFilterQuality quality, const SkColorInfo &dst,
									skvm::Uniforms *uniforms, SkArenaAlloc *arena) const {
		SkTCopyOnFirstWrite<SkMatrix> l(this->matrix);
		if (localMatrix)
			l.writable()->preConcat(*localMatrix);

		return base()->program(b, device, local, paint, mp, l.get(), quality, dst, uniforms, arena);
	}

}

#endif
