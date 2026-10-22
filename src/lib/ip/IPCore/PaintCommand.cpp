//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/PaintCommand.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <boost/functional/hash.hpp>
#include <cstdlib>
#include <ctime>
#include <sstream>

namespace IPCore
{
    namespace Paint
    {
        using namespace std;
        using namespace TwkMath;
        using namespace TwkPaint;
        using namespace TwkFB;

        size_t PolyLine::getType() const { return Command::PolyLine; }

        void PolyLine::hash(ostream& ostream) const
        {
            for (const auto& point : points)
            {
                ostream << point;
            }

            ostream << npoints << width << smoothingWidth << splat;

            for (const auto& width : widths)
            {
                ostream << width;
            }

            ostream << color << brush << join << cap << mode << debug;
        }

        PolyLine::HashValue PolyLine::hashValue() const
        {
            ostringstream ostream;

            for (const auto& point : points)
            {
                ostream << point;
            }

            ostream << npoints << width << smoothingWidth << splat;

            for (const auto& width : widths)
            {
                ostream << width;
            }

            ostream << color << brush << join << cap << mode << debug;

            std::string id = ostream.str();

            boost::hash<string> string_hash;
            size_t v = string_hash(id);

            if (sizeof(size_t) == 8)
            {
                return (v & 0xffffffff) ^ ((v >> 32) & 0xffffffff);
            }
            else
            {
                return v;
            }
        }

        void PolyLine::build() const
        {
            path.clear();

            if (!widths.empty())
            {
                for (size_t i = 0; i < npoints; i++)
                    path.add(points[i], widths[i]);
            }
            else
            {
                for (size_t i = 0; i < npoints; i++)
                    path.add(points[i]);
                path.setContantWidth(width);
            }

            path.computeGeometry(join, cap, Path::RadiallySymmetric, Path::QualityAlgorithm, true, smoothingWidth, splat);

            // setup data to be prepared for late rendering
            const PointArray& opoints = path.outputPoints();
            const PointArray& tpoints = path.outputTexCoords();
            const IndexTriangleArray& triangles = path.outputTriangles();
            const ScalarArray& direction = path.outputDirectionalities();

            size_t vertexno = opoints.size();
            size_t trino = triangles.size();

            size_t vbodatasize = 0;
            if (mode == TessellateMode)
            {
                vertexno = 3 * trino;
                vbodatasize += 4 * vertexno;
                vbodatasize += 4 * vertexno; // all vertices of a triangle have
                                             // the same color
            }
            else
            {
                vbodatasize += 4 * vertexno;

                if (mode == GradientScaleMode)
                {
                    vbodatasize += vertexno;
                }
                else if (mode == TessellateMode)
                {
                    vbodatasize += 4 * vertexno;
                }
            }

            primitiveData.clear();
            primitiveData.resize(vbodatasize);
            primitiveIndices.clear();

            if (mode != TessellateMode)
            {
                for (size_t i = 0; i < vertexno; i++)
                {
                    primitiveData[2 * i + 0] = (float)tpoints[i].x;
                    primitiveData[2 * i + 1] = (float)tpoints[i].y;
                }
                for (size_t i = 0; i < vertexno; i++)
                {
                    primitiveData[2 * vertexno + 2 * i + 0] = (float)opoints[i].x;
                    primitiveData[2 * vertexno + 2 * i + 1] = (float)opoints[i].y;
                }
                if (mode == GradientScaleMode)
                {
                    for (size_t i = 0; i < vertexno; i++)
                    {
                        primitiveData[4 * vertexno + i] = (float)direction[i];
                    }
                }

                // indices
                primitiveIndices.resize(3 * triangles.size());
                for (size_t i = 0; i < trino; i++)
                {
                    primitiveIndices[3 * i + 0] = triangles[i][0];
                    primitiveIndices[3 * i + 1] = triangles[i][1];
                    primitiveIndices[3 * i + 2] = triangles[i][2];
                }
            }
            else
            {
                // fill in vertices
                for (size_t i = 0; i < trino; i++)
                {
                    primitiveData[6 * i + 0] = (float)tpoints[triangles[i][0]].x;
                    primitiveData[6 * i + 1] = (float)tpoints[triangles[i][0]].y;
                    primitiveData[6 * i + 2] = (float)tpoints[triangles[i][1]].x;
                    primitiveData[6 * i + 3] = (float)tpoints[triangles[i][1]].y;
                    primitiveData[6 * i + 4] = (float)tpoints[triangles[i][2]].x;
                    primitiveData[6 * i + 5] = (float)tpoints[triangles[i][2]].y;
                }
                size_t offset = 6 * trino;
                for (size_t i = 0; i < trino; i++)
                {
                    primitiveData[offset + 6 * i + 0] = (float)opoints[triangles[i][0]].x;
                    primitiveData[offset + 6 * i + 1] = (float)opoints[triangles[i][0]].y;
                    primitiveData[offset + 6 * i + 2] = (float)opoints[triangles[i][1]].x;
                    primitiveData[offset + 6 * i + 3] = (float)opoints[triangles[i][1]].y;
                    primitiveData[offset + 6 * i + 4] = (float)opoints[triangles[i][2]].x;
                    primitiveData[offset + 6 * i + 5] = (float)opoints[triangles[i][2]].y;
                }

                // fill in color per triangle
                offset = 12 * trino;
                srand(std::time(0));
                for (size_t i = 0; i < trino; i++)
                {
                    float r = (float)rand() / RAND_MAX;
                    float g = (float)rand() / RAND_MAX;
                    float b = (float)rand() / RAND_MAX;
                    primitiveData[offset + 12 * i] = r;
                    primitiveData[offset + 12 * i + 1] = g;
                    primitiveData[offset + 12 * i + 2] = b;
                    primitiveData[offset + 12 * i + 3] = 1.0;

                    primitiveData[offset + 12 * i + 4] = r;
                    primitiveData[offset + 12 * i + 5] = g;
                    primitiveData[offset + 12 * i + 6] = b;
                    primitiveData[offset + 12 * i + 7] = 1.0;

                    primitiveData[offset + 12 * i + 8] = r;
                    primitiveData[offset + 12 * i + 9] = g;
                    primitiveData[offset + 12 * i + 10] = b;
                    primitiveData[offset + 12 * i + 11] = 1.0;
                }
            }

            primitiveVertexCount = vertexno;
            primitiveTriangleCount = trino;

            built = true;
        }

        PushFrameBuffer::PushFrameBuffer() {}

        void PushFrameBuffer::hash(ostream& o) const { o << "pu"; }

        size_t PushFrameBuffer::getType() const { return Command::PushFrameBuffer; }

        PopFrameBuffer::PopFrameBuffer() {}

        void PopFrameBuffer::hash(ostream& o) const { o << "po"; }

        size_t PopFrameBuffer::getType() const { return Command::PopFrameBuffer; }

        void Text::hash(ostream& o) const { o << pos << color << text << font << origin << ptsize << scale << rotation << spacing; }

        size_t Text::getType() const { return Command::Text; }

        void Rectangle::hash(ostream& o) const { o << pos << height << width << color; }

        size_t Rectangle::getType() const { return Command::Rectangle; }

        void Quad::hash(ostream& o) const { o << points[0] << points[1] << points[2] << points[3] << drawMode << color; }

        size_t Quad::getType() const { return Command::Quad; }

        void ExecuteAllBefore::hash(ostream& o) const {}

        size_t ExecuteAllBefore::getType() const { return Command::ExecuteAllBefore; }

    } // namespace Paint
} // namespace IPCore
