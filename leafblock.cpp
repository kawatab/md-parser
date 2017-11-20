// md-parser/leafblock.cpp - an element for markdown parser
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


#include "leafblock.hpp"

#include <unordered_map>
#include "linehandler.hpp"
#include "inlineparser.hpp"
#include "parser.hpp"
#include "texthandler.hpp"


////////////////
// Leaf Block //
////////////////

LeafBlock::LeafBlock(ContainerBlock* parent, const QString& text)
  : Block(parent),
    text_(text)
{}

LeafBlock::~LeafBlock() {
}

bool LeafBlock::appendIndentedText(LineHandler* /* lineHandler */) {
  return false;
}

bool LeafBlock::appendParagraphText(const LineHandler& /* lineHandler */) {
  return false;
}

void LeafBlock::appendLine(const LineHandler& lineHandler) {
  text_.append(lineHandler.putLinebreakAtBOL());
}

void LeafBlock::appendLine2(const LineHandler& lineHandler) {
  text_.append(lineHandler.putLinebreakAtEOL());
}


/////////////////////
// paragraph block //
/////////////////////

ParagraphBlock::ParagraphBlock(ContainerBlock* parent, const LineHandler& lineHandler)
  : LeafBlock(parent, lineHandler.currentText())
{}

ParagraphBlock::~ParagraphBlock() {
}

bool ParagraphBlock::isParagraph() const {
  return true;
}

bool ParagraphBlock::appendParagraphText(const LineHandler& lineHandler) {
  if (!writable()) return false;

  appendLine(lineHandler);
  return true;
}

bool ParagraphBlock::appendIndentedText(LineHandler* lineHandler) {
  if (!writable()) return false;

  // Handle indented text as plain text, if there is no blank line before it
  lineHandler->skipWhitespace();
  appendLine(*lineHandler);
  return true;
}

HeadingBlock* ParagraphBlock::convertToSetextHeading(const LineHandler& lineHandler) {
  if (writable()) {
    std::unordered_map<char, int> markdownList = {{'=', 1}, {'-', 2}};
    QString text{lineHandler.trimmed()};
    int length{text.length()};
    
    for (auto markdown : markdownList) {
      if (text.count(markdown.first) == length) {
	return new HeadingBlock(parent(), text_.trimmed(), markdown.second);
      }
    }
  }

  return nullptr;
}

void ParagraphBlock::handleBlankLine(const LineHandler& /* lineHandler */) {
  parent()->setHasBlankline(true);
  close();
}

void ParagraphBlock::close() {
  if (!writable()) return;
  
  TextHandler temp{text_};
  int pos{0};
  QString label{temp.findLinkLabel(&pos, ':')};

  if (!label.isEmpty()) {
    QString reference{temp.findLinkReference(&pos)};

    if (!reference.isEmpty()) {
      bool ok;
      QString title{temp.findLinkTitle(&pos, &ok)};

      if (ok) {
	parent()->parser()->defineLink(label, reference, title);
	text_ = temp.rest(pos);

	if (text_.isEmpty()) {
	  parent()->removeLast();
	} else {
	  close();
	}
      }
    }
  }
  
  disable();
}

QString ParagraphBlock::html() const {
  static const QString NO_TAG{"%1"};
  static const QString WITH_TAG{"<p>%1</p>"};
  
  return (parent()->hasBlankline() ? WITH_TAG : NO_TAG).arg(InlineParser(text_, parent()->parser()).textToHTML());
}

/////////////////////////
// indented code block //
/////////////////////////

const int IndentedCodeBlock::INDENT_SIZE = 4;

IndentedCodeBlock::IndentedCodeBlock(ContainerBlock* parent, const LineHandler& lineHandler)
  : LeafBlock(parent, lineHandler.currentText()),
    pending_()
{}

IndentedCodeBlock::~IndentedCodeBlock() {
}

bool IndentedCodeBlock::appendIndentedText(LineHandler* lineHandler) {
  if (!writable()) return false;

  text_.append(pending_);
  pending_.clear();
  appendLine(*lineHandler);

  return true;
}

void IndentedCodeBlock::handleBlankLine(const LineHandler& lineHandler) {
  int indent{INDENT_SIZE + parent()->indent()};
  LineHandler removed{lineHandler.removeIndent(indent)};
  pending_.append('\n');

  if (removed.indent() >= indent) {
    pending_.append(removed.currentText());
  }
};

QString IndentedCodeBlock::html() const {
  static const QString indentedCodeTemplate{"<pre><code>%1\n</code></pre>"};

  return indentedCodeTemplate.arg(InlineParser(text_, parent()->parser()).codeToHTML());
}


///////////////////////
// Fenced Code Block //
///////////////////////

FencedCodeBlock::FencedCodeBlock(ContainerBlock* parent, QChar fence, int count, const QString& rest, int indent)
  : LeafBlock(parent, ""),
    fence_(fence),
    count_(count),
    rest_(rest),
    indent_(indent)
{}

FencedCodeBlock::~FencedCodeBlock() {
}

