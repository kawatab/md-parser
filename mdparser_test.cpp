// md-parser/mdparser_test.cpp - test program
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


#include "mdparser_test.hpp"

#include <iostream>
#include <QFile>
#include <QXmlStreamReader>
#include "parser.hpp"


MDParser_test::MDParser_test(QFile* xmlFile)
  : xmlFile_(xmlFile)
{}

void MDParser_test::run() {
  QXmlStreamReader xmlReader{xmlFile_};
  Parser parser;
  QString mdText;
  QString htmlText;
  int okCount{0};
  int faultCount{0};

  while(!xmlReader.atEnd() && !xmlReader.hasError()) {
    QXmlStreamReader::TokenType token{xmlReader.readNext()};

    if(token == QXmlStreamReader::StartDocument) {
      continue;
    }

    if(token == QXmlStreamReader::StartElement) {
      if(xmlReader.name() == "items") {
	continue;
      }
      
      if(xmlReader.name() == "markdown") {
	mdText = xmlReader.readElementText();
      }
    }

    if (xmlReader.atEnd() || xmlReader.hasError()) {
      break;
    }
    
    token = xmlReader.readNext();

    if(token == QXmlStreamReader::StartElement) {
      if(xmlReader.name() == "html") {
	htmlText = xmlReader.readElementText();

	QString result{parser.getHTMLText(mdText)};

	if (result == htmlText) {
	  ++okCount;
	} else {
	  --faultCount;
	  std::cout << "test " << okCount + faultCount << ":" << std::endl;
	  std::cout << qPrintable(result) << std::endl;
	}
      }
    }
  }

  if(xmlReader.hasError()) {
    qCritical("xmlFile.xml Parse Error\n%s", qPrintable(xmlReader.errorString()));

    return;
  }

  std::cout << "Success: " << okCount << std::endl;
  std::cout << "Fault: " << faultCount << std::endl;
}
