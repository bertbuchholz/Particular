#include <QObject>
#include <QApplication>

#ifndef Q_MOC_RUN
#include <boost/serialization/nvp.hpp>
#endif

#include <Message_logger.h>

#include "My_viewer.h"
#include "Spatial_hash.h"
#include "End_condition.h"
#include "Data_config.h"

BOOST_CLASS_EXPORT_GUID(Box_barrier, "Box_barrier")
BOOST_CLASS_EXPORT_GUID(Plane_barrier, "Plane_barrier")
//BOOST_CLASS_EXPORT_GUID(Moving_box_barrier, "Moving_box_barrier")
BOOST_CLASS_EXPORT_GUID(Molecule_releaser, "Molecule_releaser")
BOOST_CLASS_EXPORT_GUID(Box_portal, "Box_portal")
BOOST_CLASS_EXPORT_GUID(Sphere_portal, "Sphere_portal")
BOOST_CLASS_EXPORT_GUID(Brownian_box, "Brownian_box")
BOOST_CLASS_EXPORT_GUID(Tractor_barrier, "Tractor_barrier")
BOOST_CLASS_EXPORT_GUID(Charged_barrier, "Charged_barrier")
BOOST_CLASS_EXPORT_GUID(Molecule_capture_condition, "Molecule_capture_condition")

//extern "C"
//{
//    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
//}

int main(int argc, char** argv)
{
    QApplication application(argc,argv);

    if (!(QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_3_3))
    {
        QMessageBox e;
        e.setText("OpenGL version 3.3 or higher necessary but couldn't be found.");
        e.exec();

        abort();
    }

    QStringList arguments = application.arguments();
    bool const use_unstable_options = arguments.contains("--use-unstable-options");

    Message_logger::get_instance()->init("log.txt");

    Core core(use_unstable_options);

    QGLFormat format;
    format.setSwapInterval(1);

    My_viewer * viewer = new My_viewer(core, format);
    viewer->show();
    viewer->start();

    return application.exec();
}
