// md-parser/containerblock.cpp - an element for markdown parser
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


#include "containerblock.hpp"

#include <unordered_map>
#include "htmltag.hpp"
#include "leafblock.hpp"
#include "linehandler.hpp"
#include "inlineparser.hpp"
#include "parser.hpp"


/////////////////////
// Container Block //
/////////////////////

ContainerBlock::ContainerBlock(Parser* parser, ContainerBlock* parent, int indent)
  : Block(parent),
    parser_(parser),
    children_(),
    depth_(0),
    indent_(indent)
{
  if (parent) {
    depth_ = parent->depth();
  }
}

ContainerBlock::ContainerBlock(ContainerBlock* parent, int indent)
  : ContainerBlock(parent->parser(), parent, indent)
{
  if (parent) {
    depth_ = parent->depth();
  }
}

ContainerBlock::~ContainerBlock() {
  for (auto block : children_) {
    delete block;
  }
}

void ContainerBlock::setParser(Parser* parser) {
  parser_ = parser;
}

void ContainerBlock::close() {
  if (!isEmpty()) {
    last()->close();
  }

  disable();
}

void ContainerBlock::appendBlock(Block* block) {
  if (!isEmpty() && last()->writable()) {
    last()->close();
  }
  
  children_.append(block);
}

void ContainerBlock::appendLeafBlock(LeafBlock* block) {
  appendBlock(block);
}

void ContainerBlock::appendContainerBlock(ContainerBlock* block) {
  appendBlock(block);
  parser_->setCurrent(block);
}

const QList<Block*> ContainerBlock::children() const {
  return children_;
}

const Parser* ContainerBlock::parser() const {
  return parser_;
}

Parser* ContainerBlock::parser() {
  return parser_;
}

int ContainerBlock::depth() const {
  return depth_;
}

int ContainerBlock::indent() const {
  return indent_;
}

bool ContainerBlock::isEmpty() const {
  return children_.isEmpty();
}

Block* ContainerBlock::first() {
  return children_.isEmpty() ? nullptr : children_.first();
}

const Block* ContainerBlock::first() const {
  return children_.isEmpty() ? nullptr : children_.first();
}

Block* ContainerBlock::last() {
  return children_.isEmpty() ? nullptr : children_.last();
}

const Block* ContainerBlock::last() const {
  return children_.isEmpty() ? nullptr : children_.last();
}

void ContainerBlock::removeLast() {
  children_.removeLast();
}

bool ContainerBlock::isIndentEnoughForChild(int /*indent*/) const {
  return true;
}

bool ContainerBlock::hasBlankline() const {
  return true;
}

void ContainerBlock::setHasBlankline(bool /*hasBlankline*/) {
  return; // do nothing
}

bool ContainerBlock::dispatchBlankLine(const LineHandler& lineHandler) {
  if (!lineHandler.isBlank()) return false;

  if (!isEmpty()) {
    last()->handleBlankLine(lineHandler);
  }

  return true;
}

bool ContainerBlock::dispatchContainerBlock(LineHandler* lineHandler) {
  return dispatchBlockQuote(lineHandler) ||
    dispatchBulletList(lineHandler) ||
    dispatchOrderedList(lineHandler);
}

bool ContainerBlock::dispatchBlockQuote(LineHandler* lineHandler) {
  if (!lineHandler->matchBlockQuote()) return false;

  if (depth() < lineHandler->depth()) {
    appendBlockQuote(*lineHandler);
  }
    
  return true;
}

void ContainerBlock::appendBlockQuote(const LineHandler& lineHandler) {
  appendContainerBlock(new BlockQuoteBlock(this, lineHandler.indent()));
  parser()->current()->appendBlockQuote(lineHandler);
}

