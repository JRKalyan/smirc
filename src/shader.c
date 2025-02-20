#include "shader.h"

// TODO eventually the ability to generically call other processes
// (Like with SDL_CreateProcess) could be used to call a glsl compiler,
// rather than baking glsl into the default SMIRC platform.
// This functionality would be desired in general for SMIRC extensions
// to make use of existing, non-composable software.
// There are other considerations:
// - glslang is not very well documented. Not prohibitively so, as the
//   source is readable, but it seems to have a tiny incomplete example and
//   that is it.
// - With this approach we eventually would require statically linking
//   it as a dependency, complicating build process. To keep SMIRC
//   minimal we could pre-build minimal shaders, not ship a glsl compiler,
//   and make an extension which uses the generic sub process capabilities
//   of SMIRC to invoke compilation. Or write a different SPIRV frontend...!
// - GLSLANG C interface can seg fault or has undefined behaviour instead of
//   propagating errors upwards. It looks to be a significant task to add in
//   error handling at every level of that code, that I am not equipped for and
//   may not be accepted as a requirement by the maintainers. This is a
//   requirement of the C code in SMIRC that I have written up until using this
//   dependency.
// + SMIRC should be designed with swappable platforms. Adding builtin support
//   for various platform features, such as a shader compiler, is not against
//   the design philosophy inherently. Minimal isn't necessarily better, we just
//   don't want too much functionality in the 'default' platform; but
//   alternative platforms can be generated to serve a purpose with less demands.

// TODO investigate slang. The simplicity of the glslang compiler API compared
// to the slang compiler API is desirable for now.

#include <glslang/Include/glslang_c_interface.h>

// TODO are defaults fine? Should I be pulling from device information to
// compile shaders optimally?
#include <glslang/Public/resource_limits_c.h>

#include <stdio.h>

bool shader_compiler_init(ShaderCompiler* compiler) {
    (void)compiler;
    static uint32_t glslang_initialize_count = 0;

    if (glslang_initialize_count > 0) {
        // Only allowed to call glslang_initialize_process once per process
        return false;
    }

    glslang_initialize_count++;
    glslang_initialize_process();
    return true;
}

void shader_compiler_destroy(ShaderCompiler* compiler) {
    (void)compiler;
    glslang_finalize_process();
}

bool shader_spirv_init_from_program(
    const glslang_input_t* input,
    glslang_program_t* program,
    ShaderSPIRV* spirv) {
    const bool result = glslang_program_link(
        program,
        GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT);

    if (result == false) {
        // TODO formalize logging
        printf("%s\n", glslang_program_get_info_log(program));
        printf("%s\n", glslang_program_get_info_debug_log(program));
        return false;
    }

    glslang_program_SPIRV_generate(program, input->stage);

    spirv->size = glslang_program_SPIRV_get_size(program) * sizeof(uint32_t);
    spirv->words = malloc(spirv->size);

    if (spirv->words == NULL) {
        return false;
    }

    glslang_program_SPIRV_get(program, spirv->words);

    const char* msgs = glslang_program_SPIRV_get_messages(program);
    if (msgs != NULL) {
        printf("MSGS: %s", msgs);
        return false;
    }

    return true;
}

bool shader_spirv_init_from_shader(
    glslang_shader_t* shader,
    const glslang_input_t* input,
    ShaderSPIRV* spirv) {
    if (glslang_shader_preprocess(shader, input) == false) {
        printf("%s\n", glslang_shader_get_info_log(shader));
        printf("%s\n", glslang_shader_get_info_debug_log(shader));
        return false;
    }

    if (glslang_shader_parse(shader, input) == false) {
        printf("%s\n", glslang_shader_get_info_log(shader));
        printf("%s\n", glslang_shader_get_info_debug_log(shader));
        printf("%s\n", glslang_shader_get_preprocessed_code(shader));
        return false;
    }

    glslang_program_t* program = glslang_program_create();
    if (program == NULL) {
        return false;
    }
    glslang_program_add_shader(program, shader);

    const bool result = shader_spirv_init_from_program(input, program, spirv);


    glslang_program_delete(program);
    return result;
}

bool shader_spirv_init_from_glsl(
    const ShaderCompiler* compiler,
    const ShaderStage stage,
    const char* glsl,
    ShaderSPIRV* spirv) {

    (void)compiler;

    glslang_stage_t glslang_stage;
    switch (stage) {
    case SHADER_VERTEX:
        glslang_stage = GLSLANG_STAGE_VERTEX;
        break;
    case SHADER_FRAGMENT:
        glslang_stage = GLSLANG_STAGE_FRAGMENT;
        break;
    default:
        glslang_stage = GLSLANG_STAGE_VERTEX;
        break;
    }

    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = glslang_stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_6,
        .code = glsl,
        .default_version = 450,
        .default_profile = GLSLANG_NO_PROFILE,
        .forward_compatible = false,
        .force_default_version_and_profile = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource(),
    };

    glslang_shader_t* shader = glslang_shader_create(&input);
    if (shader == NULL) {
        return false;
    }

    const bool result = shader_spirv_init_from_shader(shader, &input, spirv);
    glslang_shader_delete(shader);

    return result;
}

void shader_spirv_destroy(ShaderSPIRV* spirv) {
    free(spirv->words);
}

// TODO formalize file io
char* read_file(const char* name, size_t* out_size) {
    FILE* file = fopen(name, "r");
    if (file == NULL) {
        printf("OPEN FAILURE %s", name);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    size_t size = ftell(file);
    printf("FILE: %s | SIZE: %d\n", name, (int)size);
    rewind(file);

    if (size == 0) {
        return NULL;
    }

    char* buf = malloc(sizeof(char) * (size + 1));
    if (buf == NULL) {
        fclose(file);
        return NULL;
    }

    if (fread(buf, 1, size, file) < size) {
        free(buf);
        fclose(file);
        return NULL;
    }
    fclose(file);

    if (out_size != NULL) {
        *out_size = size;
    }
    buf[size] = 0;
    return buf;
}

bool shader_spirv_init(
    const char* file,
    const ShaderStage stage,
    const ShaderCompiler* compiler,
    ShaderSPIRV* spirv) {

    char* shader_source = read_file(file, NULL);
    if (shader_source == NULL) {
        return false;
    }
    const bool res = shader_spirv_init_from_glsl(compiler, stage, shader_source, spirv);
    free(shader_source);

    return res;
}
