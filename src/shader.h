#include <stdbool.h>
#include <stdint.h>

typedef struct ShaderCompiler {
} ShaderCompiler;

typedef struct ShaderSPIRV {
    uint32_t* words;
    uint32_t size;
} ShaderSPIRV;

bool shader_compiler_init(ShaderCompiler* compiler); 
void shader_compiler_destroy(ShaderCompiler* compiler);

typedef enum {
    SHADER_VERTEX = 0,
    SHADER_FRAGMENT,
} ShaderStage;

bool shader_spirv_init(
    const char* file,
    const ShaderStage stage,
    const ShaderCompiler* compiler,
    ShaderSPIRV* spirv);
void shader_spirv_destroy(ShaderSPIRV* spirv);