bool ContainerBlock::dispatchBulletList(LineHandler* lineHandler) {
  LineHandler copy{*lineHandler};
  int baseIndent{copy.indent()};
  QChar bullet{copy.findBullet()};

  if (bullet.isNull()) return false;

  if (depth() > copy.depth()) {
    ContainerBlock* current{this};
    
    do {
      parser()->unwind();
      current = parser()->current();
    } while (current->depth() > copy.depth());

    return current->dispatchBulletList(lineHandler);
  }

  if (copy.isBlank()) {
    if (!isEmpty() && last()->appendParagraphText(*lineHandler)) {
      *lineHandler = copy;

      return false;
    }
  } else {
    if (!isIndentEnoughForChild(baseIndent)) {
      return parser()->unwind() &&
	parser()->current()->dispatchBulletList(lineHandler);
    }
  }

  *lineHandler = copy;
  appendFirstBulletList(lineHandler, bullet, baseIndent);
  
  return true;
}

void ContainerBlock::appendBulletList(QChar /*bullet*/, int /*baseIndent*/, int /*indent*/, bool /*hasBlankline*/) {
  Q_ASSERT(false); // cannot invoke it.
}

bool ContainerBlock::dispatchOrderedList(LineHandler* lineHandler) {
  static const QString nr1OrderedListString{"1."};
  LineHandler copy{*lineHandler};
  int baseIndent{copy.indent()};
  QStringRef digit{copy.findDigit()};

  if (digit.isEmpty()) return false;

  if (depth() > copy.depth()) {
    ContainerBlock* current{this};
    
    do {
      parser()->unwind();
      current = parser()->current();
    } while (current->depth() > copy.depth());

    return current->dispatchOrderedList(lineHandler);
  }

  if (copy.isBlank()) {
    if (!isEmpty() && last()->appendParagraphText(*lineHandler)) {
      *lineHandler = copy;

      return false;
    }
  } else if (!isIndentEnoughForChild(baseIndent)) {
    return parser()->unwind() &&
      parser()->current()->dispatchOrderedList(lineHandler);
  }
  
  if (!isEmpty() && last()->isParagraph() &&
      digit != nr1OrderedListString) {
    return false;
  }
  
  qulonglong begin{digit.left(digit.length() - 1).toULongLong()};
  QChar separator{digit.at(digit.length() - 1)};
  int markerLength{digit.length() + 1};
  *lineHandler = copy;
  appendFirstOrderedList(lineHandler, begin, separator, baseIndent, markerLength);

  return true;
}

void ContainerBlock::appendOrderedList(QChar /*separator*/, int /*baseIndent*/, int /*indent*/, int /*markerLength*/, bool /*hasBlankline*/) {
  Q_ASSERT(false); // cannot invoke it.
}

void ContainerBlock::appendFirstBulletList(LineHandler* lineHandler, const QString& bullet, int baseIndent) {
  int indent{lineHandler->indent()};

  if (baseIndent + 1 == indent) {
    ++indent;
  }

  BulletListBlock* listBlock{new BulletListBlock(this, indent)};
  appendContainerBlock(listBlock);
  BulletListItem* listItem{new BulletListItem(listBlock, indent, bullet, baseIndent)};
  listBlock->appendContainerBlock(listItem);
}

void ContainerBlock::appendFirstOrderedList(LineHandler* lineHandler, qulonglong begin, QChar separator, int baseIndent, int markerLength) {
  OrderedListBlock* listBlock{new OrderedListBlock(this, lineHandler->indent(), begin)};
  appendContainerBlock(listBlock);
  OrderedListItem* listItem{new OrderedListItem(listBlock, listBlock->indent(), separator, baseIndent, markerLength)};
  listBlock->appendContainerBlock(listItem);
}

bool ContainerBlock::dispatchIndentedCode(const LineHandler& lineHandler) {
  int requirement{IndentedCodeBlock::INDENT_SIZE + indent()};
  LineHandler removed{lineHandler.removeIndent(requirement)};

  if (removed.indent() < requirement) return false;
  
  if (depth() > lineHandler.depth()) {
    ContainerBlock* current{this};
    
    do {
      parser()->unwind();
      current = parser()->current();
    } while (current->depth() > lineHandler.depth());

    current->appendIndentedText(&removed);
  } else if (isEmpty() || !last()->appendIndentedText(&removed)) {
    appendIndentedText(&removed);
  }
  
  return true;
}

