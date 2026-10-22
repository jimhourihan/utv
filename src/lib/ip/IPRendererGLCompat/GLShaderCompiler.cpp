//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#include <IPRendererGLCompat/GLShaderCompiler.h>
#include <vector>

namespace IPCore
{
    namespace Shader
    {
        using namespace std;

        GLFunctionState::~GLFunctionState()
        {
            if (shader)
                glDeleteShader(shader);
        }

        string GLShaderCompiler::apiVersion() const { return TwkGLF::safeGLGetString(GL_VERSION); }

        string GLShaderCompiler::languageVersion() const { return TwkGLF::safeGLGetString(GL_SHADING_LANGUAGE_VERSION); }

        FunctionState* GLShaderCompiler::compile(const string& header, const string& source, string& log)
        {
            GLuint shaderID = glCreateShader(GL_FRAGMENT_SHADER);
            int status;
            int logsize;

            const char* src[2];
            src[0] = header.c_str();
            src[1] = source.c_str();

            glShaderSource(shaderID, 2, src, 0);
            glCompileShader(shaderID);
            glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logsize);

            log.clear();

            if (logsize > 1)
            {
                vector<char> buffer(logsize + 1);
                GLsizei rlen;
                glGetShaderInfoLog(shaderID, logsize, &rlen, &buffer.front());
                log = &buffer.front();
            }

            if (status != GL_TRUE)
            {
                glDeleteShader(shaderID);
                return nullptr;
            }

            return new GLFunctionState(shaderID);
        }

    } // namespace Shader
} // namespace IPCore
