// md-parser/linehandler.hpp - a text handler for markdown parser
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
#include <QRegularExpressionMatch>


class LineHandler {
public:
  explicit LineHandler(const QStringRef& line);
  
  int countIndent() const;
  QString currentText() const;
  int depth() const;
  QChar findBullet();
  int findHeadingMarker();
  QStringRef findDigit();
  QStringRef firstWord() const;
  int indent() const;
  int indexOf(QChar chr) const;
  bool isAutolink() const;
  bool isBlank() const;
  bool isHTMLTagType7() const;
  bool matchBlockQuote();
  bool matchHTMLCloseTag(const QRegularExpression& tag) const;
  bool matchHTMLOpenTag(const QRegularExpression& tag) const;
  bool matchHTMLTag(const QStringList& tagList) const;
  QRegularExpressionMatch matchHTMLTag() const;
  QString noWhitespace() const;
  int position() const;
  QString putLinebreakAtBOL() const;
  QString putLinebreakAtEOL() const;
  LineHandler removeIndent(int indent) const;
  void removeLastSequence(QChar chr);
  int skipFenceChar(QChar fenceChr);
  void skipWhitespace();
  QString trimmed() const;

private:
  bool findBullet(QChar bullet);
  bool skipWhitespaceFollowedListMarker(int pos2, int logical2);

  static const int TAB_SIZE;

  LineHandler(const QStringRef& line, int physicalPosition, int logicalPosition, int offset, int depth, int indent);

  QStringRef currentTextRef() const;

  QStringRef line_;
  int physicalPosition_;
  int logicalPosition_;
  int offset_;
  int depth_;
  int indent_;
};