bool ContainerBlock::appendIndentedText(LineHandler* lineHandler) {
  if (!writable()) return false;
  
  appendLeafBlock(new IndentedCodeBlock(this, *lineHandler));

  return true;
}

bool ContainerBlock::appendParagraph(const LineHandler& lineHandler) {
  if (!writable()) return false;
  
  if (!lineHandler.isBlank()) {
    appendLeafBlock(new ParagraphBlock(this, lineHandler));
  }

  return true;
}

bool ContainerBlock::appendParagraphText(const LineHandler& lineHandler) {
  return appendParagraph(lineHandler);
}

void ContainerBlock::dispatchHeadingAndParagraph(LineHandler* lineHandler) {
  dispatchNoText(*lineHandler) ||
    dispatchHeadingBlock(lineHandler) ||
    appendParagraph(*lineHandler);
}

bool ContainerBlock::dispatchHeadingBlock(LineHandler* lineHandler) {
  int count{lineHandler->findHeadingMarker()};
  int currentIndent = lineHandler->indent();

  if (count == 0) return false;
  
  lineHandler->removeLastSequence('#');
  parser()->unwindUntil(currentIndent);
  parser()->current()->appendLeafBlock(new HeadingBlock(parser()->current(), lineHandler->trimmed(), count));

  return true;
    
}

bool ContainerBlock::dispatchNoText(const LineHandler& lineHandler) {
  if (!lineHandler.isBlank()) return false;

  if (!isEmpty()) {
    last()->handleBlankLine(lineHandler);
  }

  return true;
}

bool ContainerBlock::dispatchLeafBlock(LineHandler* lineHandler) {
  return dispatchHTMLBlock(lineHandler) ||
    dispatchFencedCodeBlock(*lineHandler) ||
    dispatchSetextHeading(*lineHandler) ||
    dispatchThematicBreak(*lineHandler) ||
    appendFencedCodeText(*lineHandler) ||
    (!isEmpty() && last()->appendHTMLBlockText(*lineHandler));
}

bool ContainerBlock::dispatchHTMLBlock(LineHandler* lineHandler) {
  static const HTMLTag htmlTag{};
  
  if (!isEmpty() && (last()->closeHTMLBlock(*lineHandler) ||
		     last()->appendHTMLBlockText(*lineHandler))) {
    return true;
  }

  // Autolinks
  if (lineHandler->isAutolink()) return false;

  // const HTMLTag& htmlTag(parser()->htmlTag());
  
  // type 1, 2, 3, 4 and 5
  for (auto pair : htmlTag.type12345List()) {
    if (lineHandler->matchHTMLOpenTag(*pair.first)) {
      HTMLBlockWithCloseTag* block{new HTMLBlockWithCloseTag(this, *lineHandler, pair.second)};
      appendLeafBlock(block);

      if (lineHandler->matchHTMLCloseTag(*pair.second)) {
	block->disable();
      }
      
      return true;
    }
  }
    
  // for type 6
  if (lineHandler->matchHTMLTag(htmlTag.type6List())) {
    appendLeafBlock(new HTMLBlock(this, *lineHandler));

    return true;
  }

  // for type 7
  if ((isEmpty() || !last()->writable()) &&
      !lineHandler->matchHTMLCloseTag(htmlTag.type1CloseTag()) &&
      lineHandler->isHTMLTagType7()) {
    appendLeafBlock(new HTMLBlock(this, *lineHandler));
    
    return true;
  }

  return false;
}

