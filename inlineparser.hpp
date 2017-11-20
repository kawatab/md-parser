// md-parser/inlineparser.hpp - a parser for inline text
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

#include <QStack>
#include <QString>

#include "precedence.hpp"

class Parser;


class InlineParser {
public:
  InlineParser(const QString& line, const Parser* parser);

  QString codeToHTML();
  QString textToHTML();
  QString textToPlain();

private:
  int applyAutolink(int begin, int pos);
  int applyEmailAutolink(int begin, int pos);
  int applyFullReferenceImage(int begin, int pos, const QString& description, bool isHTML);
  int applyFullReferenceLink(int begin, int pos, const QString& linkText, bool isHTML);
  int applyImage(int begin, int pos, bool isHTML);
  int applyInlineImage(int begin, int pos, const QString& label, bool isHTML);
  int applyInlineLink(int begin, int pos, const QString& label, bool isHTML);
  int applyLink(int begin, int pos, const QString& linkLabel, bool isHTML);
  int applyShortcutReferenceImage(int begin, int pos, const QString& linkLabel, bool isHTML);
  int applyShortcutReferenceLink(int begin, int pos, const QString& linkLabel, bool isHTML);
  bool closePrecedence(int* pos, QStack<Precedence*>* pending) const;
  bool failToClosePrecedence(QStack<Precedence*>* stack, QStack<Precedence*>* pending) const;
  int findLinkDestination(int pos, QString* destination) const;
  int findLinkTitle(int pos, QString* title) const;
  bool findSameDelimiter(int pos, QStack<Precedence*>* pending, QStack<Precedence*>* stack) const;
  QLinkedList<Precedence> parse(bool isHTML = true);
  int replaceAutolink(int begin);
  int replaceCodeSpan(int begin);
  int replaceImage(int begin, bool isHTML);
  int replaceLink(int begin, bool isHTML);
  int replaceSpecialCharacter(int pos);
  int replaceSquareBrackets(int begin);
  int replaceWhitespace(int begin);
  int skipEmphasis(int pos, QLinkedList<Precedence>* split, QStack<Precedence*>* pending) const;
  int skipTagName(int begin) const;
  int skipWhitespace(int pos) const;

  QString line_;
  const Parser* parser_;
};
