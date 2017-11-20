// md-parser/character.hpp - character class for markdown parser
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


class EscapeChar {
public:
  EscapeChar();
  EscapeChar(const QString& str, int inputLength);
  EscapeChar(QChar chr, int inputLength);

  bool isEmpty() const;
  int inputLength() const;
  QString output() const;

  static EscapeChar get(const QStringRef& text);

private:
  static EscapeChar getBackslashEscaped(const QStringRef& text);
  static EscapeChar getEntityWithCode(const QStringRef& text);
  static EscapeChar getEntityWithHex(const QStringRef& text);
  static EscapeChar getEntityWithText(const QStringRef& text);

  QString str_;
  int inputLength_;
};

class EntityChar {
public:
  EntityChar();
private:
  explicit EntityChar(const QString& str);

public:
  bool isEmpty() const;
  QString output() const;

  static EntityChar get(QChar chr);

private:
  QString str_;
};
