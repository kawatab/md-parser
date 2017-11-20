// md-parser/linehandler.cpp - a text handler for markdown parser
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


#include "linehandler.hpp"

#include <QRegularExpression>
#include "texthandler.hpp"


const int LineHandler::TAB_SIZE = 4;

LineHandler::LineHandler(const QStringRef& line)
  : line_(line),
    physicalPosition_(0),
    logicalPosition_(0),
    offset_(0),
    depth_(0),
    indent_(0) {
}

LineHandler::LineHandler(const QStringRef& line, int physicalPosition, int logicalPosition, int offset, int depth, int indent)
  : line_(line),
    physicalPosition_(physicalPosition),
    logicalPosition_(logicalPosition),
    offset_(offset),
    depth_(depth),
    indent_(indent) {
}

int LineHandler::indexOf(QChar chr) const {
  return line_.indexOf(chr, physicalPosition_);
}

QString LineHandler::currentText() const {
  if (offset_ == 0) {
    return currentTextRef().toString();
  }

  QString temp{offset_, QChar(' ')};
  currentTextRef().appendTo(&temp);

  return temp;

}

QStringRef LineHandler::currentTextRef() const {
  return physicalPosition_ < line_.length() ?
    line_.mid(physicalPosition_) : QStringRef();
}

QString LineHandler::putLinebreakAtBOL() const {
  QString temp{"\n    "};
  temp.truncate(offset_ + 1);
  currentTextRef().appendTo(&temp);

  return temp;
}

QString LineHandler::putLinebreakAtEOL() const {
  if (offset_ == 0) {
    return currentTextRef().toString() + QChar('\n');
  }
  
  QString temp{offset_, QChar(' ')};
  currentTextRef().appendTo(&temp);
  temp += QChar('\n');

  return temp;
}

int LineHandler::position() const {
  return logicalPosition_;
}

int LineHandler::depth() const {
  return depth_;
}

int LineHandler::indent() const {
  return indent_;
}

int LineHandler::countIndent() const {
  int count{0};
  int length{line_.length()};

  for (int pos{0}; pos < length; ++pos) {
    QChar chr{line_.at(pos)};

    if (chr == '\t') {
      count += TAB_SIZE - pos % TAB_SIZE;
    } else if (chr == ' ') {
      ++count;
    } else {
      break;
    }
  }

  return count;
}

LineHandler LineHandler::removeIndent(int indent) const {
  if (indent == indent_) {
    return *this;
  }
  
  if (indent < indent_) {
    return LineHandler(line_).removeIndent(indent);
  }

  int pos{physicalPosition_};
  int length{line_.length()};
  int target{logicalPosition_ + indent - indent_};
  int logical{logicalPosition_ + offset_};
  int offset{0};
  
  for (; pos < length; ++pos) {
    if (logical >= target) {
      offset = logical - target;
      logical = target;
      break;
    }
    
    QChar chr{line_.at(pos)};
    
    if (chr == '\t') {
      logical += TAB_SIZE - logical % TAB_SIZE;
    } else if (chr == ' ') {
      ++logical;
    } else {
      break;
    }
  }

  int diff{logical - logicalPosition_};
  
  return LineHandler(line_, pos, logical, offset, depth_, diff + indent_);
}

void LineHandler::removeLastSequence(QChar chr) {
  int pos{line_.length()};

  // skip whitespaces
  while (--pos > physicalPosition_) {
    QChar temp{line_.at(pos)};
    
    if (temp != ' ' && temp != '\t') break;
  }

  // if the next character is another
  if (pos <= physicalPosition_ || line_.at(pos) != chr) return;

  while (--pos > physicalPosition_) {
    if (line_.at(pos) != chr) break;
  }
  
  if (pos == physicalPosition_) {
    line_.truncate(pos);
  } else {
    // if the next character isn't whitespace
    if (line_.at(pos) != ' ' && line_.at(pos) != '\t') return;

    // skip whitespaces
    while (--pos > physicalPosition_) {
      QChar temp{line_.at(pos)};
    
      if (temp != ' ' && temp != '\t') break;
    }

    line_.truncate(pos + 1);
  }
}

void LineHandler::skipWhitespace() {
  int physical{physicalPosition_};
  int logical{logicalPosition_};
  int length{line_.length()};
  int indent{indent_};

  for (; physical < length; ++physical) {
    QChar chr{line_.at(physical)};
    if (chr == '\t') {
      int diff = TAB_SIZE - logical % TAB_SIZE;
      logical += diff;
      indent += diff;
    } else if (chr == ' ') {
      ++logical;
      ++indent;
    } else {
      break;
    }
  }

  physicalPosition_ = physical;
  logicalPosition_ = logical;
  indent_ = indent;
}

QString LineHandler::trimmed() const {
  return line_.mid(physicalPosition_).trimmed().toString();
}

