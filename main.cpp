#include <QApplication>
#include <QObject>

#include "My_viewer.h"


int main(int argc, char** argv)
{
    QApplication application(argc,argv);

    My_viewer::test();

    My_viewer * viewer = new My_viewer;
    viewer->show();

    return application.exec();
}
