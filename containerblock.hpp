// md-parser/containerblock.hpp - an element for markdown parser
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
#include "block.hpp"

class BulletListItem;
class LeafBlock;
class OrderedListItem;
class Parser;


class ContainerBlock : public Block {
public:
  ContainerBlock() = delete;
  ContainerBlock(const ContainerBlock& other) = delete;
  ContainerBlock(Parser* parser, ContainerBlock* parent, int indent);
  ContainerBlock(ContainerBlock* parent, int indent);
  ContainerBlock& operator=(const ContainerBlock& other) = delete;
  virtual ~ContainerBlock() override;

  bool appendFencedCodeText(const LineHandler& lineHandler) override;
  bool appendIndentedText(LineHandler* lineHandler) override;
  bool appendParagraphText(const LineHandler& lineHandler) override;
  void close() override;

  virtual void appendBulletList(QChar bullet, int baseIndent, int indent, bool hasBlankline);
  virtual void appendContainerBlock(ContainerBlock* block);
  virtual void appendFirstBulletList(LineHandler* lineHandler, const QString& bullet, int baseIndent);
  virtual void appendFirstOrderedList(LineHandler* lineHandler, qulonglong begin, QChar separator, int baseIndent, int makerLength);
  virtual void appendLeafBlock(LeafBlock* block);
  virtual void appendOrderedList(QChar separator, int baseIndent, int indent, int markerLength, bool hasBlankline);
  virtual bool appendParagraph(const LineHandler& lineHandler);
  virtual void appendBlockQuote(const LineHandler& lineHandler);
  virtual bool dispatchBlankLine(const LineHandler& lineHandler);
  virtual bool dispatchBulletList(LineHandler* lineHandler);
  virtual bool dispatchContainerBlock(LineHandler* lineHandler);
  virtual bool dispatchIndentedCode(const LineHandler& lineHandler);
  virtual bool dispatchOrderedList(LineHandler* lineHandler);
  virtual bool dispatchSetextHeading(const LineHandler& lineHandler);
  virtual bool hasBlankline() const;
  virtual bool isIndentEnoughForChild(int indent) const;
  virtual void setHasBlankline(bool hasBlankline);

  const QList<Block*> children() const;
  int depth() const;
  void dispatchHeadingAndParagraph(LineHandler* lineHandler);
  bool dispatchLeafBlock(LineHandler* lineHandler);
  Block* first();
  const Block* first() const;
  int indent() const;
  bool isEmpty() const;
  Block* last();
  const Block* last() const;
  void removeLast();
  Parser* parser();
  const Parser* parser() const;
  void setParser(Parser* parser);

protected:
  bool dispatchBlockQuote(LineHandler* lineHandler);
  
private:
  void appendBlock(Block* block);
  void appendThematicBreak();
  bool dispatchFencedCodeBlock(const LineHandler& lineHandler);
  bool dispatchHeadingBlock(LineHandler* lineHandler);
  bool dispatchHTMLBlock(LineHandler* lineHandler);
  bool dispatchNoText(const LineHandler& lineHandler);
  bool dispatchThematicBreak(const LineHandler& lineHandler);

  Parser* parser_;
  QList<Block*> children_;

protected:
  int depth_;
  int indent_;
};

class BodyBlock : public ContainerBlock {
public:
  BodyBlock();
  ~BodyBlock() override;

  bool appendParagraph(const LineHandler& lineHandler) override;
  QString html() const override;
};

class ListBlock : public ContainerBlock {
public:
  ListBlock() = delete;
  ListBlock(ContainerBlock* parent, int indent);
  ~ListBlock() override;

  void appendFirstBulletList(LineHandler* lineHandler, const QString& bullet, int baseIndent) override;
  void appendFirstOrderedList(LineHandler* lineHandler, qulonglong begin, QChar separator, int baseIndent, int markerLength) override;
  int baseIndent() const override;
  bool dispatchBulletList(LineHandler* lineHandler) override;
  bool dispatchIndentedCode(const LineHandler& lineHandler) override;
  bool dispatchOrderedList(LineHandler* lineHandler) override;
  bool hasBlankline() const override;
  void setHasBlankline(bool hasBlankline) override;

protected:
  bool hasBlankline_;
};

