//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPRendererGLCompat/PaintCommandGL.h>
#include <TwkMath/Function.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLPipeline.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/BasicGLProgram.h>
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Frustum.h>
#include <TwkGLText/TwkGLText.h>
#include <TwkGLF/GLText.h>
#include <stl_ext/string_algo.h>

namespace IPCore
{
    namespace Paint
    {
        using namespace std;
        using namespace TwkMath;
        using namespace TwkPaint;
        using namespace TwkFB;
        using namespace TwkGLText;
        using namespace TwkGLF;

        void Font::init() const
        {
            if (useFTGLText())
            {
                if (m_name == "")
                    GLtext::init();
                else
                    GLtext::init(m_name.c_str());
            }
            m_initialized = true;
        }

        void Font::setSize(unsigned int s) const
        {
            assert(m_initialized);
            m_size = s;
            if (useFTGLText())
                GLtext::size(s);
            else
                m_text = TwkText::Font::load(m_name, s);
        }

        void Font::setColor(float r, float g, float b, float a) const
        {
            if (useFTGLText())
                GLtext::color(r, g, b, a);
        }

        Box2f Font::boundsNL(const string& text, float space) const
        {
            return useFTGLText() ? GLtext::boundsNL(text.c_str(), space) : m_text.boundsNL(text, space);
        }

        pair<float, float> Font::computeText(const string& text, string& t, float space, string origin) const
        {
            assert(m_initialized);

            vector<string> buffer;
            stl_ext::tokenize(buffer, text, "\n\r");
            string pipes;
            for (int i = buffer.size() - 1; i >= 0; i--)
            {
                if (i != buffer.size() - 1)
                {
                    t += "\n";
                    pipes += "\n";
                }
                t += buffer[i];
                pipes += "|,~`'\"yqj";
            }

            Box2f pb = boundsNL(pipes, space);
            float h = pb.min.y - pb.max.y - globalDescenderHeight();
            Box2f tb = boundsNL(t, space);
            float w = -tb.max.x;

            float xoffset, yoffset;
            if (origin == "top-left")
            {
                xoffset = 0;
                yoffset = h;
            }
            else if (origin == "top-center")
            {
                xoffset = w / 2.0;
                yoffset = h;
            }
            else if (origin == "top-right")
            {
                xoffset = w;
                yoffset = h;
            }
            else if (origin == "center-left")
            {
                xoffset = 0;
                yoffset = h / 2.0;
            }
            else if (origin == "center-center")
            {
                xoffset = w / 2.0;
                yoffset = h / 2.0;
            }
            else if (origin == "center-right")
            {
                xoffset = w;
                yoffset = h / 2.0;
            }
            else if (origin == "bottom-left")
            {
                xoffset = 0;
                yoffset = 0;
            }
            else if (origin == "bottom-center")
            {
                xoffset = w / 2.0;
                yoffset = 0;
            }
            else if (origin == "bottom-right")
            {
                xoffset = w;
                yoffset = 0;
            }
            else // old behavior default
            {
                xoffset = 0;
                yoffset = globalAscenderHeight() - (pb.max.y - pb.min.y);
            }
            return make_pair(xoffset, yoffset);
        }

        float Font::globalAscenderHeight() const { return useFTGLText() ? GLtext::globalAscenderHeight() : m_text.ascender(); }

        float Font::globalDescenderHeight() const { return useFTGLText() ? GLtext::globalDescenderHeight() : m_text.descender(); }

        namespace
        {

            //
            //  The vertex layout of PolyLine::build()'s primitiveData
            //

