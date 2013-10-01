#include <QObject>
#include <QApplication>

#include <boost/serialization/nvp.hpp>

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

BOOST_CLASS_EXPORT_GUID(Molecule_capture_condition, "Molecule_capture_condition")




int main(int argc, char** argv)
{
    QApplication application(argc,argv);

//    QGLFormat f;
//    f.setVersion(3, 2);
//    f.setProfile(QGLFormat::CoreProfile);
//    QGLFormat::setDefaultFormat(f);

//    QGLFormat glFormat;
//    glFormat.setVersion(3, 2);
//    glFormat.setProfile(QGLFormat::CoreProfile); // Requires >=Qt-4.8.0
//    glFormat.setSampleBuffers(true);

    spatial_hash_test();

    My_viewer * viewer = new My_viewer();
    viewer->show();

    return application.exec();
}