bool ContainerBlock::dispatchFencedCodeBlock(const LineHandler& lineHandler) {
  LineHandler copy{lineHandler};
  copy.skipWhitespace();
  int indent{copy.indent()};
  
  if (indent < this->indent() + IndentedCodeBlock::INDENT_SIZE) {
    const char fenceCharList[] = {'`', '~'};
    int count;
    
    for (const char fenceChar : fenceCharList) {
      if ((count = copy.skipFenceChar(fenceChar)) >= 3) {
	if (copy.indexOf('`') >= 0) return false;
	  
	if (depth() > copy.depth()) {
	  ContainerBlock* current{this};
	    
	  do {
	    parser()->unwind();
	    current = parser()->current();
	  } while (current->depth() > copy.depth());
	    
	  return current->dispatchFencedCodeBlock(lineHandler);
	}

	if (isEmpty()) {
	  appendLeafBlock(new FencedCodeBlock(this, fenceChar, count, copy.firstWord().toString(), indent));

	  return true;
	}

	return last()->appendHTMLBlockText(lineHandler) || 
	  last()->toggleFencedCodeBlock(fenceChar, count, copy, indent);
      }
      
      if (count > 0) break;
    }
  }

  return false;
}

bool ContainerBlock::appendFencedCodeText(const LineHandler& lineHandler) {
  if (isEmpty() || depth() > lineHandler.depth()) return false;
  
  return last()->appendFencedCodeText(lineHandler);
}

bool ContainerBlock::dispatchThematicBreak(const LineHandler& lineHandler) {
  const char markdownList[] = { '-', '*', '_' };
  int indent{lineHandler.countIndent()};

  if (indent - this->indent() > 3) return false;
  
  QString text{lineHandler.noWhitespace()};
  int length{text.length()};
    
  if (length < 3) return false;
  
  for (char markdown : markdownList) {
    if (text.count(markdown) == length) {
      parser()->unwindUntil(lineHandler.position());
      parser()->current()->appendThematicBreak();
      return true;
    }
  }

  return false;
}

void ContainerBlock::appendThematicBreak() {
  appendLeafBlock(new ThematicBreak(this));
}

bool ContainerBlock::dispatchSetextHeading(const LineHandler& lineHandler) {
  if (!isEmpty()) {
    HeadingBlock* heading{last()->convertToSetextHeading(lineHandler)};
    
    if (heading) {
      removeLast();
      appendLeafBlock(heading);
      return true;
    }
  }

  return false;
}


////////////////
// Body Block //
////////////////

BodyBlock::BodyBlock()
  : ContainerBlock(nullptr, nullptr, 0) {
}

BodyBlock::~BodyBlock() {
}

QString BodyBlock::html() const {
  QStringList htmlText;

  for (auto block : children()) {
    htmlText.append(block->html());
  }

  return htmlText.join('\n');
}

bool BodyBlock::appendParagraph(const LineHandler& lineHandler) {
  if (isEmpty() || !last()->appendParagraphText(lineHandler)) {
    appendLeafBlock(new ParagraphBlock(this, lineHandler));
  }

  return true;
}


////////////////
// List Block //
////////////////

ListBlock::ListBlock(ContainerBlock* parent, int indent)
  : ContainerBlock(parent, indent),
    hasBlankline_(false)
{}

ListBlock::~ListBlock() {}

void ListBlock::appendFirstBulletList(LineHandler* lineHandler, const QString& bullet, int baseIndent) {
  if (parser()->unwind()) {
    parser()->current()->appendFirstBulletList(lineHandler, bullet, baseIndent);
  }
}

void ListBlock::appendFirstOrderedList(LineHandler* lineHandler, qulonglong begin, QChar separator, int baseIndent, int markerLength) {
  if (parser()->unwind()) {
    parser()->current()->appendFirstOrderedList(lineHandler, begin, separator, baseIndent, markerLength);
  }
}

bool ListBlock::dispatchBulletList(LineHandler* lineHandler) {
  return parser()->unwind() && parser()->current()->dispatchBulletList(lineHandler);
}

