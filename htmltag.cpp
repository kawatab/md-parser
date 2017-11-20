// md-parser/htmltag.cpp - a set of html tags for markdown parser
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


#include "htmltag.hpp"


HTMLTag::HTMLTag()
  : type1CloseTag_("</(script|pre|style)>"),
    type12345List_(),
    type6List_()
{
  // HTML tag (type 1)
  type12345List_.append(qMakePair(new QRegularExpression("^\\s*<(script|pre|style)([\\s>].*|)$"), new QRegularExpression(type1CloseTag_)));
  
  // HTML tag (type 2, 3, 4 and 5)
  appendType12345("^\\s*<!--", "-->");
  appendType12345("^\\s*<\\?", "\\?>");
  appendType12345("^\\s*<![A-Z]", ">");
  appendType12345("^\\s*<!\\[CDATA\\[", "]]>");

  // HTML tag (type 6)
  type6List_.prepend("address");
  type6List_.prepend("article");
  type6List_.prepend("aside");
  type6List_.prepend("base");
  type6List_.prepend("basefont");
  type6List_.prepend("blockquote");
  type6List_.prepend("body");
  type6List_.prepend("caption");
  type6List_.prepend("center");
  type6List_.prepend("col");
  type6List_.prepend("colgroup");
  type6List_.prepend("dd");
  type6List_.prepend("details");
  type6List_.prepend("dialog");
  type6List_.prepend("dir");
  type6List_.prepend("div");
  type6List_.prepend("dl");
  type6List_.prepend("dt");
  type6List_.prepend("fieldset");
  type6List_.prepend("figcaption");
  type6List_.prepend("figure");
  type6List_.prepend("footer");
  type6List_.prepend("form");
  type6List_.prepend("frame");
  type6List_.prepend("frameset");
  type6List_.prepend("h1");
  type6List_.prepend("h2");
  type6List_.prepend("h3");
  type6List_.prepend("h4");
  type6List_.prepend("h5");
  type6List_.prepend("h6");
  type6List_.prepend("head");
  type6List_.prepend("header");
  type6List_.prepend("hr");
  type6List_.prepend("html");
  type6List_.prepend("iframe");
  type6List_.prepend("legend");
  type6List_.prepend("li");
  type6List_.prepend("link");
  type6List_.prepend("main");
  type6List_.prepend("menu");
  type6List_.prepend("menuitem");
  type6List_.prepend("meta");
  type6List_.prepend("nav");
  type6List_.prepend("noframes");
  type6List_.prepend("ol");
  type6List_.prepend("optgroup");
  type6List_.prepend("option");
  type6List_.prepend("p");
  type6List_.prepend("param");
  type6List_.prepend("section");
  type6List_.prepend("source");
  type6List_.prepend("summary");
  type6List_.prepend("table");
  type6List_.prepend("tbody");
  type6List_.prepend("td");
  type6List_.prepend("tfoot");
  type6List_.prepend("th");
  type6List_.prepend("thead");
  type6List_.prepend("title");
  type6List_.prepend("tr");
  type6List_.prepend("track");
  type6List_.prepend("ul");
}

HTMLTag::~HTMLTag() {
  for (auto pair : type12345List_) {
    delete pair.first;
    delete pair.second;
  }
}

void HTMLTag::appendType12345(const QString& openTag, const QString& closeTag) {
  type12345List_.append(qMakePair(new QRegularExpression(openTag), new QRegularExpression(closeTag)));
}

const QRegularExpression& HTMLTag::type1CloseTag() const {
  return type1CloseTag_;
}

const QList<QPair<QRegularExpression*, QRegularExpression*> >& HTMLTag::type12345List() const {
  return type12345List_;
}

const QStringList& HTMLTag::type6List() const {
  return type6List_;
}

