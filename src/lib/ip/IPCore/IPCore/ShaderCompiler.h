//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPCore__ShaderCompiler__h__
#define __IPCore__ShaderCompiler__h__
#include <string>

namespace IPCore
{
    namespace Shader
    {

        //
        //  What a compiled Shader::Function holds for the renderer backend
        //  (GL: the shader object). Deleting it releases it.
        //

        struct FunctionState
        {
            virtual ~FunctionState() = default;
        };

        //
        //  class Shader::Compiler
        //
        //  Compiles the GLSL that Shader::Function generates. The renderer
        //  backend provides it (setCompiler()). The version strings choose
        //  the GLSL dialect that Function generates; they are empty until
        //  the backend can query its device.
        //

        class Compiler
        {
        public:
            virtual ~Compiler() = default;

            virtual std::string apiVersion() const = 0;      // GL: GL_VERSION
            virtual std::string languageVersion() const = 0; // GL: GL_SHADING_LANGUAGE_VERSION

            //
            //  Compiles a fragment shader made of header followed by
            //  source. Returns null if it fails. log gets the compiler's
            //  messages either way.
            //

            virtual FunctionState* compile(const std::string& header, const std::string& source, std::string& log) = 0;
        };

        void setCompiler(Compiler*);
        Compiler* compiler();

    } // namespace Shader
} // namespace IPCore

#endif // __IPCore__ShaderCompiler__h__
