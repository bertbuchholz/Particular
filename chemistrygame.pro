# #####################################################################
# Automatically generated by qmake (2.01a) Mon Oct 12 12:58:25 2009
# #####################################################################
TEMPLATE = app
TARGET = 
DEPENDPATH += .


QT += opengl \
    xml
CONFIG += qt \
    warn_on \
    thread \
    console \
    QGLViewer \
    OpenMesh \
    eigen3 \
    BertSharedLib \
    ANN  #\
#    MacOSg++

# to relink external libs used by the exetubale to copies inside of the bundle Contents/Frameworks dir
#    BUNDLE_DIR = chemo.app/Contents/
#    install_name_tool -change libboost_serialization.dylib @executable_path/../Frameworks/libboost_serialization.dylib $${BUNDLE_DIR}/MacOS/chemo

#CONFIG -= app_bundle

EXT_DIR = ../extern
BERT_SHARED_DIR = ../shared

DEPENDPATH += $${BERT_SHARED_DIR}

include($${BERT_SHARED_DIR}/libs.pri)

INCLUDEPATH += . \
    $${BERT_SHARED_DIR} \
    $${EXT_DIR}

MOC_DIR = ./moc
OBJECTS_DIR = ./obj

QMAKE_CXXFLAGS += -Wall \
    -Wextra \
    -fPIC \
    -std=c++11 \
    -ftemplate-depth=1024 # \
#    -ferror-limit=1

macx {
    QMAKE_CXX = /opt/local/bin/clang++-mp-3.3
    QMAKE_LINK = /opt/local/bin/clang++-mp-3.3

    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS_X86_64 = $$QMAKE_CXXFLAGS
}

LIBS += -lGLU
#    -lpthread \
#    -lGLEW

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
    Editor_screen.cpp

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
    data/shaders/blur_1D.frag

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
    Editor_screen.h