void FencedCodeBlock::handleBlankLine(const LineHandler& lineHandler) {
  if (writable()) {
    appendLine2(lineHandler.removeIndent(indent_));
  } else {
    parent()->setHasBlankline(true);
  }
}

bool FencedCodeBlock::toggleFencedCodeBlock(QChar fenceChar, int count, const LineHandler& rest, int indent) {
  if (writable()) {
    if (count < count_ || fenceChar != fence_ || !rest.isBlank()) return false;

    disable();
  } else {
    parent()->appendLeafBlock(new FencedCodeBlock(parent(), fenceChar, count, rest.firstWord().toString(), indent));
  }
  
  return true;
}

bool FencedCodeBlock::appendFencedCodeText(const LineHandler& lineHandler) {
  if (!writable()) return false;
  
  appendLine2(lineHandler.removeIndent(indent_));
  
  return true;
}

bool FencedCodeBlock::appendIndentedText(LineHandler* lineHandler) {
  if (!writable()) {
    return false;
  }
  
  appendLine2(lineHandler->removeIndent(indent_));
  
  return true;
}

QString FencedCodeBlock::html() const {
  static const QString FencedCodeTemplate{"<pre><code>%1</code></pre>"};
  static const QString FencedCodeWithLanguageTemplate{"<pre><code class=\"language-%2\">%1</code></pre>"};
  const Parser* parser{parent()->parser()};
  QString temp{InlineParser(text_, parser).codeToHTML()};

  return rest_.isEmpty() ?
    FencedCodeTemplate.arg(temp) :
    FencedCodeWithLanguageTemplate.arg(temp, InlineParser(rest_, parser).textToHTML());
}


///////////////////
// Heading block //
///////////////////

HeadingBlock::HeadingBlock(ContainerBlock* parent, const QString& line, int level)
  : LeafBlock(parent, line),
    tag_(QString("<h%1>%2</h%1>").arg(level))
{}

HeadingBlock::HeadingBlock(ContainerBlock* parent, const LineHandler& lineHandler, int level)
  : LeafBlock(parent, lineHandler.trimmed()),
    tag_(QString("<h%1>%2</h%1>").arg(level))
{}

HeadingBlock::~HeadingBlock() {
}

void HeadingBlock::handleBlankLine(const LineHandler& /* lineHandler */) {
  disable();
}

QString HeadingBlock::html() const {
  return tag_.arg(InlineParser(text_, parent()->parser()).textToHTML());
}

////////////////////
// Thematic break //
////////////////////

ThematicBreak::ThematicBreak(ContainerBlock* parent)
  : LeafBlock(parent, "")
{
  disable();
}

ThematicBreak::~ThematicBreak() {
}

QString ThematicBreak::html() const {
  return "<hr />";
}


////////////////
// HTML Block //
////////////////

HTMLBlock::HTMLBlock(ContainerBlock* parent, const LineHandler& lineHandler)
  : LeafBlock(parent, lineHandler.currentText())
{}

HTMLBlock::~HTMLBlock() {
}

bool HTMLBlock::appendIndentedText(LineHandler* lineHandler) {
  return appendHTMLBlockText(*lineHandler);
}

bool HTMLBlock::appendHTMLBlockText(const LineHandler& lineHandler) {
  if (!writable() || lineHandler.indent() < parent()->indent()) {
    return false;
  }
  
  appendLine(lineHandler.removeIndent(parent()->indent()));

  return true;
}

HeadingBlock* HTMLBlock::convertToSetextHeading(const LineHandler& lineHandler) {
  if (writable()) {
    std::unordered_map<char, int> markdownList = {{'=', 1}, {'-', 2}};
    QString text{lineHandler.trimmed()};
    int length = text.length();
    
    for (auto markdown : markdownList) {
      if (text.count(markdown.first) == length) {
	return new HeadingBlock(parent(), text_.trimmed(), markdown.second);
      }
    }
  }

  return nullptr;
}

QString HTMLBlock::html() const {
  return text_;
}

bool HTMLBlock::closeHTMLBlock(const LineHandler& /* lineHandler */) {
  return false;
}

void HTMLBlock::handleBlankLine(const LineHandler& /* lineHandler */) {
  disable();
}


///////////////////////////////
// HTML Block with close tag //
///////////////////////////////

HTMLBlockWithCloseTag::HTMLBlockWithCloseTag(ContainerBlock* parent, const LineHandler& lineHandler, const QRegularExpression* closeTag)
  : HTMLBlock(parent, lineHandler),
    closeTag_(closeTag)
{}

HTMLBlockWithCloseTag::~HTMLBlockWithCloseTag() {
}

bool HTMLBlockWithCloseTag::closeHTMLBlock(const LineHandler& lineHandler) {
  // for type 1
  if (!lineHandler.matchHTMLCloseTag(*closeTag_)) return false;

  appendHTMLBlockText(lineHandler);
  disable();
    
  return true;
}

void HTMLBlockWithCloseTag::handleBlankLine(const LineHandler& lineHandler) {
  if (writable()) {
    appendLine(lineHandler);
  }
}
