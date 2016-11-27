#ifndef PICKING_H
#define PICKING_H

#include <iostream>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <memory>
#include <functional>
#include <QDir>

#include "GL_utilities.h"

void set_pick_index(int const index);

static const QString picking_frag = "#version 120\n\
        uniform vec3 color;\
        void main(void)\
        {\
            gl_FragColor = vec4(color, 1.0);\
        }\
        ";

static const QString picking_vert = "\
        void main(void)\
        {\
            gl_Position = ftransform();\
        }\
        ";

class Picking
{
public:
    Picking() :
        redShift  (16),
        greenShift (8),
        blueShift  (0),
        redMask  (0xFF << redShift),
        greenMask(0xFF << greenShift),
        blueMask (0xFF << blueShift)
    { }

    void init(QGLContext const* context, QSize const& size)
    {
//        _shader_program = std::unique_ptr<QGLShaderProgram>(
//                    init_program(context, Data_config::get_instance()->get_qdata_path() + "/shaders/picking.vert", Data_config::get_instance()->get_qdata_path() + "/shaders/picking.frag"));

        _shader_program = std::unique_ptr<QGLShaderProgram>(new QGLShaderProgram(context));

        _shader_program->addShaderFromSourceCode(QGLShader::Vertex, picking_vert);
        _shader_program->addShaderFromSourceCode(QGLShader::Fragment, picking_frag);
        _shader_program->link();

        _framebuffer = new QGLFramebufferObject(size, QGLFramebufferObject::Depth);
    }

    void init(QGLContext const* context)
    {
        init(context, QSize(1024, 1024));
    }

    int do_pick(float const x, float const y, std::function<void()> drawFunction)
    {
        return do_pick(int(x * _framebuffer->width()), int(y * _framebuffer->height()), drawFunction);
    }

    int do_pick(int const x, int const y, std::function<void()> drawFunction)
    {
        _framebuffer->bind();
        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);

        glViewport(0, 0, _framebuffer->width(), _framebuffer->height());

        _is_Picking = true;
        glDisable(GL_BLEND);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _shader_program->bind();

        drawFunction();

        _shader_program->release();

        GLubyte data[4];

        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

        bool const is_geometry = (data[3] == 255);

//        std::cout << int(data[0]) << std::endl;
//        std::cout << int(data[1]) << std::endl;
//        std::cout << int(data[2]) << std::endl;
//        std::cout << int(data[3]) << std::endl;

        int foundIndex = -1;

        if (is_geometry)
        {
            foundIndex =
                    (data[0] << redShift) +
                    (data[1] << greenShift) +
                    (data[2] << blueShift);
        }

        glEnable(GL_BLEND);

//        _framebuffer->toImage().save("/tmp/img.png");

        glPopAttrib();
        _framebuffer->release();

        _is_Picking = false;
        return foundIndex;
    }


    void set_index(int const index) const
    {
        // possible breaking due to the use of floats, but when using integers, there is a flickering problem

//        glColor4ub((index & redMask)   >> redShift,
//                   (index & greenMask) >> greenShift,
//                   (index & blueMask)  >> blueShift,
//                   255);

        int r = (index & redMask) >> redShift;
        int g = (index & greenMask) >> greenShift;
        int b = (index & blueMask)  >> blueShift;

        _shader_program->setUniformValue("color", QVector3D(r / 255.0f, g / 255.0f, b / 255.0f));
    }

    static Picking* getInstance()
    {
        if(!instance)
            instance = new Picking;
        return instance;
    }

private:
    std::unique_ptr<QGLShaderProgram> _shader_program;
    QGLFramebufferObject * _framebuffer;

    static Picking* instance;
    int redShift;
    int greenShift;
    int blueShift;
    GLuint redMask;
    GLuint greenMask;
    GLuint blueMask;
    bool _is_Picking;
};



/*
template <class data_t>
class Picking
{
public:
    typedef data_t Data;

    Picking() :
        redShift(16),
        greenShift(8),
        blueShift(0),
        redMask(0xFF << redShift),
        greenMask(0xFF << greenShift),
        blueMask(0xFF)
    { }

    Data doPick(int x, int y)
    {
        Data result;

        glDisable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glShadeModel(GL_FLAT);

        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw();

        GLubyte* data = new GLubyte[4];

         glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

         int foundIndex = ((data[0] << 16) + (data[1] << 8) + data[2]);

         // do we have geometry under the mouse or not: if it's index is FFFFFF, it means we pick up
         // the clear color. unless there are FFFFFF quads in the mesh, this should be safe
         bool isGeometry = !(foundIndex == 0xFFFFFF);

         // std::cout << std::hex << index << std::endl;

         if (isGeometry)
         {
             typename std::map<int, Data>::const_iterator iter = _indexTripleMap.find(foundIndex);
             if (iter != _indexTripleMap.end())
             {
                 result = iter->second;
             }
         }

        glEnable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glShadeModel(GL_SMOOTH);

        delete [] data;

        // qglClearColor(backgroundColor());

        return result;
    }

    virtual void draw() = 0;

protected:
    int redShift;
    int greenShift;
    int blueShift;
    GLuint redMask;
    GLuint greenMask;
    GLuint blueMask;

    std::map<Data, int> _tripleIndexMap;
    std::map<int, Data> _indexTripleMap;
};


template <class data_t>
class CutPathPickingCm : public Picking<data_t>
{
public:
    CutPathPickingCm(std::vector< std::vector<IndexTriple> > const& cutPaths, ContourManifold const& cm) :
        Picking<data_t>(),
        _cutPaths(cutPaths),
        _cm(cm)
    {}

    virtual void draw()
    {
        int serialIndex = 0;

        glLineWidth(5.0f);
        glPointSize(10.0f);

        glPushMatrix();
        glScalef(1.0f / DRAW_DOWN_SCALE, 1.0f / DRAW_DOWN_SCALE, 1.0f);

        for (unsigned int cutPathIndex = 0; cutPathIndex < _cutPaths.size(); ++cutPathIndex)
        {
            std::vector<IndexTriple> const& cutPath = _cutPaths[cutPathIndex];

            for (unsigned int vertexIndex = 0; vertexIndex < cutPath.size(); ++vertexIndex)
            {
                ++serialIndex;

                IndexTriple const& indexTriple = cutPath[vertexIndex];

                glPushMatrix();
                glTranslatef(0.0f, 0.0f, indexTriple._fIndex * DRAW_FRAME_WIDTH);

                this->_tripleIndexMap[IndexTriple(-1, cutPathIndex, vertexIndex)] = serialIndex;
                this->_indexTripleMap[serialIndex] = IndexTriple(-1, cutPathIndex, vertexIndex);

                glColor4ub((serialIndex & this->redMask) >> this->redShift, (serialIndex & this->greenMask) >> this->greenShift, (serialIndex & this->blueMask) >> this->blueShift, 255);

                glBegin(GL_POINTS);
                Mesh::Point const& p = _cm[indexTriple._fIndex][indexTriple._cIndex].getVertices()[indexTriple._vIndex].getPos();
                glVertex3fv(p.data());
                glEnd();

                glPopMatrix();
            }
        }

        glPopMatrix();
    }

private:
    std::vector< std::vector<IndexTriple> > const& _cutPaths;
    ContourManifold const& _cm;
};
*/


#endif // PICKING_H