            void primitivesFor(const PolyLine& cmd, PrimitiveData& primitives, std::vector<VertexAttribute>& primitiveAttributes)
            {
                const size_t vertexno = cmd.primitiveVertexCount;

                primitives.m_dataBuffer = &(cmd.primitiveData[0]);
                primitives.m_indexBuffer = cmd.mode == PolyLine::TessellateMode ? 0 : &(cmd.primitiveIndices[0]);
                primitives.m_primitiveType = GL_TRIANGLES;
                primitives.m_vertexNo = vertexno;
                primitives.m_primitiveNo = cmd.primitiveTriangleCount;
                primitives.m_size = sizeof(float) * cmd.primitiveData.size();

                primitiveAttributes.clear();
                primitiveAttributes.push_back(VertexAttribute(std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 0)); // texture
                primitiveAttributes.push_back(
                    VertexAttribute(std::string("in_Position"), GL_FLOAT, 2, 2 * vertexno * sizeof(float), 0)); // vertex
                if (cmd.mode == PolyLine::GradientScaleMode)
                {
                    primitiveAttributes.push_back(
                        VertexAttribute(std::string("in_DirectionCoord"), GL_FLOAT, 1, 4 * vertexno * sizeof(float), 0)); // direction coord
                }
                else if (cmd.mode == PolyLine::TessellateMode)
                {
                    primitiveAttributes.push_back(VertexAttribute(std::string("in_UniformColor"), GL_FLOAT, 4, 4 * vertexno * sizeof(float),
                                                                  0)); // color for each triangle
                }
            }

            //
            // this is for rv3 backwards compatability
            // in rv4 we updated the mechanism for over mode (the basic paint
            // stroke), and the look different from rv3.
            //
            void executePolyLineOldOverMode(const PolyLine& cmd, PrimitiveData& primitives,
                                            std::vector<VertexAttribute>& primitiveAttributes, CommandContext& context)
            {
                const GLFBO* textureFBO = context.currentTexture;
                const GLFBO* currentFBO = context.currentRender;
                const float w = textureFBO->width();
                const float h = textureFBO->height();

                GLState* glState = context.glState;

                currentFBO->bind();
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                TWK_GLDEBUG;
                glClear(GL_COLOR_BUFFER_BIT);
                TWK_GLDEBUG;

                // Draw a quad with the existing renders as the background
                GLPipeline* glPipeline = glState->useGLProgram(textureRectGLProgram());

                // transforms
                Mat44f identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

                Frustumf f;
                f.window(0, w - 1, 0, h - 1, -1, 1, true);
                Mat44f projMat = f.matrix();

                glPipeline->setModelview(identity);
                glPipeline->setProjection(projMat);
                glPipeline->setViewport(0, 0, w, h);

                GLint id = 0;
                glPipeline->setUniformInt("texture0", 1, &id);
                glActiveTexture(GL_TEXTURE0);
                textureFBO->bindColorTexture(0);

                //
                //  NOTE: rectangle coords are [0,w] x [0,h]
                //  *not* [0,w-1] x [0,h-1]
                //
                float data[] = {0, 0, 0, 0, w, 0, w - 1, 0, w, h, w - 1, h - 1, 0, h, 0, h - 1};
                PrimitiveData buffer(data, NULL, GL_TRIANGLE_FAN, 4, 1, 16 * sizeof(float));
                std::vector<VertexAttribute> attributeInfo;

                attributeInfo.push_back(VertexAttribute(std::string("in_Position"), GL_FLOAT, 2, 2 * sizeof(float), 4 * sizeof(float)));

                attributeInfo.push_back(VertexAttribute(std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 4 * sizeof(float)));

                RenderPrimitives renderprimitives(glState->activeGLProgram(), buffer, attributeInfo, glState->vboList());

                renderprimitives.setupAndRender();

                textureFBO->unbindColorTexture();

                //  do paint
                if (cmd.brush == "gauss")
                {
                    glPipeline = glState->useGLProgram(softPaintOldReplaceGLProgram());
                }
                else
                {
                    glPipeline = glState->useGLProgram(paintOldReplaceGLProgram());
                }

                glPipeline->setProjection(context.projMatrix);
                glPipeline->setModelview(context.modelviewMatrix);
                glPipeline->setViewport(0, 0, w, h);

                Command::Color pcolor = cmd.color;
                glPipeline->setUniformFloat("uniformColor", 4, &(pcolor[0]));

                // draw
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                RenderPrimitives renderprimitives2(glState->activeGLProgram(), primitives, primitiveAttributes, glState->vboList());
                renderprimitives2.setupAndRender();

                // cleanup
                currentFBO->unbind();

                glDisable(GL_BLEND);
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE0);
            }

