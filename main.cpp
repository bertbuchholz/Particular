#include <QApplication>
#include <QObject>

#include "My_viewer.h"
#include "Spatial_hash.h"
#include "End_condition.h"

BOOST_CLASS_EXPORT_GUID(Box_barrier, "Box_barrier")
BOOST_CLASS_EXPORT_GUID(Plane_barrier, "Plane_barrier")
BOOST_CLASS_EXPORT_GUID(Moving_box_barrier, "Moving_box_barrier")
BOOST_CLASS_EXPORT_GUID(Molecule_releaser, "Molecule_releaser")
BOOST_CLASS_EXPORT_GUID(Box_portal, "Box_portal")

BOOST_CLASS_EXPORT_GUID(Molecule_capture_condition, "Molecule_capture_condition")

int main(int argc, char** argv)
{
    QApplication application(argc,argv);

    QGLFormat f;
    f.setVersion(3, 2);
    f.setProfile(QGLFormat::CoreProfile);
    QGLFormat::setDefaultFormat(f);

    spatial_hash_test();

    My_viewer * viewer = new My_viewer;
    viewer->show();

    return application.exec();
}
