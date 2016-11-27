#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <QImage>
#include <QColor>
#include <cassert>

#include "Color_utilities.h"

template <class Data>
class Frame_buffer
{
public:
    typedef Data data_t;

    Frame_buffer()
    {
        set_size(0, 0);
    }

    Frame_buffer(int const width, int const height)
    {
        set_size(width, height);
    }

    Frame_buffer(int const width, int const height, Data const& initial_data) : Frame_buffer(width, height)
    {
        set_size(width, height);
        std::fill(_data.begin(), _data.end(), initial_data);
    }

    virtual ~Frame_buffer() {}

    virtual void set_data(int const serial_index, Data const& data)
    {
        assert(serial_index < _size);

        _data[serial_index] = data;
    }

    virtual void set_data(int const x, int const y, Data const& data)
    {
        int const pixel = get_coordinate(x, y);

        set_data(pixel, data);
    }

    virtual Data const& get_data(int const x, int const y) const
    {
        int const pixel = get_coordinate(x, y);

        return get_data(pixel);
    }

    virtual Data const& get_data(int const serial) const
    {
        assert(serial < _size);

        return _data[serial];
    }

    virtual Data & get_data(int const x, int const y)
    {
        int const pixel = get_coordinate(x, y);

        return get_data(pixel);
    }

    virtual Data & get_data(int const serial)
    {
        assert(serial < _size);

        return _data[serial];
    }

    std::vector<Data> const& get_data() const
    {
        return _data;
    }

    Data get_interpolated_value(float const x, float const y) const // coordinate between 0..1
    {
        assert(x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f);

        int const x_int = int(x * (get_width() - 1));
        float const x_offset = (x * (get_width() - 1)) - x_int;

        int const y_int = int(y * (get_height() - 1));
        float const y_offset = (y * (get_height() - 1)) - y_int;

        Data const d0 = get_data(x_int, y_int) * (1.0f - x_offset) + get_data(std::min(x_int + 1, get_width() - 1), y_int) * x_offset;

        Data const d1 = get_data(x_int, std::min(y_int + 1, get_height() - 1)) * (1.0f - x_offset) +
                                 get_data(std::min(x_int + 1, get_width() - 1), std::min(y_int + 1, get_height() - 1)) * x_offset;

        Data const d12 = d0 * (1.0f - y_offset) + d1 * y_offset;

        return d12;
    }

    Data const* get_raw_data() const
    {
        return _data.data();
    }

    Data * get_raw_data()
    {
        return _data.data();
    }

    void set_raw_data(int const width, int const height, std::vector<Data> data)
    {
        _width = width;
        _height = height;
        _size = _width * _height;

        _data = data;

        assert(_size == _data.size());
    }

    virtual void set_size(int const width, int const height)
    {
        _width = width;
        _height = height;
        _size = _width * _height;

        clear();
    }

    virtual void clear()
    {
        _data.clear();
        _data.resize(_size);
    }

    int get_size() const
    {
        return _size;
    }

    int get_width() const
    {
        return _width;
    }

    int get_height() const
    {
        return _height;
    }

    void get_coordinates(int const serial, int & x, int & y) const
    {
        x = serial % _width;
        y = serial / _width;
    }

    inline int get_coordinate(int const x, int const y) const
    {
        return x + _width * y;
    }

    bool is_valid(int const x, int const y) const
    {
        return (x >= 0 && x < _width && y >= 0 && y < _height);
    }


protected:
    int _width;
    int _height;
    int _size;

    std::vector<Data> _data;
};


QImage convert_with_alpha(Frame_buffer<Color4> const& frame_buffer);


template <typename Converter, typename Data>
QImage convert(Frame_buffer<Data> const& frame_buffer)
{
    QImage image(frame_buffer.get_width(), frame_buffer.get_height(), QImage::Format_RGB32);

    Converter converter;

    int const height = frame_buffer.get_height();

    for (int serial_index = 0; serial_index < frame_buffer.get_size(); ++serial_index)
    {
        QColor color = converter(frame_buffer.get_data(serial_index));

        int x, y;
        frame_buffer.get_coordinates(serial_index, x, y);

        image.setPixel(x, height - y - 1, color.rgb());
    }

    return image;
}

template <typename Converter, typename Data>
Frame_buffer<Data> convert(QImage const& qimage)
{
    if (qimage.isNull())
    {
        std::cout << __FUNCTION__ << " Could not convert image, image is empty." << std::endl;
        return Frame_buffer<Data>();
    }

    Frame_buffer<Data> frame_buffer(qimage.width(), qimage.height());

    Converter converter;

    int const height = qimage.height();

    for (int serial_index = 0; serial_index < frame_buffer.get_size(); ++serial_index)
    {
        int x, y;
        frame_buffer.get_coordinates(serial_index, x, y);

        Data data = converter(qimage.pixel(x, height - y - 1));

        frame_buffer.set_data(serial_index, data);
    }

    return frame_buffer;
}

#endif // FRAME_BUFFER_H