            void executePolyLine(const PolyLine& cmd, CommandContext& context)
            {
                if (!cmd.npoints)
                    return;

                PolyLine::HashValue newid = cmd.hashValue();
                if (!cmd.built || cmd.idhash != newid)
                {
                    cmd.idhash = newid;
                    cmd.build();
                }

                PrimitiveData primitives;
                std::vector<VertexAttribute> primitiveAttributes;
                primitivesFor(cmd, primitives, primitiveAttributes);

                if (cmd.version < 3 && cmd.mode == PolyLine::OverMode)
                {
                    executePolyLineOldOverMode(cmd, primitives, primitiveAttributes, context);
                    return;
                }

                const GLFBO* originalFBO = context.initialRender;
                const GLFBO* textureFBO = context.currentTexture;
                const GLFBO* currentFBO = context.currentRender;
                const float w = textureFBO->width();
                const float h = textureFBO->height();

                //////////////////////////////////////////////////////////////////////////
                // first render the background
                //////////////////////////////////////////////////////////////////////////
                GLState* glState = context.glState;
                currentFBO->bind();

                GLPipeline* glPipeline = glState->useGLProgram(textureRectGLProgram());
                Mat44f identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
                Frustumf f;
                f.window(0, w - 1, 0, h - 1, -1, 1, true);
                Mat44f projMat = f.matrix();

                glPipeline->setModelview(identity);
                glPipeline->setProjection(projMat);
                glPipeline->setViewport(0, 0, w, h);

                int id = 0;
                glPipeline->setUniformInt("texture0", 1, &id);
                glActiveTexture(GL_TEXTURE0);
                textureFBO->bindColorTexture(0);

                glDisable(GL_BLEND);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                TWK_GLDEBUG;
                glClear(GL_COLOR_BUFFER_BIT);
                TWK_GLDEBUG;
                //  NOTE: rectangle coords are [0,w] x [0,h]
                //  *not* [0,w-1] x [0,h-1]
                float data[] = {0, 0, 0, 0, w, 0, w - 1, 0, w, h, w - 1, h - 1, 0, h, 0, h - 1};
                PrimitiveData buffer(data, NULL, GL_TRIANGLE_FAN, 4, 1, 16 * sizeof(float));
                std::vector<VertexAttribute> attributeInfo2;
                attributeInfo2.push_back(VertexAttribute(std::string("in_Position"), GL_FLOAT, 2, 2 * sizeof(float), 4 * sizeof(float)));

                attributeInfo2.push_back(VertexAttribute(std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 4 * sizeof(float)));
                RenderPrimitives renderprimitives2(glState->activeGLProgram(), buffer, attributeInfo2, glState->vboList());
                renderprimitives2.setupAndRender();

                textureFBO->unbindColorTexture();

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);

                //////////////////////////////////////////////////////////////////////////
                // render paint onto the background
                //////////////////////////////////////////////////////////////////////////
                switch (cmd.mode)
                {
                case PolyLine::EraseMode:
                    if (cmd.brush != "gauss")
                        glPipeline = glState->useGLProgram(paintEraseGLProgram());
                    else
                        glPipeline = glState->useGLProgram(softPaintEraseGLProgram());
                    break;
                case PolyLine::ScaleMode:
                case PolyLine::GradientScaleMode:
                    if (cmd.brush != "gauss")
                        glPipeline = glState->useGLProgram(paintScaleGLProgram());
                    else
                        glPipeline = glState->useGLProgram(softPaintScaleGLProgram());
                    break;
                case PolyLine::CloneMode:
                    if (cmd.brush != "gauss")
                        glPipeline = glState->useGLProgram(paintCloneGLProgram());
                    else
                        glPipeline = glState->useGLProgram(softPaintCloneGLProgram());
                    break;
                case PolyLine::TessellateMode:
                    glPipeline = glState->useGLProgram(paintTessellateGLProgram());
                    break;
                case PolyLine::OverMode:
                default:
                    if (cmd.brush != "gauss")
                        glPipeline = glState->useGLProgram(paintReplaceGLProgram());
                    else
                        glPipeline = glState->useGLProgram(softPaintReplaceGLProgram());
                    break;
                }

