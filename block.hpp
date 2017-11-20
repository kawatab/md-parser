// md-parser/block.hpp - an element for markdown parser
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

class ContainerBlock;
class HeadingBlock;
class LineHandler;


class Block {
public:
  Block() = delete;
  Block(ContainerBlock* parent);
  Block(const Block& other) = delete;
  Block& operator=(const Block& other) = delete;
  virtual ~Block();

  virtual bool appendFencedCodeText(const LineHandler& lineHandler);
  virtual bool appendHTMLBlockText(const LineHandler& lineHandler);
  virtual void appendLine(const LineHandler& lineHandler);
  virtual bool appendIndentedText(LineHandler* lineHandler) = 0;
  virtual bool appendParagraphText(const LineHandler& lineHandler) = 0;
  virtual int baseIndent() const;
  virtual void close();
  virtual bool closeHTMLBlock(const LineHandler& lineHandler);
  virtual HeadingBlock* convertToSetextHeading(const LineHandler& lineHandler);
  virtual QString fence() const;
  virtual void handleBlankLine(const LineHandler& lineHandler);
  virtual QString html() const = 0;
  virtual bool isFencedCodeBlock() const;
  virtual bool isParagraph() const;
  virtual bool toggleFencedCodeBlock(QChar fenceChar, int count, const LineHandler& rest, int indent);

  void disable();
  ContainerBlock* parent();
  const ContainerBlock* parent() const;
  bool writable() const;

private:
  ContainerBlock* parent_;
  bool writable_;
};
