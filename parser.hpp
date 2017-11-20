// md-parser/parser.hpp - a markdown parser
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

#include <QMap>
#include <QPair>
#include "containerblock.hpp"

class ConainerBlock;


class Parser {
public:
  Parser();
  Parser(const Parser& other) = delete;
  Parser& operator=(const Parser& other) = delete;
  ~Parser();

  ContainerBlock* current();
  void defineLink(const QString& label, const QString& reference, const QString& title);
  QString getImageText(const QString& label) const;
  QString getImageText(const QString& label, const QString& description) const;
  QString getLinkText(const QString& label) const;
  QString getLinkText(const QString& label, const QString& text) const;
  QString getHTMLText(const QString& mdText);
  void setCurrent(ContainerBlock* container);
  bool unwind();
  bool unwindUntil(int indent);


private:
  ContainerBlock* current_;
  QMap<QString, QPair<QString, QString> > linkList_;

public:
  const QString inlineLinkTemplate1;
  const QString inlineLinkTemplate2;
  const QString inlineImageTemplate1;
  const QString inlineImageTemplate2;
};