bool ListBlock::dispatchIndentedCode(const LineHandler& lineHandler) {
  return parser()->unwind() && parser()->current()->dispatchIndentedCode(lineHandler);
}

bool ListBlock::dispatchOrderedList(LineHandler* lineHandler) {
  return parser()->unwind() && parser()->current()->dispatchOrderedList(lineHandler);
}

int ListBlock::baseIndent() const {
  return isEmpty() ? 0 : first()->baseIndent();
}

bool ListBlock::hasBlankline() const {
  return hasBlankline_;
}

void ListBlock::setHasBlankline(bool hasBlankline) {
  hasBlankline_ |= hasBlankline;
}

///////////////
// List Item //
///////////////

ListItem::ListItem(ContainerBlock* parent, int indent, int baseIndent)
  : ContainerBlock(parent, indent),
    hasBlankline_(false),
    baseIndent_(baseIndent)
{}

ListItem::~ListItem() {}

int ListItem::baseIndent() const {
  return baseIndent_;
}

bool ListItem::hasBlankline() const {
  Q_ASSERT(parent());
  
  return parent()->hasBlankline();
}

void ListItem::setHasBlankline(bool hasBlankline) {
  hasBlankline_ |= hasBlankline;
}

void ListItem::appendContainerBlock(ContainerBlock* block) {
  parent()->setHasBlankline(hasBlankline_);
  ContainerBlock::appendContainerBlock(block);
}

void ListItem::appendLeafBlock(LeafBlock* block) {
  parent()->setHasBlankline(hasBlankline_);
  ContainerBlock::appendLeafBlock(block);
}

bool ListItem::dispatchSetextHeading(const LineHandler& lineHandler) {
  LineHandler copy{lineHandler};
  copy.skipWhitespace();
  int indent{copy.indent()};

  return indent >= this->indent() &&
    ContainerBlock::dispatchSetextHeading(lineHandler);
}

bool ListItem::dispatchBlankLine(const LineHandler& lineHandler) {
  if (!lineHandler.isBlank()) return false;

  if (isEmpty()) {
    setHasBlankline(true);
    close();
  } else {
    last()->handleBlankLine(lineHandler);
  }

  return true;
}

bool ListItem::dispatchIndentedCode(const LineHandler& lineHandler) {
  if (!writable()) return false;

  int indent{IndentedCodeBlock::INDENT_SIZE + this->indent()};
  LineHandler removed{lineHandler.removeIndent(indent)};

  if (depth() != lineHandler.depth()) return false;

  if (removed.indent() < indent) {
    if (!isEmpty() && !last()->writable()) {
      LineHandler copy{lineHandler};
      copy.skipWhitespace();
      int indent{copy.indent()};

      if (indent < this->indent()) {
	if (isFollowedBy(&copy, indent)) return false;

	QStringRef digit{copy.findDigit()};
	parser()->unwind();
	parser()->unwind();

	return parser()->current()->dispatchIndentedCode(lineHandler);
      }
    }
    return false;
  }
  
  if (depth() > lineHandler.depth()) {
    ContainerBlock* current{this};
    
    do {
      parser()->unwind();
      current = parser()->current();
    } while (current->depth() > lineHandler.depth());

    current->appendIndentedText(&removed);
  } else if (isEmpty() || !last()->appendIndentedText(&removed)) {
    appendIndentedText(&removed);
  }
  
  return true;
}

bool ListItem::appendParagraph(const LineHandler& lineHandler) {
  if (!writable()) {
    parser()->unwind();
    parser()->unwind();

    return parser()->current()->appendParagraph(lineHandler);
  }

  if (isEmpty()) {
    if (!lineHandler.isBlank()) {
      appendLeafBlock(new ParagraphBlock(this, lineHandler));
    }
  } else if (!last()->writable()) {
    setHasBlankline(true);
    appendLeafBlock(new ParagraphBlock(this, lineHandler));
  } else if (!last()->appendParagraphText(lineHandler)) {
    int indent{IndentedCodeBlock::INDENT_SIZE + this->indent()};
    LineHandler removed{lineHandler.removeIndent(indent)};

    if (removed.indent() < this->indent()) {
      parser()->unwind();
      parser()->unwind();
      
      parser()->current()->appendParagraph(lineHandler);
    } else {
      appendLeafBlock(new ParagraphBlock(this, lineHandler));
    }
  }

  return true;
}