QRegularExpressionMatch LineHandler::matchHTMLTag() const {
  static const QRegularExpression re("\\A\\s*(<[a-zA-Z][a-zA-Z0-9]*\\s*/?>|<[a-zA-Z][a-zA-Z0-9]*\\s[^<>]*/?>|</[a-zA-Z][a-zA-Z0-9]*>)(.*)\\z", QRegularExpression::CaseInsensitiveOption);
  
  return re.match(line_, physicalPosition_);
}

bool LineHandler::matchHTMLOpenTag(const QRegularExpression& tag) const {
  return tag.match(line_, physicalPosition_).hasMatch();
}

bool LineHandler::matchHTMLCloseTag(const QRegularExpression& tag) const {
  return matchHTMLOpenTag(tag);
}

bool LineHandler::matchHTMLTag(const QStringList& tagList) const {
  int pos{physicalPosition_};
  int length{line_.length()};

  while (pos < length - 3) {
    if (line_.at(pos).isSpace()) {
      ++pos;
    } else {
      if (line_.at(pos++) != '<') return false;

      if (line_.at(pos) == '/') ++pos;
      
      QStringRef temp{line_.mid(pos)};
      length -= pos;
      
      for (auto tagName : tagList) {
	pos = tagName.length();
	int result = tagName.compare(temp.left(pos));

	if (result > 0) {
	  continue;
	} else if (result < 0) {
	  return false;
	} else if (pos >= length) {
	  return true;
	} else {
	  QChar chr{temp.at(pos)};
	  
	  return chr.isSpace() || chr == '>' ||
	    (chr == '/' && ++pos < length && temp.at(pos) == '>');
	}
      }
      
      return false;
    }
  }

  return false;
}

int LineHandler::findHeadingMarker() {
  int count{0};
  int pos{physicalPosition_};
  int logical{logicalPosition_ + offset_};
  int offset{0};
  int length{line_.length()};

  while (pos < length) {
    QChar chr{line_.at(pos)};

    if (chr == '#') {
      if (count >= 6) return false;
      
      ++count;
      ++pos;
      ++logical;
    } else if (chr == ' ') {
      pos = pos + 1;
      logical = logical + 1;

      break;
    } else if (chr == '\t') {
      pos = pos + 1;
      offset = TAB_SIZE - logical % TAB_SIZE - 1;
      logical = logical + 1;

      break;
    } else {
      return 0;
    }
  }

  if (count > 0) {
    physicalPosition_ = pos;
    logicalPosition_ = logical;
    offset_ = offset;
  }
  
  return count;
}

bool LineHandler::matchBlockQuote() {
  if (physicalPosition_ >= line_.length() ||
      line_.at(physicalPosition_) != QChar('>')) return false;

  int length{line_.length()};

  // skip first '>'
  ++depth_;
  indent_ = 0;
  ++physicalPosition_;
  logicalPosition_ += offset_ + 1;
  offset_ = 0;

  // keep last position
  int lastPhysical{physicalPosition_};
  int lastLogical{logicalPosition_};
  int lastIndent{indent_};
    
  while (physicalPosition_ < length) {
    QChar chr{line_.at(physicalPosition_)};

    if (chr == '\t') {
      int tabSize{TAB_SIZE - logicalPosition_ % TAB_SIZE};
      indent_ += tabSize;

      if (indent_ > 3) {
	if (line_.at(lastPhysical) == '\t') {
	  offset_ = TAB_SIZE - lastLogical % TAB_SIZE - 1;
	}

	logicalPosition_ = lastLogical + 1;
	physicalPosition_ = lastPhysical + 1;
	indent_ = lastIndent;

	break;
      }
	
      ++physicalPosition_;
      logicalPosition_ += tabSize;
    } else if (chr == ' ') {
      ++indent_;

      if (indent_ > 3) {
	if (line_.at(lastPhysical) == '\t') {
	  offset_ = TAB_SIZE - lastLogical % TAB_SIZE - 1;
	}

	logicalPosition_ = lastLogical + 1;
	physicalPosition_ = lastPhysical + 1;
	indent_ = lastIndent;

	break;
      }
	
      ++physicalPosition_;
      ++logicalPosition_;
    } else if (chr == '>') {
      ++depth_;
      indent_ = -1;
      ++physicalPosition_;
      ++logicalPosition_;
      lastPhysical = physicalPosition_;
      lastLogical = logicalPosition_;
      lastIndent = indent_;
    } else {
      break;
    }
  }

  if (indent_ < 0) {
    indent_ = 0;
  }
  
  return true;
}

int LineHandler::skipFenceChar(QChar fenceChr) {
  int physical{physicalPosition_};
  int logical{logicalPosition_};
  int length{line_.length()};
  int count{0};
  
  for (; physical < length; ++physical, ++logical, ++count) {
    if (line_.at(physical) != fenceChr) break;
  }
  
  physicalPosition_ = physical;
  logicalPosition_ = logical;

  return count;
}
  
QStringRef LineHandler::firstWord() const {
  int begin{physicalPosition_};
  int length{line_.length()};

  // skip whitespaces
  for (; begin < length; ++begin) {
    QChar chr{line_.at(begin)};
    
    if (chr != ' ' && chr != '\t') break;
  }

  if (begin >= length) {
    return line_.mid(0, 0);
  }
  
  int end{begin + 1};

  // find next whitespace or line end
  for (; end < length; ++end) {
    QChar chr{line_.at(end)};
    
    if (chr == ' ' || chr == '\t') break;
  }

  return line_.mid(begin, end - begin);
}
  
