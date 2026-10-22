//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPRendererGLCompat__GLShaderCompiler__h__
#define __IPRendererGLCompat__GLShaderCompiler__h__
#include <IPCore/ShaderCompiler.h>
#include <IPCore/ShaderFunction.h>
#include <TwkGLF/GL.h>

namespace IPCore
{
    namespace Shader
    {

        //
        //  A compiled Shader::Function: the GL shader object, deleted with
        //  it.
        //

        struct GLFunctionState : public FunctionState
        {
            explicit GLFunctionState(GLuint s)
                : shader(s)
            {
            }

            ~GLFunctionState() override;

            GLuint shader;
        };

        //
        //  The GL shader object of a compiled function, or 0
        //

        inline GLuint glShader(const Function* F)
        {
            const GLFunctionState* state = static_cast<const GLFunctionState*>(F->state());
            return state ? state->shader : 0;
        }

        //
        //  Compiles fragment shaders in the current GL context
        //

        class GLShaderCompiler : public Compiler
        {
        public:
            std::string apiVersion() const override;
            std::string languageVersion() const override;
            FunctionState* compile(const std::string& header, const std::string& source, std::string& log) override;
        };

    } // namespace Shader
} // namespace IPCore

#endif // __IPRendererGLCompat__GLShaderCompiler__h__
