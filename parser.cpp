// md-parser/parser.cpp - a markdown parser
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


#include "parser.hpp"

#include <QVector>
#include "linehandler.hpp"
#include "inlineparser.hpp"


Parser::Parser()
  : current_(),
    linkList_(),
    inlineLinkTemplate1("<a href=\"%2\">%1</a>"),
    inlineLinkTemplate2("<a href=\"%2\" title=\"%3\">%1</a>"),
    inlineImageTemplate1("<img src=\"%2\" alt=\"%1\" />"),
    inlineImageTemplate2("<img src=\"%2\" alt=\"%1\" title=\"%3\" />")
{}

Parser::~Parser() {
}

ContainerBlock* Parser::current() {
  return this->current_;
}

void Parser::setCurrent(ContainerBlock* container) {
  this->current_ = container;
}
  
QString Parser::getHTMLText(const QString& mdText) {
  BodyBlock root;
  current_ = &root;
  root.setParser(this);
  linkList_.clear();

  for (QStringRef line : mdText.splitRef('\n')) {

    LineHandler lineHandler{line};

    if (!current()->dispatchBlankLine(lineHandler)) {
      while (!current()->dispatchIndentedCode(lineHandler) &&
	     !current()->dispatchLeafBlock(&lineHandler)) {
	lineHandler.skipWhitespace();

	if (!current()->dispatchContainerBlock(&lineHandler)) {
	  current()->dispatchHeadingAndParagraph(&lineHandler);

	  break;
	}
      }
    }
  }
  
  while (unwind()) {}

  root.close();

  return root.html();
}

bool Parser::unwind() {
  ContainerBlock* parent = current()->parent();
  
  if (!parent) return false;

  current()->close();
  current_ = parent;

  return true;
}

bool Parser::unwindUntil(int indent) {
  while (indent < current()->indent()) {
    if (!unwind()) break;
  }

  return true;
}

void Parser::defineLink(const QString& label, const QString& reference, const QString& title) {
  QString lowercaseLabel{label.toLower()};

  if (linkList_.contains(lowercaseLabel)) return;

  linkList_.insert(lowercaseLabel, qMakePair(reference, title));
}

QString Parser::getLinkText(const QString& label) const {
  QString lowercaseLabel{label.toLower()};

  auto link{linkList_.find(lowercaseLabel)};

  if (link == linkList_.end()) return QString();

  QString reference{link->first};
  QString title{link->second};
  QString parsedLabel{InlineParser(label, this).textToHTML()};

  return title.isEmpty() ?
    inlineLinkTemplate1.arg(parsedLabel, reference) :
    inlineLinkTemplate2.arg(parsedLabel, reference, title);
}

QString Parser::getLinkText(const QString& label, const QString& text) const {
  QString lowercaseLabel{label.toLower()};

  auto link{linkList_.find(lowercaseLabel)};

  if (link == linkList_.end()) return QString();

  QString reference{link->first};
  QString title{link->second};
  QString parsedLabel{InlineParser(text, this).textToHTML()};

  return title.isEmpty() ?
    inlineLinkTemplate1.arg(parsedLabel, reference) :
    inlineLinkTemplate2.arg(parsedLabel, reference, title);
}

QString Parser::getImageText(const QString& label) const {
  QString lowercaseLabel{label.toLower()};

  auto img{linkList_.find(lowercaseLabel)};

  if (img == linkList_.end()) return QString();

  QString reference{img->first};
  QString title{InlineParser(img->second, this).textToHTML()};
  QString alt{InlineParser(label, this).textToPlain()};

  return title.isEmpty() ?
    inlineImageTemplate1.arg(alt, reference) :
    inlineImageTemplate2.arg(alt, reference, title);
}

QString Parser::getImageText(const QString& label, const QString& description) const {
  QString lowercaseLabel{label.toLower()};

  auto img{linkList_.find(lowercaseLabel)};

  if (img == linkList_.end()) return QString();

  QString reference{img->first};
  QString title{InlineParser(img->second, this).textToHTML()};
  QString alt{InlineParser(description, this).textToPlain()};

  return title.isEmpty() ?
    inlineImageTemplate1.arg(alt, reference) :
    inlineImageTemplate2.arg(alt, reference, title);
}