class ListItem : public ContainerBlock {
public:
  ListItem() = delete;
  ListItem(ContainerBlock* parent, int indent, int baseIndent);
  ~ListItem() override;

  void appendContainerBlock(ContainerBlock* block) override;
  void appendLeafBlock(LeafBlock* block) override;
  bool appendParagraph(const LineHandler& lineHandler) override;
  int baseIndent() const override;
  bool dispatchBlankLine(const LineHandler& lineHandler) override;
  bool dispatchIndentedCode(const LineHandler& lineHandler) override;
  bool dispatchSetextHeading(const LineHandler& lineHandler) override;
  bool hasBlankline() const override;
  void setHasBlankline(bool hasBlankline) override;

protected:
  virtual bool isFollowedBy(LineHandler* lineHandler, int indent) const = 0;

  bool hasBlankline_;
private:
  int baseIndent_;
};

class BulletListItem : public ListItem {
public:
  BulletListItem() = delete;
  BulletListItem(ContainerBlock* parent, int indent, const QString& bullet, int baseIndent);
  ~BulletListItem() override;

  bool dispatchBulletList(LineHandler* lineHandler) override;
  bool dispatchContainerBlock(LineHandler* lineHandler) override;
  QString html() const override;
  bool isFollowedBy(LineHandler* lineHandler, int indent) const override;
  bool isIndentEnoughForChild(int indent) const override;

  QString bullet() const;

private:
  QString bullet_;
};

class BulletListBlock : public ListBlock {
public:
  BulletListBlock() = delete;
  BulletListBlock(ContainerBlock* parent, int indent);
  ~BulletListBlock() override;

  void appendBulletList(QChar bullet, int baseIndent, int indent, bool hasBlankline) override;
  QString html() const override;
};

class OrderedListItem : public ListItem {
public:
  OrderedListItem() = delete;
  OrderedListItem(ContainerBlock* parent, int indent, QChar separator, int baseIndent, int markerLength);
  ~OrderedListItem() override;

  bool dispatchContainerBlock(LineHandler* lineHandler) override;
  bool dispatchOrderedList(LineHandler* lineHandler) override;
  QString html() const override;
  bool isFollowedBy(LineHandler* lineHandler, int indent) const override;
  bool isIndentEnoughForChild(int indent) const override;

  int markerLength() const;

private:
  QChar separator_;
  int markerLength_;
};

class OrderedListBlock : public ListBlock {
public:
  OrderedListBlock() = delete;
  OrderedListBlock(ContainerBlock* parent, int indent, qulonglong begin);
  ~OrderedListBlock() override;

  void appendOrderedList(QChar separator, int baseIndent, int indent, int markerLength, bool hasBlankline) override;
  QString html() const override;

private:
  qulonglong begin_;
};

class BlockQuoteBlock : public ContainerBlock {
public:
  BlockQuoteBlock() = delete;
  BlockQuoteBlock(ContainerBlock* parent, int indent);
  ~BlockQuoteBlock() override;

  void appendBlockQuote(const LineHandler& lineHandler) override;
  bool appendFencedCodeText(const LineHandler& lineHandler) override;
  bool appendParagraph(const LineHandler& lineHandler) override;
  bool dispatchBlankLine(const LineHandler& lineHandler) override;
  bool dispatchContainerBlock(LineHandler* lineHandler) override;
  bool dispatchIndentedCode(const LineHandler& lineHandler) override;
  bool dispatchSetextHeading(const LineHandler& lineHandler) override;
  void handleBlankLine(const LineHandler& lineHandler) override;
  QString html() const override;

private:
  bool dispatchBlockQuote(LineHandler* lineHandler);
};
