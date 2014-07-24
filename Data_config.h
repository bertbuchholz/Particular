#ifndef DATA_CONFIG_H
#define DATA_CONFIG_H

#include <iostream>
#include <cassert>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
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

    QString get_absolute_qfilename(QString const& relative_path, bool const check_existence = true) const;

    std::string get_absolute_filename(QString const& relative_path, bool const check_existence = true) const;

//    std::string get_absolute_filename(std::string const& relative_path) const
//    {
//        return get_absolute_filename(QString::fromStdString(relative_path));
//    }

private:
    void init();

    static Data_config * _data_config;

    std::string _data_path;
    QStringList _data_paths;
};

#endif // DATA_CONFIG_H