//////////////////////
// Bullet List Item //
//////////////////////

BulletListItem::BulletListItem(ContainerBlock* parent, int indent, const QString& bullet, int baseIndent)
  : ListItem(parent, indent, baseIndent),
    bullet_(bullet)
{}

BulletListItem::~BulletListItem() {}

QString BulletListItem::bullet() const {
  return bullet_;
}

bool BulletListItem::isIndentEnoughForChild(int indent) const {
  return indent >= baseIndent() + 2;
}

bool BulletListItem::dispatchContainerBlock(LineHandler* lineHandler) {
  return dispatchBlockQuote(lineHandler) ||
    dispatchBulletList(lineHandler) ||
    dispatchOrderedList(lineHandler);
}

bool BulletListItem::dispatchBulletList(LineHandler* lineHandler) {
  const int bulletLength{1}; // '*' or '-'
  LineHandler copy{*lineHandler};
  int baseIndent{copy.indent()};
  QChar bullet{copy.findBullet()};
  
  if (bullet == this->bullet()) {
    if (depth() > copy.depth()) {
      ContainerBlock* current{this};
      
      do {
	parser()->unwind();
	current = parser()->current();
      } while (current->depth() > copy.depth());
      
      return current->dispatchBulletList(lineHandler);
    }

    if (baseIndent < parent()->baseIndent()) {
      bool writable{isEmpty() ? this->writable() : last()->writable()};
      parser()->unwind();
      parser()->unwind();
      parser()->current()->setHasBlankline(!writable);

      return parser()->current()->dispatchBulletList(lineHandler);
    }

    *lineHandler = copy;

    if (baseIndent <= this->baseIndent() + bulletLength) {
      parent()->appendBulletList(bullet, baseIndent, copy.indent(), hasBlankline_);
    } else {
      appendFirstBulletList(lineHandler, this->bullet(), baseIndent);
    }

    return true;
  } else {
    return ContainerBlock::dispatchBulletList(lineHandler);
  }
}

QString BulletListItem::html() const {
  QStringList itemList;

  for (auto block : children()) {
    itemList.append(block->html());
  }
  
  QString text{itemList.join('\n')};
  
  if (text.length() > 0) {
    return QString(text.at(0) == '<' ?
		   (text.at(text.length() -1) == '>' ?
		    "<li>\n%1\n</li>" : "<li>\n%1</li>") :
		   (text.at(text.length() -1) == '>' ?
		    "<li>%1\n</li>" : "<li>%1</li>")).arg(text);
  }

  return QString("<li></li>");
}

bool BulletListItem::isFollowedBy(LineHandler* lineHandler, int indent) const {
  QChar bullet{lineHandler->findBullet()};
	
  return !bullet.isNull() &&
    bullet == this->bullet() &&
    indent < this->baseIndent() + 1;
}


///////////////////////
// Bullet List Block //
///////////////////////

BulletListBlock::BulletListBlock(ContainerBlock* parent, int indent)
  : ListBlock(parent, indent)
{}

BulletListBlock::~BulletListBlock() {
}

void BulletListBlock::appendBulletList(QChar bullet, int baseIndent, int indent, bool hasBlankline) {
  BulletListItem* listItem{new BulletListItem(this, indent, bullet, baseIndent)};
  appendContainerBlock(listItem);
  setHasBlankline(hasBlankline);
}

QString BulletListBlock::html() const {
  QString text;

  for (auto block : children()) {
    text.append(block->html() + '\n');
  }
  
  return QString("<ul>\n%1</ul>").arg(text);
}


