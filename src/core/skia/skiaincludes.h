#ifndef SKIAINCLUDES_H
#define SKIAINCLUDES_H

#include "skiadefines.h"

#undef foreach

#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrContext.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPixelRef.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkPathMeasure.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageFilter.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkColorFilter.h"

#include "include/pathops/SkPathOps.h"

#include "include/utils/SkRandom.h"
#include "include/utils/SkTextUtils.h"

#include "include/core/SkStrokeRec.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkDiscretePathEffect.h"

#include "include/effects/SkGradientShader.h"
#include "include/effects/SkBlurImageFilter.h"
#include "include/effects/SkDropShadowImageFilter.h"

#include "src/gpu/gl/GrGLDefines.h"

#include "include/gpu/gl/GrGLTypes.h"
#include "include/gpu/gl/GrGLFunctions.h"
#include "include/gpu/gl/GrGLInterface.h"

#include "third_party/externals/sdl/include/SDL.h"

#include <QtGui/qopengl.h>

#include "src/core/SkPointPriv.h"
#include "src/core/SkStroke.h"
#include "src/core/SkGeometry.h"

#define foreach Q_FOREACH

#endif // SKIAINCLUDES_H