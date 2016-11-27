#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H

#include "Color.h"

#include <map>
#include <vector>

class Color_palette
{
public:
    Color_palette()
    {
        setup();
    }


    void setup()
    {
        // S_11
//        add_color(5, Color(60.0f / 255.0f, 186.0f / 255.0f, 200.0f / 255.0f));
//        add_color(5, Color(147.0f / 255.0f, 237.0f / 255.0f, 212.0f / 255.0f));
//        add_color(5, Color(243.0f / 255.0f, 245.0f / 255.0f, 196.0f / 255.0f));
//        add_color(5, Color(249.0f / 255.0f, 203.0f / 255.0f, 143.0f / 255.0f));
//        add_color(5, Color(241.0f / 255.0f, 145.0f / 255.0f, 129.0f / 255.0f));

        // Emerge USA
//        add_color(5, Color::from_int(5, 82, 125));
//        add_color(5, Color::from_int(0, 166, 214));
//        add_color(5, Color::from_int(151, 222, 255));
//        add_color(5, Color::from_int(252, 251, 231));
//        add_color(5, Color::from_int(211, 0, 39));

        // TEMPERATE
//        add_color(5, Color::from_int(105, 210, 231));
//        add_color(5, Color::from_int(167, 219, 216));
//        add_color(5, Color::from_int(224, 228, 204));
//        add_color(5, Color::from_int(243, 134, 48));
//        add_color(5, Color::from_int(250, 105, 0));

        // Temperatures
        add_color(5, Color::from_int(63, 105, 147));
        add_color(5, Color::from_int(160, 194, 218));
        add_color(5, Color::from_int(209, 209, 209));
        add_color(5, Color::from_int(249, 198, 78));
        add_color(5, Color::from_int(217, 96, 26));
    }

    void add_color(int const num_colors, Color const& c)
    {
        _color_positions.push_back(_color_positions.size() / float(num_colors - 1));
        _colors.push_back(c);
    }

    Color get_gradient_color(float const s) const
    {
        assert(s >= 0.0f && s <= 1.0f);

        auto iter = std::lower_bound(_color_positions.begin(), _color_positions.end(), s);

        if (iter == --_color_positions.end()) return _colors.back();

        int const index = iter - _color_positions.begin();

        float const s0 = _color_positions[index];
        Color const& c0 = _colors[index];

        float const s1 = _color_positions[index + 1];
        Color const& c1 = _colors[index + 1];

        float const alpha = (s1 - s) / (s1 - s0);

        return alpha * c0 + (1.0f - alpha) * c1;
    }


    Color const& get_color(int const i) const
    {
        return _colors[i];
    }

    Color4 get_gradient_color_4(float const s, float const alpha) const
    {
        return Color4(get_gradient_color(s), alpha);
    }

private:
    std::vector<float> _color_positions;
    std::vector<Color> _colors;
};

#endif // COLOR_PALETTE_H
