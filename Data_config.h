#ifndef DATA_CONFIG_H
#define DATA_CONFIG_H

#include <iostream>
#include <cassert>
#include <QCoreApplication>
#include <QDir>
#include <vector>

class Data_config
{
public:
    static Data_config * get_instance()
    {
        if (!_data_config)
        {
            _data_config = new Data_config;
            _data_config->init();
        }

        return _data_config;
    }

//    std::string const& get_data_path() const
//    {
//        return _data_path;
//    }

//    QString get_qdata_path() const
//    {
//        return QString::fromStdString(_data_path);
//    }

    QString get_absolute_qfilename(QString const& relative_path) const
    {
        return QString::fromStdString(get_absolute_filename(relative_path));
    }

    std::string get_absolute_filename(QString const& relative_path) const
    {
        for (QString const& path : _data_paths)
        {
            QString absolute_filename = path + "/" + relative_path;

            if (QFile::exists(absolute_filename))
            {
                return absolute_filename.toStdString();
            }
        }

        throw;
    }

//    std::string get_absolute_filename(std::string const& relative_path) const
//    {
//        return get_absolute_filename(QString::fromStdString(relative_path));
//    }

private:
    void init()
    {
        _data_path = "";

        QString app_path = QCoreApplication::applicationDirPath();

        QStringList search_paths;
        search_paths << "data" << "../data" << "../../../data";

        for (QString const& path : search_paths)
        {
            QDir dir(app_path + "/" + path);

            if (dir.exists())
            {
//                _data_path = dir.absolutePath().toStdString();
//                break;
                _data_paths.append(dir.absolutePath());
            }
        }

        if (_data_paths.empty())
        {
            std::cout << __PRETTY_FUNCTION__ << " no data paths found" << std::endl;
            assert(false);
        }
        else
        {
            for (QString const p : _data_paths)
            {
                std::cout << __PRETTY_FUNCTION__ << " data path found: " << p.toStdString() << std::endl;
            }
        }
    }

    static Data_config * _data_config;

    std::string _data_path;
    QStringList _data_paths;
};

#endif // DATA_CONFIG_H