QChar LineHandler::findBullet() {
  static const QChar list[]{ '-', '+', '*' };

  for (auto bullet : list) {
    if (findBullet(bullet)) return bullet;
  }

  return '\0';
}

bool LineHandler::findBullet(QChar bullet) {
  int pos1{physicalPosition_};
  int logical1{logicalPosition_};
  const int length{line_.length()};

  // skip whitespaces
  for (; pos1 < length; ++pos1) {
    QChar chr{line_.at(pos1)};
    
    if (chr == ' ') {
      ++logical1;
    } else if (chr == '\t') {
      logical1 += TAB_SIZE - pos1 % TAB_SIZE;
    } else {
      break;
    }
  }

  // if the first isn't the bullet
  if (pos1 >= length || line_.at(pos1) != bullet) {
    return false;
  }

  // if the bullet is at end of line
  if (pos1 == length - 1) {
    physicalPosition_ = length;
    indent_ += logical1 - logicalPosition_ + 2;
    logicalPosition_ = logical1 + 1;

    return true;
  }
    
  return skipWhitespaceFollowedListMarker(pos1 + 1, logical1 + 1);
}

QStringRef LineHandler::findDigit() {
  int begin{physicalPosition_};
  int logical{logicalPosition_};
  const int length{line_.length()};

  // skip whitespaces
  for (; begin < length; ++begin) {
    QChar chr{line_.at(begin)};
    
    if (chr == ' ') {
      ++logical;
    } else if (chr == '\t') {
      logical += TAB_SIZE - begin % TAB_SIZE;
    } else {
      break;
    }
  }

  // check first digit
  if (begin + 1 >= length || !line_.at(begin).isDigit()) {
    return line_.mid(0, 0);
  }

  int count{1};
  int end{begin + 1};
  ++logical;
  
  // check the rest of sequence
  for (;end < length; ++end, ++logical) {
    if (line_.at(end).isDigit()) {
      if (++count > 9) break;
    } else {
      QChar last{line_.at(end)};
      
      return ((last == '.' || last == ')') &&
	      skipWhitespaceFollowedListMarker(end + 1, logical + 1)) ?
	line_.mid(begin, end - begin + 1) :
	line_.mid(0, 0);
    }
  }
  
  return line_.mid(0, 0);
}

bool LineHandler::skipWhitespaceFollowedListMarker(int beginPos, int beginLogical) {
  // skip whitespace
  int count{0};
  int pos{beginPos};
  int logical{beginLogical};
  int length{line_.length()};
  
  for (; pos < length; ++pos) {
    QChar chr{line_.at(pos)};
    
    if (chr == ' ') {
      ++logical;
      ++count;
      
      if (count > 4) {
	physicalPosition_ = beginPos + 1;
	indent_ += beginLogical - logicalPosition_ + 1;
	logicalPosition_ = beginLogical + 1;
	
	return true;
      }
    } else if (chr == '\t') {
      int diff{TAB_SIZE - logical % TAB_SIZE};
      logical += diff;
      count += diff;
      
      if (count > 4) {
	logical = beginLogical;

	if (line_.at(logical) == '\t') {
	  offset_ = TAB_SIZE - logical % TAB_SIZE - 1;
	} else {
	  offset_ = 0;
	}
	
	physicalPosition_ = beginPos + 1;
	indent_ += beginLogical - logicalPosition_ + 1;
	logicalPosition_ = beginLogical + 1;
	
	return true;
      }
    } else if (count == 0) {
      return false;
    } else {
      break;
    }
  }

  physicalPosition_ = pos;
  indent_ += logical - logicalPosition_;
  logicalPosition_ = logical;
  
  return true;
}

bool LineHandler::isAutolink() const {
  const TextHandler line{line_};

  return line.isAutolink();
}

bool LineHandler::isHTMLTagType7() const {
  const TextHandler handler{line_};

  int begin{handler.skipWhitespace(0)};
  
  if (line_.at(begin) == '<') {
    ++begin;
    int pos{begin};

    if (begin != (pos = handler.skipOpenTag(begin)) ||
	begin != (pos = handler.skipCloseTag(begin))) {
      pos = handler.skipWhitespace(pos);

      return pos >= line_.length();
    }
  }

  return false;
}

bool LineHandler::isBlank() const {
  int length{line_.length()};
  
  for (int i{physicalPosition_}; i < length; ++i) {
    QChar chr{line_.at(i)};
    
    if (chr != ' ' && chr != '\t') return false;
  }

  return true;
}

QString LineHandler::noWhitespace() const {
  QString temp;
  int length{line_.length()};
  
  for (int i{physicalPosition_}; i < length; ++i) {
    QChar chr{line_.at(i)};
    
    if (chr != ' ' && chr != '\t') {
      temp += chr;
    }
  }

  return temp;
}
