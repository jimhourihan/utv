//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// sampler2DRect is core from GLSL 1.40, and texture2DRect() is replaced by
// texture(). Core profile contexts don't list the extension.
#if __VERSION__ < 140
#extension GL_ARB_texture_rectangle : require
#else
#define texture2DRect texture
#endif

#if __VERSION__ >= 150
out vec4 FragColor; 
#define FRAGCOLOR FragColor
#else
#define FRAGCOLOR gl_FragColor
#define in varying
#endif

uniform sampler2DRect texture0;
in vec2 TexCoord0;

void main() 
{ 
    FRAGCOLOR = texture2DRect(texture0, TexCoord0); 
}

