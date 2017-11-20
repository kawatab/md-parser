// md-parser/block.cpp - an element for markdown parser
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


#include "block.hpp"

#include "containerblock.hpp"
#include "leafblock.hpp"


Block::Block(ContainerBlock* parent)
  : parent_(parent),
    writable_(true)
{}

Block::~Block() {}

void Block::close() {
  disable();
}

int Block::baseIndent() const {
  return 0;
}

bool Block::appendFencedCodeText(const LineHandler& /* lineHandler */) {
  return false;
}

bool Block::appendHTMLBlockText(const LineHandler& /* lineHandler */) {
  return false;
}

void Block::appendLine(const LineHandler& /* lineHandler */) {
  return; // do nothing
}

bool Block::isFencedCodeBlock() const {
  return false;
}

bool Block::isParagraph() const {
  return false;
}

bool Block::toggleFencedCodeBlock(QChar fenceChar, int count, const LineHandler& /* rest */, int indent) {
  parent()->appendLeafBlock(new FencedCodeBlock(parent(), fenceChar, count, "", indent));
  return true;
}

bool Block::closeHTMLBlock(const LineHandler& /* lineHandler */) {
  return false;
}

HeadingBlock* Block::convertToSetextHeading(const LineHandler& /* lineHandler */) {
  return nullptr;
}

QString Block::fence() const {
  return "";
}

void Block::handleBlankLine(const LineHandler& /* lineHandler */) {
  // do nothing
}

ContainerBlock* Block::parent() {
  return parent_;
}

const ContainerBlock* Block::parent() const {
  return parent_;
}

void Block::disable() {
  writable_ = false;
}
  
bool Block::writable() const {
  return writable_;
}
