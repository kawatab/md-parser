// md-parser/texthandler.hpp - a text handler of markdown parser
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


#pragma once

#include <QString>


class TextHandler {
public:
  explicit TextHandler(const QString& text);
  explicit TextHandler(const QStringRef& text);

  bool isAutolink() const;
  QString convertEntityReferecence() const;
  QString convertToPercentEncoding() const;
  QString findLinkLabel(int* begin) const;
  QString findLinkLabel(int* begin, QChar nextChar) const;
  QString findLinkReference(int* begin) const;
  QString findLinkTitle(int* begin, bool* ok) const;
  QString rest(int begin) const;
  int skipTagName(int begin) const;
  int skipAttribute(int begin) const;
  int skipAttributeName(int begin) const;
  int skipCDATASection(int begin) const;
  int skipCloseTag(int begin) const;
  int skipDeclaration(int begin) const;
  int skipDoubleQuotedAttributeValue(int begin) const;
  int skipHTMLBlock(int begin) const;
  int skipHTMLComment(int begin) const;
  int skipOpenTag(int begin) const;
  int skipProcessingInstruction(int begin) const;
  int skipQuotedAttributeValue(int begin, QChar delimiter) const;
  int skipSingleQuotedAttributeValue(int begin) const;
  int skipUnquotedAttributeValue(int begin) const;
  int skipWhitespace(int begin, bool* hasLinebreak = nullptr) const;

private:
  QStringRef text_;
};
