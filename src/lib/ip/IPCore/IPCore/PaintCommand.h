//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPCore__PaintCommand__h__
#define __IPCore__PaintCommand__h__
#include <TwkMath/Mat44.h>
#include <TwkMath/Color.h>
#include <TwkMath/Box.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkPaint/Path.h>
#include <IPCore/IPImage.h>
#include <vector>

namespace IPCore
{

    namespace Paint
    {

        class Command
        {
        public:
            using Vec4 = TwkMath::Vec4f;
            using Vec3 = TwkMath::Vec3f;
            using Vec2 = TwkMath::Vec2f;
            using Box2 = TwkMath::Box2f;
            using Matrix = TwkMath::Mat44f;
            using Color = TwkMath::Col4f;
            using Path = TwkPaint::Path;

            enum CommandType
            {
                PolyLine,
                Text,
                PushFrameBuffer,
                PopFrameBuffer,
                Rectangle,
                Quad,
                ExecuteAllBefore
            };

            float offset;
            unsigned int version;
            Color color;

            virtual void hash(std::ostream& ostream) const = 0;
            [[nodiscard]] virtual size_t getType() const = 0;

            Command()
                : offset(0)
                , version(3)
                , color(Color(0.0))
            {
            }

            Command(const Command& command) = default;

            virtual ~Command() = default;
        };

        class HoldCommand
        {
        public:
            HoldCommand() = default;
            virtual ~HoldCommand() = default;

            int startFrame{};
            int duration{};
            int eye{-1};

            bool ghostOn{};
            TwkMath::Col4f ghostColor{Command::Color(0.0)};
        };

        class PolyLine : public Command
        {
        public:
            using HashValue = unsigned int;

            enum Mode
            {
                OverMode,
                EraseMode,
                ScaleMode,
                GradientScaleMode,
                CloneMode,
                TessellateMode // each triangle can have its own color
            };

            explicit PolyLine(const Vec2* vector2d = nullptr, size_t npoints = 0, float width = 0, Color color = Color(0.0),
                              bool ownPoints = false)
                : npoints(npoints)
                , width(width)
                , smoothingWidth(1.0)
                , ownPoints(ownPoints)
                , debug(0)
                , join(Path::RoundJoin)
                , cap(Path::SquareCap)
                , mode(OverMode)
                , splat(false)
                , built(false)
                , idhash(0)
            {
                color = color;

                if (ownPoints)
                {
                    points.assign(vector2d->begin(), vector2d->end());
                }
                else
                {
                    points = {};
                }
            }

            PolyLine(const PolyLine& polyLine)
                : Command(polyLine)
                , width(polyLine.width)
                , brush(polyLine.brush)
                , debug(polyLine.debug)
                , join(polyLine.join)
                , cap(polyLine.cap)
                , mode(polyLine.mode)
                , splat(polyLine.splat)
                , smoothingWidth(polyLine.smoothingWidth)
                , built(polyLine.built)
            {
                if (polyLine.npoints > 0)
                {
                    npoints = polyLine.npoints;
                    points.assign(polyLine.points.begin(), polyLine.points.end());
                }
                else
                {
                    npoints = 0;
                    points = {};
                }

                if (polyLine.width != 0.0f)
                {
                    widths.assign(polyLine.widths.begin(), polyLine.widths.end());
                }
                else
                {
                    widths = {};
                }

                ownPoints = true;
            }

            virtual ~PolyLine() = default;

            std::vector<Vec2> points{};
            size_t npoints;
            float width;
            float smoothingWidth;
            bool splat;
            std::vector<float> widths{};
            std::string brush;
            TwkPaint::Path::JoinStyle join;
            TwkPaint::Path::CapStyle cap;
            Mode mode;
            int debug;
            bool ownPoints;

            mutable HashValue idhash;

            // the following are computed / setup right after the geometry is
            // computed they only need to be recomputed if the geometry has
            // changed. The renderer draws them (vertex layout: see
            // build()).
            mutable std::vector<float> primitiveData;
            mutable std::vector<unsigned int> primitiveIndices;
            mutable size_t primitiveVertexCount{0};
            mutable size_t primitiveTriangleCount{0};

            //
            mutable Path path;
            mutable bool built;

            void hash(std::ostream& ostream) const override;
            virtual HashValue hashValue() const;

            void build() const;

            size_t getType() const override;
        };

        class Text : public Command
        {
        public:
            explicit Text(const std::string& /*str*/ = "", const std::string& /*fnt*/ = "", float ptsze = 1.0,
                          Color color = Color(1, 1, 1, 1))
                : ptsize(ptsze)
                , scale(1.0)
                , rotation(0.0)
                , spacing(1.0)
                , pos(0, 0)
            {
                color = color;
            }

            Text(const Text& text) = default;

            Vec2 pos;
            std::string text;
            std::string font;
            std::string origin;
            float ptsize;
            float scale;
            float rotation;
            float spacing;

            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class PushFrameBuffer : public Command
        {
        public:
            PushFrameBuffer();
            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class PopFrameBuffer : public Command
        {
        public:
            PopFrameBuffer();
            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class Rectangle : public Command
        {
        public:
            explicit Rectangle(float height = 0, float width = 0, Color color = Color(0, 0, 0, 1.0))
                : height(height)
                , width(width)
                , pos(0, 0)
            {
                color = color;
            }

            Rectangle(const Rectangle& rectangle) = default;

            Vec2 pos;
            float height;
            float width;

            void hash(std::ostream&) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class Quad : public Command
        {
        public:
            enum DrawMode
            {
                SolidMode,
                OutlineMode
            };

            Quad() = default;

            Quad(const Quad& quad)
                : drawMode(quad.drawMode)
            {
                points[0] = quad.points[0];
                points[1] = quad.points[1];
                points[2] = quad.points[2];
                points[3] = quad.points[3];
            }

            Vec2 points[4];
            DrawMode drawMode;

            void hash(std::ostream&) const override;
            [[nodiscard]] size_t getType() const override;
        };

        class ExecuteAllBefore : public Command
        {
        public:
            ExecuteAllBefore() = default;

            void hash(std::ostream& ostream) const override;
            [[nodiscard]] size_t getType() const override;
        };

    } // namespace Paint

} // namespace IPCore

#endif // __IPCore__PaintCommand__h__