                // set transforms
                glPipeline->setProjection(context.projMatrix);
                glPipeline->setModelview(context.modelviewMatrix);
                glPipeline->setViewport(0, 0, w, h);

                // bind textures
                if (cmd.mode == PolyLine::EraseMode)
                {
                    id = 1;
                    glPipeline->setUniformInt("texture0", 1, &id);
                    glActiveTexture(GL_TEXTURE0 + 1);
                    originalFBO->bindColorTexture(0);
                }
                else if (cmd.mode == PolyLine::ScaleMode || cmd.mode == PolyLine::CloneMode)
                {
                    id = 1;
                    glPipeline->setUniformInt("texture0", 1, &id);
                    glActiveTexture(GL_TEXTURE0 + 1);
                    textureFBO->bindColorTexture(0);
                }

                // set uniforms
                Command::Color pcolor;
                const auto* holdCommand = dynamic_cast<const HoldCommand*>(&cmd);
                bool isGhostOn = (holdCommand != nullptr) ? holdCommand->ghostOn : false;
                pcolor = isGhostOn ? holdCommand->ghostColor : cmd.color;

                glPipeline->setUniformFloat("uniformColor", 4, &(pcolor[0]));
                float coffset[2] = {50, 50};
                if (cmd.mode == PolyLine::CloneMode)
                    glPipeline->setUniformFloat("cloneOffset", 2, coffset);
                if (cmd.mode != PolyLine::OverMode)
                {
                    glPipeline->setUniformFloat("texWidth", 1, (float*)&w);
                    glPipeline->setUniformFloat("texHeight", 1, (float*)&h);
                }

                // render
                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(context.stencilBox[0], context.stencilBox[1], context.stencilBox[2] - context.stencilBox[0],
                              context.stencilBox[3] - context.stencilBox[1]);
                }
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                RenderPrimitives renderprimitives3(glState->activeGLProgram(), primitives, primitiveAttributes, glState->vboList());
                renderprimitives3.setupAndRender();
                if (context.hasStencil)
                    glDisable(GL_SCISSOR_TEST);

