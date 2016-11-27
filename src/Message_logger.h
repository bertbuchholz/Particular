#ifndef MESSAGE_LOGGER_H
#define MESSAGE_LOGGER_H

#include <memory>

#include <QFile>
#include <QTextStream>
#include <QMutex>

class Message_logger
{
public:
    static Message_logger * get_instance();

    void init(QString const& debug_file);

    void handle_message(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    Message_logger();

    static Message_logger * _instance;

    std::unique_ptr<QFile> _log_file;
    QTextStream _log_stream;
    QMutex _mutex;
    bool _debug_mode;
};

#endif // MESSAGE_LOGGER_H
