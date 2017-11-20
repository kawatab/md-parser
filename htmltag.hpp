// md-parser/htmltag.hpp - a set of html tags for markdown parser
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

#include <QList>
#include <QPair>
#include <QStringList>
#include <QRegularExpression>


class HTMLTag {
public:
  HTMLTag();
  HTMLTag(const HTMLTag& other) = delete;
  HTMLTag& operator=(const HTMLTag& other) = delete;
  ~HTMLTag();
  
  const QRegularExpression& type1CloseTag() const;
  const QList<QPair<QRegularExpression*, QRegularExpression*> >& type12345List() const;
  const QStringList& type6List() const;

private:
  void appendType12345(const QString& openTag, const QString& closeTag);

  QRegularExpression type1CloseTag_;
  QList<QPair<QRegularExpression*, QRegularExpression*> > type12345List_;
  QStringList type6List_;
};
