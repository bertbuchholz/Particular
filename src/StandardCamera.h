#ifndef STANDARDCAMERA_H
#define STANDARDCAMERA_H

#include <QGLViewer/camera.h>

class StandardCamera : public qglviewer::Camera
{
public:
    StandardCamera() : qglviewer::Camera(), _zNearVal(0.0001f), _zFarVal(100.0f) {}

    StandardCamera(qglviewer::Camera const& cam) : qglviewer::Camera(cam),  _zNearVal(0.01f), _zFarVal(100.0f) {}
    StandardCamera(qglviewer::Camera const* cam) : qglviewer::Camera(*cam), _zNearVal(0.01f), _zFarVal(100.0f) {}

    StandardCamera(float zNearVal, float zFarVal) : _zNearVal(zNearVal), _zFarVal(zFarVal) {}

    void set_near_far(float const znear, float const zfar)
    {
        _zNearVal = znear;
        _zFarVal = zfar;
    }

    virtual qreal zNear() const
    {
        return _zNearVal;
    }

    virtual qreal zFar() const
    {
        return _zFarVal;
    }

private:
    float _zNearVal, _zFarVal;
};

#endif // STANDARDCAMERA_H
