message(Using libs.pri)

eigen3 {
    message(Using eigen3)
    INCLUDEPATH += $${EXT_DIR}/eigen3
    INCLUDEPATH += $${EXT_DIR}/eigen3/unsupported
}

BertSharedLib {
    message(Using BertSharedLib)

    LIBS += -lbert_shared

    !win32 {
        INCLUDEPATH += /opt/boost-clang/include
        LIBS += -L /opt/boost-clang/lib -lboost_serialization
    }

    win32 {
        ReleaseBuild {
            message(serial release)
            LIBS += /L$${BERT_SHARED_DIR}/release
            LIBS += $${EXT_DIR}/boost_1_55_0/stage/lib/libboost_serialization-vc120-mt-1_55.lib
        }
        DebugBuild {
            message(serial debug)
            LIBS += /L$${BERT_SHARED_DIR}/debug
            LIBS += $${EXT_DIR}/boost_1_55_0/stage/lib/libboost_serialization-vc120-mt-gd-1_55.lib
        }
    }

    QMAKE_LIBDIR += $${BERT_SHARED_DIR}

    !win32 {
        my_shared_lib.commands = +cd $${BERT_SHARED_DIR} && qmake && make
    }
    win32 {
        my_shared_lib.commands = cd $${BERT_SHARED_DIR} && qmake && nmake # the plus at line start needs to be there to avoid some stupid error msg about the jobserver
    }
    QMAKE_EXTRA_TARGETS += my_shared_lib
    PRE_TARGETDEPS += my_shared_lib
}

OpenMesh {
    message(Using OpenMesh: $${EXT_DIR})
    !win32 {
        INCLUDEPATH += $${EXT_DIR}/OpenMesh-6.3/src
        LIBS += -L$${EXT_DIR}/OpenMesh-6.3/build/Build/lib -lOpenMeshCore
    }
    win32 {
        DEFINES += _USE_MATH_DEFINES
        ReleaseBuild {
            message(OpenMesh release)
            LIBS += -L$${EXT_DIR}/OpenMesh-3.0/lib -lOpenMeshCore
        }
        DebugBuild {
            message(OpenMesh debug)
            LIBS += -L$${EXT_DIR}/OpenMesh-3.0/lib -lOpenMeshCored
        }
        INCLUDEPATH += $${EXT_DIR}/OpenMesh-3.0/src
    }
}

QGLViewer {
    message(Using QGLViewer)
    win32 {
        INCLUDEPATH += $${EXT_DIR}/libQGLViewer-2.5.2
        DebugBuild {
            LIBS += -L$${EXT_DIR}/libQGLViewer-2.5.2/QGLViewer/debug -lQGLViewerd2
        }
        ReleaseBuild {
            LIBS += -L$${EXT_DIR}/libQGLViewer-2.5.2/QGLViewer/release -lQGLViewer2
        }
    }
    linux {
        INCLUDEPATH += $${EXT_DIR}/libQGLViewer-2.6.3
        LIBS += -L$${EXT_DIR}/libQGLViewer-2.6.3/QGLViewer/ -lQGLViewer
    }
    macx {
        INCLUDEPATH += $${EXT_DIR}/libQGLViewer-2.4.0
        LIBS += -L$${EXT_DIR}/libQGLViewer-2.4.0/QGLViewer/ -lQGLViewer
    }
}


boost {
    win32 {
        message(Using boost)
        LIBS += /L$${EXT_DIR}/../boost_1_55_0/stage/lib
        INCLUDEPATH += $${EXT_DIR}/boost_1_55_0
    }
}


message(Includepath: $${INCLUDEPATH})