///////////////////////
// Ordered List Item //
///////////////////////

OrderedListItem::OrderedListItem(ContainerBlock* parent, int indent, QChar separator, int baseIndent, int markerLength)
  : ListItem(parent, indent, baseIndent),
    separator_(separator),
    markerLength_(markerLength)
{}

OrderedListItem::~OrderedListItem() {}

int OrderedListItem::markerLength() const {
  return markerLength_;
}

bool OrderedListItem::isIndentEnoughForChild(int indent) const {
  return indent >= baseIndent() + markerLength();
}

bool OrderedListItem::dispatchContainerBlock(LineHandler* lineHandler) {
  return dispatchBlockQuote(lineHandler) ||
    dispatchOrderedList(lineHandler) ||
    dispatchBulletList(lineHandler);
}

bool OrderedListItem::dispatchOrderedList(LineHandler* lineHandler) {
  const int delimiterLength{1}; // '.' or ')'
  LineHandler copy{*lineHandler};
  int baseIndent{copy.indent()};
  QStringRef digit{copy.findDigit()};
  
  if (digit.isEmpty()) return false;

  qulonglong begin{digit.left(digit.length() - 1).toULongLong()};
  QChar separator{digit.at(digit.length() - 1)};

  if (separator_ == separator) {
    if (depth() > copy.depth()) {
      ContainerBlock* current{this};
      
      do {
	parser()->unwind();
	current = parser()->current();
      } while (current->depth() > copy.depth());
      
      return current->dispatchOrderedList(lineHandler);
    }

    if (baseIndent < parent()->baseIndent()) {
      bool writable{isEmpty() ? this->writable() : last()->writable()};
      parser()->unwind();
      parser()->unwind();
      parser()->current()->setHasBlankline(!writable);

      return parser()->current()->dispatchOrderedList(lineHandler);
    }

    int markerLength{digit.length() + delimiterLength};
    *lineHandler = copy;

    if (baseIndent <= this->baseIndent() + this->markerLength()) {
      parent()->appendOrderedList(separator, baseIndent, copy.indent(), markerLength, hasBlankline_);
    } else {
      appendFirstOrderedList(lineHandler, begin, separator, baseIndent, markerLength);
    }

    return true;
  } else {
    return ContainerBlock::dispatchOrderedList(lineHandler);
  }
}

QString OrderedListItem::html() const {
  QStringList itemList;

  for (auto block : children()) {
    itemList.append(block->html());
  }
  
  QString text{itemList.join('\n')};
  
  if (text.length() > 0) {
    return QString(text.at(0) == '<' ?
		   (text.at(text.length() -1) == '>' ?
		    "<li>\n%1\n</li>" : "<li>\n%1</li>") :
		   (text.at(text.length() -1) == '>' ?
		    "<li>%1\n</li>" : "<li>%1</li>")).arg(text);
  }

  return QString("<li></li>");
}

bool OrderedListItem::isFollowedBy(LineHandler* lineHandler, int indent) const {
  QStringRef digit{lineHandler->findDigit()};
  
  if (digit.isEmpty()) return false;
  
  QChar separator{digit.at(digit.length() - 1)};
    
  return separator == this->separator_ &&
    indent < this->baseIndent() + digit.length();
}


////////////////////////
// Ordered List Block //
////////////////////////

OrderedListBlock::OrderedListBlock(ContainerBlock* parent, int indent, qulonglong begin)
  : ListBlock(parent, indent),
    begin_(begin)
{}

OrderedListBlock::~OrderedListBlock() {
}

void OrderedListBlock::appendOrderedList(QChar separator, int baseIndent, int indent, int markerLength, bool hasBlankline) {
  OrderedListItem* listItem{new OrderedListItem(this, indent, separator, baseIndent, markerLength)};
  appendContainerBlock(listItem);
  setHasBlankline(hasBlankline);
}

