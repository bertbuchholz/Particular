#include <QApplication>
#include <QObject>

#include "My_viewer.h"
#include "Spatial_hash.h"

int main(int argc, char** argv)
{
    QApplication application(argc,argv);

    My_viewer::test();
    spatial_hash_test();

    My_viewer * viewer = new My_viewer;
    viewer->show();

    return application.exec();
}
