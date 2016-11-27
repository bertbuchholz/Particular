#include "Frame_buffer.h"

#include "Color_utilities.h"

QImage convert_with_alpha(const Frame_buffer<Color4> &frame_buffer)
{
    QImage image(frame_buffer.get_width(), frame_buffer.get_height(), QImage::Format_ARGB32);
    QImage alpha = image.alphaChannel();

    int const height = frame_buffer.get_height();

    for (int serial_index = 0; serial_index < frame_buffer.get_size(); ++serial_index)
    {
        QColor color = convert<QColor>(frame_buffer.get_data(serial_index));

        int x, y;
        frame_buffer.get_coordinates(serial_index, x, y);

        image.setPixel(x, height - y - 1, color.rgb());
        alpha.setPixel(x, height - y - 1, color.alpha());
    }

    image.setAlphaChannel(alpha);

    return image;
}
