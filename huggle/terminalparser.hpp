//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef TERMINALPARSER_H
#define TERMINALPARSER_H

#include "definitions.hpp"

#include <QStringList>
#include <QString>

namespace Huggle
{
    //! Parses the data provided by user in parameters to executable
    class TerminalParser
    {
        public:
            TerminalParser(int argc, char *argv[]);
            TerminalParser(QStringList argv);
            /*!
             * \brief Checks the given arguments for an argument requesting help
             */
            bool Init();

            /*!
             * \brief Parses the given arguments, returning true if it completes the requested operation, and false if it completes nothing
             */
            bool Parse();

            /*!
             * \brief Parses a single letter argument
             */
            bool ParseChar(QChar x);

            /*!
             * \brief Displays help text listing the different arguments 
             */
            void DisplayHelp();

            //! Whether or not output should be displayed
            bool Silent = false;
        private:
            QStringList args;
    };
}

#endif // TERMINALPARSER_H
