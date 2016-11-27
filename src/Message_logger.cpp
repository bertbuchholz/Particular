#include "Message_logger.h"

#include <QtMessageHandler>

void handle_message_func(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
   Message_logger::get_instance()->handle_message(type, context, msg);
}

Message_logger * Message_logger::_instance = nullptr;

void Message_logger::init(QString const& debug_file)
{
    _debug_mode = true;

    _log_file = std::unique_ptr<QFile>(new QFile(debug_file));
    _log_file->open(QIODevice::WriteOnly | QIODevice::Truncate);

    _log_stream.setDevice(_log_file.get());

    _log_stream << "initialized.\n";
    _log_stream.flush();

    qInstallMessageHandler(&handle_message_func);
}

Message_logger *Message_logger::get_instance()
{
    if (!_instance)
    {
        _instance = new Message_logger();
    }

    return _instance;
}


void Message_logger::handle_message(QtMsgType type, QMessageLogContext const& context, QString const& msg)
{
    if (!_debug_mode) return;

    _mutex.lock();
    switch (type)
    {
    case QtDebugMsg:
    case QtInfoMsg:
        _log_stream << "File: " << context.file << " fnc: " << context.function << " line: " << context.line << "\n";
        _log_stream << msg;
        _log_stream.flush();
        break;
    case QtWarningMsg:
        _log_stream << "\n*** Warning ***\n";
        _log_stream << "File: " << context.file << " fnc: " << context.function << " line: " << context.line << "\n";
        _log_stream << msg;
        _log_stream << "\n*** Warning Complete ***\n";
        _log_stream.flush();
        break;
    case QtCriticalMsg:
        _log_stream << "\n*** Critical ***\n";
        _log_stream << "File: " << context.file << " fnc: " << context.function << " line: " << context.line << "\n";
        _log_stream << msg;
        _log_stream << "\n*** Critical Complete ***\n";
        _log_stream.flush();
        break;
    case QtFatalMsg:
        _log_stream << "\n*** Fatal ***\n";
        _log_stream << "File: " << context.file << " fnc: " << context.function << " line: " << context.line << "\n";
        _log_stream << msg;
        _log_stream << "\n*** Fatal Complete ***\n";
        _log_stream.flush();
        abort();
    }

    _mutex.unlock();

}

Message_logger::Message_logger() : _debug_mode(false)
{

}
