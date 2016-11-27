cache()

TEMPLATE = app
TARGET = particular
DEPENDPATH += src

QT += \
    opengl \
    xml \
    widgets

CONFIG += \
    qt \
    warn_on \
    QGLViewer \
    OpenMesh \
    eigen3 \
    boost \
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

include(src/libs.pri)

INCLUDEPATH += \
    $${EXT_DIR}

CONFIG(debug, debug|release) {
    MOC_DIR = build/debug/moc
    OBJECTS_DIR = build/debug/obj
    CONFIG += console
}
else {
    MOC_DIR = build/release/moc
    OBJECTS_DIR = build/release/obj
}

!win32 {
    QMAKE_CXXFLAGS += \
        -Wall \
        -Wextra \
        -fPIC \
        -ftemplate-depth=512
    LIBS += -lboost_serialization
}

win32 {
    QMAKE_CXXFLAGS += /bigobj /FS
}

macx {
    INCLUDEPATH += /opt/local/include
}

linux {
    LIBS += -lGLU
}

# Input
SOURCES += \
    src/main.cpp \
#    src/RegularBspTree.cpp \
    src/Atom.cpp \
    src/Data_config.cpp \
    src/Renderer.cpp \
    src/Level_element.cpp \
    src/Core.cpp \
    src/Draggable.cpp \
    src/Molecule_releaser.cpp \
    src/Main_game_screen.cpp \
    src/Main_menu_screen.cpp \
    src/Screen.cpp \
    src/Before_start_screen.cpp \
    src/Main_options_screen.cpp \
    src/After_finish_screen.cpp \
    src/Particle_system.cpp \
    src/Statistics_screen.cpp \
    src/Main_options_window.cpp \
    src/Level_data.cpp \
    src/Editor_screen.cpp \
    src/My_viewer.cpp \
    src/Editor_pause_screen.cpp \
    src/After_finish_editor_screen.cpp \
    src/Menu_screen.cpp \
    src/Pause_screen.cpp \
    src/GL_texture.cpp \
    src/Experiment_screen.cpp \
    src/PolygonalCurve.cpp \
    src/Help_screen.cpp \
    src/Event.cpp \
    src/Score.cpp \
    src/Draggable_event.cpp \
    src/Level_element_draw_visitor.cpp \
    src/GPU_force.cpp \
    src/level_picker_screen.cpp \
    src/Picking.cpp \
    src/Icosphere.cpp \
    src/Draw_functions.cpp \
    src/Geometry_utils.cpp \
    src/Decal.cpp \
    src/GL_utilities.cpp \
    src/Color.cpp \
    src/Color_utilities.cpp \
    src/Frame_buffer.cpp \
    src/Q_parameter_bridge.cpp \
    src/Parameter.cpp \
    src/Options_viewer.cpp \
    src/Message_logger.cpp \
    src/qxtgroupbox.cpp

OTHER_FILES += \
    data/shaders/picking.vert \
    data/shaders/picking.frag \
    data/shaders/force_calc.frag \
    data/shaders/force_calc.vert \
    data/shaders/simple.vert \
    data/shaders/molecule.frag \
    data/shaders/fullscreen_square.vert \
    data/shaders/simple_texture.frag \
    data/shaders/temperature.vert \
    data/shaders/temperature.frag \
    data/shaders/post.frag \
    data/shaders/test.frag \
    data/shaders/depth_blur_1D.frag \
    data/shaders/blur_1D.frag \
    data/shaders/heat.frag \
    data/shaders/drop_shadow.frag \
    data/shaders/particle.vert \
    data/shaders/particle.frag \
    data/shaders/distance_particle.vert \
    data/shaders/mesh.vert \
    data/shaders/molecule_new.frag

HEADERS += \
    src/My_viewer.h \
    src/Atom.h \
    src/Core.h \
    src/Spatial_hash.h \
    src/Renderer.h \
    src/GPU_force.h \
    src/Atomic_force.h \
#    src/RegularBspTree.h \
    src/Draggable.h \
    src/Visitor.h \
    src/Level_element_draw_visitor.h \
    src/Eigen_Matrix_serializer.h \
    src/End_condition.h \
    src/Level_data.h \
    src/Particle_system.h \
    src/unique_ptr_serialization.h \
    src/Sensor_data.h \
    src/Progress.h \
    src/Score.h \
    src/Data_config.h \
    src/Level_element.h \
    src/Molecule_releaser.h \
    src/Main_game_screen.h \
    src/Pause_screen.h \
    src/Main_menu_screen.h \
    src/Screen.h \
    src/Before_start_screen.h \
    src/Main_options_window.h \
    src/After_finish_screen.h \
    src/Statistics_screen.h \
    src/Editor_screen.h \
    src/Editor_pause_screen.h \
    src/After_finish_editor_screen.h \
    src/Menu_screen.h \
    src/GL_texture.h \
    src/Experiment_screen.h \
    src/PolygonalCurve.h \
    src/Help_screen.h \
    src/Event.h \
    src/Draggable_event.h \
    src/Random_generator.h \
    src/Fps.h \
    src/level_picker_screen.h \
    src/widget_text_combination.h \
    src/FloatSlider.h \
    src/Picking.h \
    src/Low_discrepancy_sequences.h \
    src/Registry.h \
    src/Registry_parameters.h \
    src/Icosphere.h \
    src/Draw_functions.h \
    src/Geometry_utils.h \
    src/Decal.h \
    src/MyOpenMesh.h \
    src/GL_utilities.h \
    src/Color.h \
    src/Color_palette.h \
    src/Color_utilities.h \
    src/Frame_buffer.h \
    src/Utilities.h \
    src/Squared_float_slider.h \
    src/Q_parameter_bridge.h \
    src/Parameter.h \
    src/StandardCamera.h \
    src/Options_viewer.h \
    src/Message_logger.h \
    src/qxtgroupbox.h \
    src/FoldableGroupBox.h \
    src/Combo_switcher.h
