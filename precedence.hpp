// md-parser/precedence.hpp - an element of markdown parser
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


class Precedence {
public:
  explicit Precedence(const QString* text);
private:
  Precedence(QChar delimiter, int count, const QString* tag, const QString* text, int begin);

public:
  int begin() const;
  bool close(int pos);
  bool close(int* pos, Precedence* inner);
  int end() const;
  void init(QChar delimiter, const QString* tag, int begin);
  bool isEmpty() const;
  bool isAheadOf(int pos) const;
  bool isContinued(QChar delimiter, int pos) const;
  bool isContinued(const Precedence& other) const;
  bool isIncomplete() const;
  bool isLeftFlankingDelimiterRun(int pos, int size);
  bool isRightFlankingDelimiterRun(int pos, int size);
  bool isSameDelimiterAs(QChar delimiter) const;
  int htmlLeftPart(QString* applied, int lastPos) const;
  int htmlRightPart(QString* applied, int lastPos) const;
  bool open(int* pos, Precedence* second);
  int plainTextLeftPart(QString* applied, int lastPos) const;
  int plainTextRightPart(QString* applied, int lastPos) const;
  void reset();

private:
  QChar delimiter_;
  int count_;
  const QString* tag_;
  const QString* text_;
  int begin_;
  int end_;
};
