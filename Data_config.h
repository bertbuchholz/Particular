#ifndef DATA_CONFIG_H
#define DATA_CONFIG_H

#include <iostream>
#include <cassert>
#include <QCoreApplication>
#include <QDir>

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

    void init()
    {
        _data_path = "";

        QString app_path = QCoreApplication::applicationDirPath();

        QStringList search_paths;
        search_paths << "/data" << "/../data" << "/../../../data";

        for (QString const& path : search_paths)
        {
            QDir dir(app_path + path);

            if (dir.exists())
            {
                _data_path = dir.absolutePath().toStdString();
                break;
            }
        }

        if (_data_path.empty())
        {
            std::cout << __PRETTY_FUNCTION__ << " no data path found" << std::endl;
            assert(false);
        }
        else
        {
            std::cout << __PRETTY_FUNCTION__ << " data path found: " << _data_path << std::endl;
        }
    }

    std::string const& get_data_path() const
    {
        return _data_path;
    }

    QString get_qdata_path() const
    {
        return QString::fromStdString(_data_path);
    }


private:
    static Data_config * _data_config;

    std::string _data_path;
};

#endif // DATA_CONFIG_H
