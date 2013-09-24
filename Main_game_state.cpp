#include "Main_game_state.h"

#include "My_viewer.h"

void Main_game_state::draw()
{
    setup_gl_points(true);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    _renderer->render(_core.get_level_data(), _core.get_current_time(), _viewer.camera());
}
