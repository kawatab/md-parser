// MD Parser - a markdown parser for CommonMark
//
// Copyright (C) 2017 Yasuhiro Yamakawa <kawatab@yahoo.co.jp>
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or any
//  later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
//  License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include <iostream>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QXmlStreamReader>
#include "mdparser_test.hpp"
#include "parser.hpp"

const char* program_name{"MD Parser"};
const char* descreption{"A markdown parser for CommonMark Spec v"};
const char* author{"Yasuhiro Yamakawa <kawatab@yahoo.co.jp>"};
const char* version{"0.1.0"};
const char* cmVersion{"0.28"};

const char* help_info{
  "usage: mdparser [<option> ...]\n"
    " File and expression options:\n"
    "  --author: show author\n"
    "  -h, --help : Show this information and exits, ignoring other options\n"
    "  -l <file>, --load <file> : Load and parse <filename>, prints results\n"
    "  -p <exprs>, --parse <exprs> : Parse <exprs>, prints results\n"
    "  -s, --spec : Show specification info\n"
    "  -t, --test : Run tests, ignoring other options\n"
    "  -v, --version : Show version\n"
    };

void parseList(const QStringList& list) {
  Parser parser{};
  
  for (QString expr : list) {
    expr.replace("\\n", "\n").replace("\\t", "\t");
    std::cout << qPrintable(parser.getHTMLText(expr)) << std::endl;
  }
}

void showAuthor() {
  std::cout << program_name << " was written by:" << std::endl;
  std::cout << "  " << author << std::endl;
}

void showVersion() {
  std::cout << program_name << " v" << version << std::endl;
}

void showSpec() {
  std::cout << "CommonMark Spec Version " << cmVersion << std::endl;
  std::cout << "see <http://spec.commonmark.org/>" << std::endl;
}

void showHelp() {
  showVersion();
  std::cout << descreption << cmVersion << std::endl;
  std::cout << help_info << std::flush;
}

void load(const QString& filename) {
  QFile mdFile{filename};

  if (!mdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning("File Load Problem\nCouldn't open %s.", qPrintable(filename));

    return;
  }

  QTextStream in{&mdFile};
  QString mdText{in.readAll()};
  Parser parser{};
  std::cout << qPrintable(parser.getHTMLText(mdText)) << std::endl;
}

void test() {
  QFile xmlFile{"test.xml"};

  if (!xmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning("Load XML File Problem\nCouldn't open test.xml.");
    return;
  }

  MDParser_test mdTest{&xmlFile};
  mdTest.run();
}

int main(int argc, char *argv[]) {
  QStringList argList{};

  for (int i{1}; i < argc; ++i) {
    argList.append(argv[i]);
  }
  
  if (argList.isEmpty()) {
    showHelp();
  } else if (argList[0].at(0) == '-') {
    if (argList[0] == "-h" || argList[0] == "--help") {
      showHelp();
    } else if (argList[0] == "--author") {
      showAuthor();
    } else if (argList[0] == "-v" || argList[0] == "--version") {
      showVersion();
    } else if (argList[0] == "-s" || argList[0] == "--spec") {
      showSpec();
    } else if (argList[0] == "-p" || argList[0] == "--parse") {
      argList.removeFirst();
      parseList(argList);
    } else if (argList[0] == "-l" || argList[0] == "--load") {
      if (argList.size() < 2) {
	qWarning("No file name");

	return 0;
      }

      load(argList[1]);
    } else if (argList[0] == "-t" || argList[0] == "--test") {
      test();
    } else {
      qWarning("%s: bad switch: %s\nUse the --help or -h flag for help.", argv[0], argv[1]);
    }
  } else {
    parseList(argList);
  }

  return 0;
}