                // clean up
                if (cmd.mode == PolyLine::EraseMode)
                    originalFBO->unbindColorTexture();
                else if (cmd.mode == PolyLine::ScaleMode || cmd.mode == PolyLine::CloneMode)
                    textureFBO->unbindColorTexture();

                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_BLEND);

                currentFBO->unbind();
                glState->useGLProgram(defaultGLProgram());
            }

            void textSetup(CommandContext& context)
            {
                const GLFBO* fbo = context.currentRender;
                context.currentTexture->copyTo(fbo);
                fbo->bind();

                if (useFTGLText())
                    glEnable(GL_TEXTURE_2D);
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                glActiveTexture(GL_TEXTURE0);
            }

            void textCleanup(CommandContext& context)
            {
                glDisable(GL_BLEND);
                if (useFTGLText())
                    glDisable(GL_TEXTURE_2D);

                const GLFBO* fbo = context.currentRender;
                fbo->unbind();
            }

            void executeText(const Text& cmd, CommandContext& context)
            {

                // render
                const GLFBO* fbo = context.currentRender;

                Font* f = new Font(cmd.font);
                if (!f->initialized())
                    f->init();
                f->setSize(cmd.ptsize);

                Command::Color textColor;
                const auto* holdCommand = dynamic_cast<const HoldCommand*>(&cmd);
                bool isGhostOn = (holdCommand != nullptr) ? holdCommand->ghostOn : false;
                textColor = isGhostOn ? holdCommand->ghostColor : cmd.color;
                f->setColor(textColor.x, textColor.y, textColor.z, textColor.w);

                const float w = fbo->width();
                const float h = fbo->height();

                Mat44f T, S;
                S.makeScale(Vec3f(cmd.scale, cmd.scale, cmd.scale));

                double x = cmd.pos.x / cmd.scale;
                double y = cmd.pos.y / cmd.scale;
                T.makeTranslation(Vec3f(x, y, 0.0f));

                GLState* glState = context.glState;
                const Mat44f modelview = context.modelviewMatrix * S * T;

                string outtext;
                pair<float, float> loc = f->computeText(cmd.text, outtext, cmd.spacing, cmd.origin);

                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(context.stencilBox[0], context.stencilBox[1], context.stencilBox[2] - context.stencilBox[0],
                              context.stencilBox[3] - context.stencilBox[1]);
                }

                if (useFTGLText())
                {
                    GLPipeline* glPipeline = glState->useGLProgram(defaultGLProgram());

                    glPipeline->setProjection(context.projMatrix);
                    glPipeline->setModelview(modelview);

                    glPipeline->useCurrentProjectionGL2();
                    glPipeline->useCurrentModelviewGL2();

                    GLState::FixedFunctionPipeline FFP(glState);
                    FFP.setViewport(0, 0, w, h);

                    GLtext::writeAtNL(loc.first, loc.second, outtext.c_str(), cmd.spacing);
                }
                else
                {
                    glViewport(0, 0, GLsizei(w), GLsizei(h));

                    TwkText::GlyphQuads quads;
                    f->textFont().layoutNL(outtext, loc.first, loc.second, cmd.spacing, quads);
                    drawText(quads, context.projMatrix, modelview, textColor, glState->vboList());
                }

                if (context.hasStencil)
                    glDisable(GL_SCISSOR_TEST);
                delete f;
            }

            void executeQuad(CommandContext& context, float* points, Command::Color color, GLint mode)
            {
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                Command::Color pcolor = color;
                GLState* glState = context.glState;
                GLPipeline* glPipeline = glState->useGLProgram(defaultGLProgram());
                const GLFBO* fbo = context.currentRender;
                int programId = glPipeline->programId();
                const float w = fbo->width();
                const float h = fbo->height();

                glPipeline->setUniformFloat("uniformColor", 4, &(pcolor[0]));
                glPipeline->setProjection(context.projMatrix);
                glPipeline->setModelview(context.modelviewMatrix);
                glPipeline->setViewport(0, 0, w, h);

                context.currentTexture->copyTo(fbo);
                fbo->bind();

                PrimitiveData databuffer((void*)points, NULL, mode, 4, 1, sizeof(float) * 8);
                std::vector<VertexAttribute> attributeInfo;
                attributeInfo.push_back(VertexAttribute(std::string("in_Position"), GL_FLOAT, 2, 0, 0));
                RenderPrimitives renderprimitives(glState->activeGLProgram(), databuffer, attributeInfo, glState->vboList());
                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(context.stencilBox[0], context.stencilBox[1], context.stencilBox[2] - context.stencilBox[0],
                              context.stencilBox[3] - context.stencilBox[1]);
                }
                renderprimitives.setupAndRender();
                if (context.hasStencil)
                    glDisable(GL_SCISSOR_TEST);
                glDisable(GL_BLEND);

                fbo->unbind();
            }

            void executeRectangle(const Rectangle& cmd, CommandContext& context)
            {
                float data[] = {cmd.pos.x,
                                cmd.pos.y,
                                cmd.pos.x + cmd.width,
                                cmd.pos.y,
                                cmd.pos.x + cmd.width,
                                cmd.pos.y + cmd.height,
                                cmd.pos.x,
                                cmd.pos.y + cmd.height};

                executeQuad(context, data, cmd.color, GL_TRIANGLE_FAN);
            }

            void executeQuadCommand(const Quad& cmd, CommandContext& context)
            {
                GLint dm = (cmd.drawMode == Quad::SolidMode) ? GL_TRIANGLE_FAN : GL_LINE_LOOP;

                executeQuad(context, (float*)cmd.points, cmd.color, dm);
            }

            void execute(const Command* cmd, CommandContext& context)
            {
                switch (cmd->getType())
                {
                case Command::PolyLine:
                    executePolyLine(*static_cast<const PolyLine*>(cmd), context);
                    break;
                case Command::Text:
                    executeText(*static_cast<const Text*>(cmd), context);
                    break;
                case Command::Rectangle:
                    executeRectangle(*static_cast<const Rectangle*>(cmd), context);
                    break;
                case Command::Quad:
                    executeQuadCommand(*static_cast<const Quad*>(cmd), context);
                    break;
                default: // PushFrameBuffer, PopFrameBuffer, ExecuteAllBefore draw nothing
                    break;
                }
            }

        } // namespace

        void renderPaintCommands(PaintContext& context)
        {
            //
            // TODO: respect the incoming pixel aspect ratio
            //

            if (context.commands.empty())
                return;

            // fbo contains the render of the current image, only used by erase
            // strokes
            const GLFBO* fbo = context.initialRender;
            const IPImage* root = context.image;
            const size_t curCmdNum = context.commands.size();

            // get ready to pingpong
            const GLFBO* textureFBO = NULL;
            const GLFBO* currentFBO = NULL;
            const GLFBO* tempfbo1 = context.tempRender1; // holds current render
            const GLFBO* tempfbo2 = context.tempRender2;

            size_t count = 0;
            for (size_t i = 0; i < context.commands.size(); count++)
            {
                const Paint::Command* cmd = context.commands[i];
                if (cmd->getType() == Paint::Command::ExecuteAllBefore)
                {
                    i++;
                    continue;
                }

                if (count % 2 == 0)
                {
                    textureFBO = tempfbo1;
                    currentFBO = tempfbo2;
                }
                else
                {
                    textureFBO = tempfbo2;
                    currentFBO = tempfbo1;
                }

                glDisable(GL_STENCIL_TEST);
                TWK_GLDEBUG;
                glDisable(GL_CULL_FACE);
                TWK_GLDEBUG;
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                TWK_GLDEBUG;

                // draw
                const Mat44f& O = root->orientationMatrix;
                const Mat44f& MP = root->placementMatrix;
                const Mat44f I = (O * MP).inverted();
                const Mat44f model = root->imageMatrix;
                const Mat44f proj = root->projectionMatrix;

                CommandContext commandContext(proj, model, fbo, textureFBO, currentFBO, context.glState, context.hasStencil,
                                              context.stencilBox);

                //
                // to speed up the rendering, we execute all consecutive text
                // commands together
                //

                if (cmd->getType() == Paint::Command::Text)
                {
                    size_t notText = 0;
                    // find all consecutive text commands
                    for (notText = i + 1; notText < context.commands.size(); ++notText)
                    {
                        if (context.commands[notText]->getType() != Paint::Command::Text)
                            break;
                    }
                    textSetup(commandContext);
                    if (i < curCmdNum - 1)
                        notText = min(notText, curCmdNum - 1);
                    for (size_t j = i; j < notText; ++j)
                    {
                        execute(context.commands[j], commandContext);
                    }
                    textCleanup(commandContext);
                    i = notText;
                }
                else
                {
                    execute(cmd, commandContext);
                    ++i;
                }
                // Update the cache AFTER command execution
                // We cache up to the second-to-last command in the batch (not the very last, which might still be dragging)
                // count is the number of commands rendered so far (1-indexed after increment)
                // Cache after rendering the second-to-last command: count == curCmdNum - 1
                bool shouldCache = context.updateCache && context.cachedfbo && (curCmdNum > 1) && (count == curCmdNum - 1);

                if (shouldCache)
                {
                    // Always save to cache from tempfbo1 to maintain consistent ping-pong state
                    // If currentFBO is tempfbo2, copy it to tempfbo1 first
                    if (currentFBO != tempfbo1)
                    {
                        currentFBO->copyTo(tempfbo1);
                    }
                    tempfbo1->copyTo(context.cachedfbo);
                    context.cacheUpdated = true;
                }
            }

            context.commandExecuted = count;

            // currentFBO contains the resulting content after rendering these
            // commands
            if (fbo && currentFBO)
                currentFBO->copyTo(fbo);
        }

    } // namespace Paint
} // namespace IPCore
