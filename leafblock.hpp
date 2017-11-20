// md-parser/leafblock.hpp - an element for markdown parser
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

#include "block.hpp"


class LeafBlock : public Block {
public:
  LeafBlock() = delete;
  LeafBlock(ContainerBlock* parent, const QString& text);
  virtual ~LeafBlock() override;

  virtual bool appendIndentedText(LineHandler* lineHandler) override;
  virtual bool appendParagraphText(const LineHandler& lineHandler) override;

  void appendLine(const LineHandler& lineHandler) override;

  void appendLine2(const LineHandler& lineHandler);

protected:
  QString text_;
};

class ParagraphBlock : public LeafBlock {
public:
  ParagraphBlock() = delete;
  ParagraphBlock(ContainerBlock* parent, const LineHandler& lineHandler);
  ~ParagraphBlock() override;

  bool appendIndentedText(LineHandler* lineHandler) override;
  bool appendParagraphText(const LineHandler& lineHandler) override;
  void close() override;
  HeadingBlock* convertToSetextHeading(const LineHandler& lineHandler) override;
  void handleBlankLine(const LineHandler& lineHandler) override;
  QString html() const override;
  bool isParagraph() const override;
};

class IndentedCodeBlock : public LeafBlock {
public:
  static const int INDENT_SIZE;

  IndentedCodeBlock() = delete;
  IndentedCodeBlock(ContainerBlock* parent, const LineHandler& lineHandler);
  ~IndentedCodeBlock() override;

  bool appendIndentedText(LineHandler* lineHandler) override;
  void handleBlankLine(const LineHandler& lineHandler) override;
  QString html() const override;

private:
  QString pending_;
};

class FencedCodeBlock : public LeafBlock {
public:
  FencedCodeBlock() = delete;
  FencedCodeBlock(ContainerBlock* parent, QChar fence, int count, const QString& rest, int indent);
  ~FencedCodeBlock() override;

  bool appendFencedCodeText(const LineHandler& lineHandler) override;
  bool appendIndentedText(LineHandler* lineHandler) override;
  void handleBlankLine(const LineHandler& lineHandler) override;
  QString html() const override;
  bool toggleFencedCodeBlock(QChar fenceChar, int count, const LineHandler& rest, int indent) override;

private:
  QChar fence_;
  int count_;
  QString rest_;
  int indent_;
};

class HeadingBlock : public LeafBlock {
public:
  HeadingBlock() = delete;
  HeadingBlock(ContainerBlock* parent, const QString& line, int level);
  HeadingBlock(ContainerBlock* parent, const LineHandler& lineHandler, int level);
  ~HeadingBlock() override;

  void handleBlankLine(const LineHandler& lineHandler) override;
  QString html() const override;

private:
  QString tag_;
};

class ThematicBreak : public LeafBlock {
public:
  ThematicBreak() = delete;
  explicit ThematicBreak(ContainerBlock* parent);
  ~ThematicBreak() override;

  QString html() const override;
};

class HTMLBlock : public LeafBlock {
public:
  HTMLBlock() = delete;
  HTMLBlock(ContainerBlock* parent, const LineHandler& lineHandler);
  ~HTMLBlock() override;

  bool appendHTMLBlockText(const LineHandler& lineHandler) override;
  bool appendIndentedText(LineHandler* lineHandler) override;
  bool closeHTMLBlock(const LineHandler& lineHandler) override;
  HeadingBlock* convertToSetextHeading(const LineHandler& lineHandler) override;
  void handleBlankLine(const LineHandler& lineHandler) override;
  QString html() const override;
};

class HTMLBlockWithCloseTag : public HTMLBlock {
public:
  HTMLBlockWithCloseTag() = delete;
  HTMLBlockWithCloseTag(ContainerBlock* parent, const LineHandler& lineHandler, const QRegularExpression* closeTag);
  ~HTMLBlockWithCloseTag() override;

  bool closeHTMLBlock(const LineHandler& lineHandler) override;
  void handleBlankLine(const LineHandler& lineHandler) override;

private:
  const QRegularExpression* closeTag_;
};
