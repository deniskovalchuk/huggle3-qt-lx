//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "exception.hpp"
#include <iostream>
#include <QDir>
#include "configuration.hpp"
#include "syslog.hpp"

// Test if we are in GNU/Linux
// __linux__ is defined also in Android, which however isn't GNU
#if defined(__linux__) && !defined(HUGGLE_ANDROID)
    // linux code goes here
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
static QString Breakpad_DumpPath = "/tmp";
#elif defined HUGGLE_WIN
    // windows code goes here
#include <windows.h>
#include <winbase.h>
#include <dbghelp.h>
#include <winnt.h>
QString Breakpad_DumpPath = QDir::tempPath();
#endif

using namespace Huggle;

#ifdef HUGGLE_BREAKPAD
google_breakpad::ExceptionHandler *Exception::GoogleBP_handler = NULL;
    #if _MSC_VER
        #pragma warning ( push )
        #pragma warning ( disable : 4100 )
    #else
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-parameter"
    #endif

#if HUGGLE_BREAKPAD == 0
    google_breakpad::MinidumpDescriptor *Exception::GoogleBP_descriptor = NULL;
    static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
    {
        std::cout << "Dump path: " << descriptor.path() << std::endl;
        return succeeded;
    }
#endif

#if HUGGLE_BREAKPAD == 1
    // windows
    const wchar_t kPipeName[] = L"\\\\.\\pipe\\BreakpadCrashServices\\Huggle";
    bool dumpCallback(const wchar_t* dump_path, const wchar_t* minidump_id, void* context, EXCEPTION_POINTERS* exinfo,
                      MDRawAssertionInfo* assertion, bool succeeded) {

        if (succeeded)
        {
            std::cout << "Dump generated in " + Breakpad_DumpPath.toStdString() << std::endl;
        } else
        {
            std::cout << "Failed to generate dump in " + Breakpad_DumpPath.toStdString() << std::endl;
        }
      return succeeded;
    }
#endif

    #if _MSC_VER
        #pragma warning ( pop )
    #else
        #pragma GCC diagnostic pop
    #endif
#endif

Exception::Exception(const QString &text, bool is_recoverable)
{
    this->construct(text, "{hidden}", is_recoverable);
}

Exception::Exception(const QString &text, const QString &source, bool is_recoverable)
{
    this->construct(text, source, is_recoverable);
}

Exception::Exception(const QString &text, const char *source)
{
    this->construct(text, QString(source), true);
}

bool Exception::IsRecoverable() const
{
    return this->isRecoverable;
}

void Exception::construct(const QString &text, const QString &source, bool is_recoverable)
{
    std::cerr << "FATAL Exception thrown: " + text.toStdString() << std::endl;
    this->Source = source;
    this->StackTrace = GetCurrentStackTrace();
    this->Message = text;
    this->ErrorCode = 2;
    this->isRecoverable = is_recoverable;
}

void Exception::ThrowSoftException(const QString &text, const QString &source)
{
    if (Configuration::HuggleConfiguration->Verbosity > 0)
    {
        throw new Huggle::Exception(text, source);
    } else
    {
        Syslog::HuggleLogs->WarningLog("Soft exception: " + text + " source: " + source);
    }
}

QString Exception::GetCurrentStackTrace()
{
    QString result;
#if defined(__linux__) && !defined(HUGGLE_ANDROID)
    result = "";
    void *array[HUGGLE_STACK];
    unsigned int i;
    size_t size;
    char **messages;
    size = static_cast<size_t>(backtrace(array, HUGGLE_STACK));
    messages = backtrace_symbols(array, static_cast<int>(size));
    for (i = 1; i < size && messages != nullptr; ++i)
    {
        result += QString(QString::number(i) + QString(" ") + QString(messages[i]) + QString("\n"));
    }
    free(messages);
#elif defined HUGGLE_WIN
    result = "";
    unsigned int   i;
    void          *stack[HUGGLE_STACK];
    unsigned short frames;
    SYMBOL_INFO   *symbol;
    HANDLE         process;
    process = GetCurrentProcess();
    SymInitialize( process, NULL, TRUE );
    frames               = CaptureStackBackTrace( 0, HUGGLE_STACK, stack, NULL );
    symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
    if (!symbol)
        return "Failed to retrieve stack trace";
    symbol->MaxNameLen   = 255;
    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
    for( i = 0; i < frames; i++ )
    {
        SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );
        QString symbol_name = "unknown symbol";
        if (!QString(symbol->Name).isEmpty())
        symbol_name = QString(symbol->Name);
        result += QString(QString::number(frames - i - 1) + QString(" ") + symbol_name + QString(" 0x") +
                          QString::number(symbol->Address, 16) + QString("\n"));
    }
    free( symbol );
#else
    result = "Stack trace not available for this OS";
#endif
    return result;
}

void Exception::InitBreakpad()
{
#ifdef HUGGLE_BREAKPAD
    #if HUGGLE_BREAKPAD == 0
        // linux code
        Exception::GoogleBP_descriptor = new google_breakpad::MinidumpDescriptor("/tmp");
        Exception::GoogleBP_handler = new google_breakpad::ExceptionHandler(*Exception::GoogleBP_descriptor,  NULL,
                                                                             dumpCallback, NULL, true, -1);
    #endif
    #if HUGGLE_BREAKPAD == 1
        // windows code
        Exception::GoogleBP_handler = new google_breakpad::ExceptionHandler(Breakpad_DumpPath.toStdWString(), NULL, dumpCallback,
                                                                            NULL, google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                                            MiniDumpNormal, kPipeName, NULL);
    #endif
#endif
}

void Exception::ExitBreakpad()
{
#ifdef HUGGLE_BREAKPAD
    #if HUGGLE_BREAKPAD == 0
        delete Exception::GoogleBP_descriptor;
    #endif
    delete Exception::GoogleBP_handler;
#endif
}


NullPointerException::NullPointerException(const QString &name, const QString &source) : Exception("", source)
{
    this->ErrorCode = 1;
    this->Message = QString("Null pointer exception. The variable you referenced (") + name + ") had null value.";
}
