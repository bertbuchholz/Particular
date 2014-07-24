#include "Data_config.h"

Data_config * Data_config::_data_config = nullptr;


QString Data_config::get_absolute_qfilename(const QString &relative_path, const bool check_existence) const
{
    return QString::fromStdString(get_absolute_filename(relative_path, check_existence));
}

std::string Data_config::get_absolute_filename(const QString &relative_path, const bool check_existence) const
{
    for (QString const& path : _data_paths)
    {
        QString absolute_filename = path + "/" + relative_path;

        if (check_existence)
        {
            if (QFile::exists(absolute_filename))
            {
                return absolute_filename.toStdString();
            }
        }
        else
        {
            QDir dir(path);

            if (dir.exists())
            {
                return absolute_filename.toStdString();
            }
        }
    }

    throw std::runtime_error("No such file.");
}

void Data_config::init()
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
        std::cout << __FUNCTION__ << " no data paths found" << std::endl;

        QMessageBox e;
        e.setText("Data path could not be found. Make sure the folder data is in the same directory as the executable.");
        e.exec();

        assert(false);
    }
    else
    {
        for (QString const p : _data_paths)
        {
            std::cout << __FUNCTION__ << " data path found: " << p.toStdString() << std::endl;
        }
    }
}
