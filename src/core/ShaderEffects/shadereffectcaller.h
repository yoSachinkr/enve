#ifndef SHADEREFFECTCALLER_H
#define SHADEREFFECTCALLER_H
#include "RasterEffects/rastereffectcaller.h"
#include "shadereffectprogram.h"

class ShaderEffectCaller : public RasterEffectCaller {
    e_OBJECT
public:
    ShaderEffectCaller(const QMargins& margin,
                       const ShaderEffectProgram& program,
                       const UniformSpecifiers& uniformSpecifiers);

    void processGpu(QGL33 * const gl,
                    GpuRenderTools& renderTools,
                    GpuRenderData& data);
private:
    void setupProgram(QGL33 * const gl, QJSEngine& engine,
                      const GLfloat gPosX, const GLfloat gPosY);

    const ShaderEffectProgram mProgram;
    const UniformSpecifiers mUniformSpecifiers;
};


#endif // SHADEREFFECTCALLER_H