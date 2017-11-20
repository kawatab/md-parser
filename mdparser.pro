TEMPLATE = app
TARGET = mdparser
INCLUDEPATH += .

# Input
HEADERS += block.hpp \
           character.hpp \
           containerblock.hpp \
           htmltag.hpp \
           inlineparser.hpp \
           leafblock.hpp \
           linehandler.hpp \
           mdparser_test.hpp \
           parser.hpp \
           precedence.hpp \
           texthandler.hpp

SOURCES += block.cpp \
           character.cpp \
           containerblock.cpp \
           htmltag.cpp \
           inlineparser.cpp \
           leafblock.cpp \
           linehandler.cpp \
           main.cpp \
           mdparser_test.cpp \
           parser.cpp \
           precedence.cpp \
           texthandler.cpp

CONFIG += c++11 \
    debug
