cache()

TEMPLATE = app
TARGET = chemistrygame
DEPENDPATH += .

QT += opengl \
    xml
CONFIG += qt \
    warn_on \
    thread \
    QGLViewer \
    OpenMesh \
    eigen3 \
    BertSharedLib \
    boost \
    ANN \
    c++11

# to relink external libs used by the exetubale to copies inside of the bundle Contents/Frameworks dir
#    BUNDLE_DIR = chemo.app/Contents/
#    install_name_tool -change libboost_serialization.dylib @executable_path/../Frameworks/libboost_serialization.dylib $${BUNDLE_DIR}/MacOS/chemo

#CONFIG -= app_bundle

# see: http://eigen.tuxfamily.org/index.php?title=FAQ#Vectorization
#DEFINES += EIGEN_DONT_VECTORIZE
DEFINES += EIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT

win32 {
    DEFINES += NOMINMAX BOOST_ALL_NO_LIB
}

EXT_DIR = ../extern
BERT_SHARED_DIR = ../shared

DEPENDPATH += $${BERT_SHARED_DIR}

include($${BERT_SHARED_DIR}/libs.pri)

INCLUDEPATH += . \
    $${BERT_SHARED_DIR} \
    $${EXT_DIR}

CONFIG(debug, debug|release) {
    MOC_DIR = debug/moc
    OBJECTS_DIR = debug/obj
}
else {
    MOC_DIR = release/moc
    OBJECTS_DIR = release/obj
}

!win32 {
QMAKE_CXXFLAGS += -Wall \
    -Wextra \
    -fPIC #\
#    -std=c++11 \
#    -ftemplate-depth=1024 # \
#    -ferror-limit=1
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -O3
    QMAKE_LFLAGS_RELEASE -= -O1
}

win32 {
    QMAKE_CXXFLAGS += /bigobj /FS
    QMAKE_CFLAGS_RELEASE    = /Zi
    QMAKE_LFLAGS_RELEASE    = /MAP /DEBUG /OPT:REF
}

macx {
    INCLUDEPATH += /opt/local/include
}

linux {
    LIBS += -lGLU
}

# Input
SOURCES += main.cpp \
    RegularBspTree.cpp \
    Atom.cpp \
    Data_config.cpp \
    Renderer.cpp \
    Level_element.cpp \
    Core.cpp \
    Draggable.cpp \
    Molecule_releaser.cpp \
    ANN_wrapper_functions.cpp \
    Main_game_screen.cpp \
    Main_menu_screen.cpp \
    Screen.cpp \
    Before_start_screen.cpp \
    Main_options_screen.cpp \
    After_finish_screen.cpp \
    Particle_system.cpp \
    Statistics_screen.cpp \
    Main_options_window.cpp \
    Level_data.cpp \
    Editor_screen.cpp \
    My_viewer.cpp \
    Editor_pause_screen.cpp \
    After_finish_editor_screen.cpp \
    Menu_screen.cpp \
    Pause_screen.cpp \
    GL_texture.cpp \
    Experiment_screen.cpp \
    PolygonalCurve.cpp \
    Help_screen.cpp \
    Event.cpp \
    Score.cpp \
    Draggable_event.cpp \
    Level_element_draw_visitor.cpp \
    GPU_force.cpp

OTHER_FILES += TODO.txt \
    data/shaders/picking.vert \
    data/shaders/picking.frag \
    data/shaders/force_calc.frag \
    data/shaders/force_calc.vert \
    README.txt \
    data/shaders/simple.vert \
    data/shaders/molecule.frag \
    data/shaders/fullscreen_square.vert \
    data/shaders/simple_texture.frag \
    data/shaders/temperature.vert \
    temperature.frag \
    data/shaders/temperature.frag \
    data/shaders/post.frag \
    data/shaders/test.frag \
    data/shaders/depth_blur_1D.frag \
    data/shaders/blur_1D.frag \
    heat.frag \
    data/shaders/heat.frag \
    data/shaders/drop_shadow.frag \
    data/shaders/particle.vert \
    data/shaders/particle.frag \
    data/shaders/distance_particle.vert \
    data/shaders/mesh.vert \
    data/shaders/molecule_new.frag

HEADERS += \
    My_viewer.h \
    Atom.h \
    Core.h \
    Spatial_hash.h \
    Renderer.h \
    GPU_force.h \
    Atomic_force.h \
    RegularBspTree.h \
    Draggable.h \
    Visitor.h \
    Level_element_draw_visitor.h \
    Eigen_Matrix_serializer.h \
    End_condition.h \
    Level_data.h \
    Particle_system.h \
    unique_ptr_serialization.h \
    Sensor_data.h \
    Progress.h \
    Score.h \
    Data_config.h \
    Level_element.h \
    Molecule_releaser.h \
    ANN_wrapper_functions.h \
    Main_game_screen.h \
    Pause_screen.h \
    Main_menu_screen.h \
    Screen.h \
    Before_start_screen.h \
    Main_options_window.h \
    After_finish_screen.h \
    Statistics_screen.h \
    Editor_screen.h \
    Editor_pause_screen.h \
    After_finish_editor_screen.h \
    Menu_screen.h \
    GL_texture.h \
    Experiment_screen.h \
    PolygonalCurve.h \
    Help_screen.h \
    Event.h \
    Draggable_event.h