QString OrderedListBlock::html() const {
  QString text;

  for (auto block : children()) {
    text.append(block->html() + '\n');
  }
  
  if (begin_ == 1) {
    return QString("<ol>\n%1</ol>").arg(text);
  } else {
    return QString("<ol start=\"%2\">\n%1</ol>").arg(text, QString::number(begin_));
  }
}


/////////////////
// Block Quote //
/////////////////

BlockQuoteBlock::BlockQuoteBlock(ContainerBlock* parent, int indent)
  : ContainerBlock(parent, indent)
{
  ++depth_;
}

BlockQuoteBlock::~BlockQuoteBlock() {
}

QString BlockQuoteBlock::html() const {
  QString text;

  for (auto block : children()) {
    text.append(block->html() + '\n');
  }
  
  return QString("<blockquote>\n%1</blockquote>").arg(text);
}

bool BlockQuoteBlock::dispatchBlankLine(const LineHandler& lineHandler) {
  if (!lineHandler.isBlank()) return false;

  parser()->unwind();

  return true;
}

void BlockQuoteBlock::handleBlankLine(const LineHandler& /* lineHandler */) {
  close();
}

bool BlockQuoteBlock::dispatchContainerBlock(LineHandler* lineHandler) {
  return dispatchBlockQuote(lineHandler) ||
    dispatchBulletList(lineHandler) ||
    dispatchOrderedList(lineHandler);
}

bool BlockQuoteBlock::dispatchBlockQuote(LineHandler* lineHandler) {
  if (!writable() || !lineHandler->matchBlockQuote()) return false;

  if (depth() < lineHandler->depth()) {
    appendBlockQuote(*lineHandler);
  }
    
  return true;
}

bool BlockQuoteBlock::dispatchIndentedCode(const LineHandler& lineHandler) {
  if (!writable()) return false;

  int requirement{IndentedCodeBlock::INDENT_SIZE + indent()};
  LineHandler removed{lineHandler.removeIndent(requirement)};

  if (removed.indent() < requirement) {
    // for lazy continuation lines
    if (removed.indent() >= IndentedCodeBlock::INDENT_SIZE &&
	lineHandler.depth() == 0 &&
	!isEmpty()) {
      LineHandler copy{lineHandler};
      copy.skipWhitespace();

      return last()->appendParagraphText(copy);
    }

    return false;
  }

  if (depth() > lineHandler.depth()) {
    ContainerBlock* current{this};
    
    do {
      parser()->unwind();
      current = parser()->current();
    } while (current->depth() > lineHandler.depth());

    current->appendIndentedText(&removed);
  } else if (isEmpty() || !last()->appendIndentedText(&removed)) {
    appendIndentedText(&removed);
  }
  
  return true;
}

bool BlockQuoteBlock::appendFencedCodeText(const LineHandler& lineHandler) {
  if (isEmpty() || depth() > lineHandler.depth()) return false;
  
  return last()->appendFencedCodeText(lineHandler);
}

void BlockQuoteBlock::appendBlockQuote(const LineHandler& lineHandler) {
  if (depth() >= lineHandler.depth()) return;
  
  appendContainerBlock(new BlockQuoteBlock(this, lineHandler.indent()));
  parser()->current()->appendBlockQuote(lineHandler);
}

bool BlockQuoteBlock::appendParagraph(const LineHandler& lineHandler) {
  if (!writable()) return false;

  // lazy or not?
  if (isEmpty() || !last()->appendParagraphText(lineHandler)) {
    if (depth() > lineHandler.depth()) {
      ContainerBlock* current{this};
      
      do {
	parser()->unwind();
	current = parser()->current();
      } while (current->depth() > lineHandler.depth());
      
      current->appendLeafBlock(new ParagraphBlock(current, lineHandler));
    } else {
      appendLeafBlock(new ParagraphBlock(this, lineHandler));
    }
  }

  return true;
}

bool BlockQuoteBlock::dispatchSetextHeading(const LineHandler& /* lineHandler */) {
  return false;
}
