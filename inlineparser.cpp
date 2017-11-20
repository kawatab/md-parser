// md-parser/inlineparser.cpp - a parser for inline text
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


#include "inlineparser.hpp"

#include <QLinkedList>
#include <QRegularExpression>
#include <QUrl>
#include "character.hpp"
#include "parser.hpp"
#include "precedence.hpp"
#include "texthandler.hpp"


InlineParser::InlineParser(const QString& line, const Parser* parser)
  : line_(line),
    parser_(parser)
{}

QString InlineParser::textToHTML() {
  QLinkedList<Precedence> split{parse()};
  
  if (split.isEmpty()) return line_;

  QString temp{};
  int lastPos{0};
  QStack<const Precedence*> pending{};
  
  for (const Precedence& current : split) {
    if (current.isIncomplete()) continue;

    while (!pending.isEmpty()) {
      const Precedence* last{pending.last()};

      if (current.isAheadOf(last->end())) {
	break;
      } else {
	pending.pop();
	lastPos = last->htmlRightPart(&temp, lastPos);
      }
    }

    lastPos = current.htmlLeftPart(&temp, lastPos);
    pending.append(&current);
  }

  while (!pending.isEmpty()) {
    lastPos = pending.pop()->htmlRightPart(&temp, lastPos);
  }

  temp.append(line_.right(line_.length() - lastPos));

  return temp;
}

QString InlineParser::textToPlain() {
  QLinkedList<Precedence> split{parse(false)};
  
  if (split.isEmpty()) return line_;

  QString temp{};
  int lastPos{0};
  QStack<const Precedence*> pending{};
  
  for (const Precedence& current : split) {
    if (current.isIncomplete()) continue;

    while (!pending.isEmpty()) {
      const Precedence* last{pending.last()};

      if (current.isAheadOf(last->end())) {
	break;
      } else {
	pending.pop();
	lastPos = last->plainTextRightPart(&temp, lastPos);
      }
    }

    lastPos = current.plainTextLeftPart(&temp, lastPos);
    pending.append(&current);
  }

  while (!pending.isEmpty()) {
    lastPos = pending.pop()->plainTextRightPart(&temp, lastPos);
  }

  temp.append(line_.right(line_.length() - lastPos));

  return temp;
}

QString InlineParser::codeToHTML() {
  EntityChar entity;

  for (int pos{0}; pos < line_.length(); ++pos) {
    if (!(entity = EntityChar::get(line_.at(pos))).isEmpty()) {
      line_.replace(pos, 1, entity.output());
      pos += entity.output().length() - 1;
    }
  }

  return line_;
};

QLinkedList<Precedence> InlineParser::parse(bool isHTML) {
  QLinkedList<Precedence> split;
  QStack<Precedence*> pending;
  int pos{skipWhitespace(0)};
  int temp;
  
  while (pos < line_.length()) {
    if (pos != (temp = replaceSquareBrackets(pos)) ||
	pos != (temp = replaceLink(pos, isHTML)) ||
	pos != (temp = replaceImage(pos, isHTML)) ||
	pos != (temp = replaceCodeSpan(pos)) ||
	pos != (temp = replaceSpecialCharacter(pos)) ||
	pos != (temp = replaceWhitespace(pos)) ||
	pos != (temp = skipEmphasis(pos, &split, &pending))) {
      pos = temp;
    } else {
      ++pos;
    }
  }

  return split;
}

bool InlineParser::closePrecedence(int* pos, QStack<Precedence*>* pending) const {
  const int singleSize{1};
  QStack<Precedence*> stack;

  if (findSameDelimiter(*pos, pending, &stack)) {
    Precedence* inner{stack.last()};
    
    if (!pending->isEmpty()) {
      Precedence* outer{pending->last()};

      if (outer->isContinued(*inner)) {
	if (outer->isLeftFlankingDelimiterRun(*pos, singleSize) &&
	    outer->isRightFlankingDelimiterRun(*pos, singleSize)) {
	  if (outer->close(pos, inner)) {
	    pending->pop();
	  
	    return true;
	  }

	  return failToClosePrecedence(&stack, pending);
	}
      }
    
      if (outer->close(pos, inner)) {
	pending->pop();
      
	return true;
      }
    }

    if (inner->close(*pos)) return true;
  }

  return failToClosePrecedence(&stack, pending);
}

bool InlineParser::findSameDelimiter(int pos, QStack<Precedence*>* pending, QStack<Precedence*>* stack) const {
  Precedence* inner;
  QChar chr{line_.at(pos)};

  do {
    if (pending->isEmpty()) return false;
  
    inner = pending->pop();
    stack->push(inner);
  } while (!inner->isSameDelimiterAs(chr));

  return true;
}

bool InlineParser::failToClosePrecedence(QStack<Precedence*>* stack, QStack<Precedence*>* pending) const {
  while (!stack->isEmpty()) {
    pending->push(stack->pop());
  }

  return false;
}

int InlineParser::skipWhitespace(int pos) const {
  int length{line_.length()};

  for (; pos < length; ++pos) {
    if (!line_.at(pos).isSpace()) break;
  }

  return pos;
}

int InlineParser::replaceSquareBrackets(int begin) {
  int pos{begin};
  int temp;
  TextHandler text{line_};

  if (pos == (temp = replaceAutolink(pos)) &&
      pos == (temp = text.skipHTMLBlock(pos))) {
    return begin;
  }

  return (temp < line_.length() && line_.at(temp) == '<') ?
    replaceSquareBrackets(temp) : temp;
}

int InlineParser::replaceAutolink(int begin) {
  if (begin >= line_.length() || line_.at(begin) != '<') return begin;

  int pos{begin + 1};

  if (line_.at(pos).isSpace()) return begin;
  
  // check auto link
  for (; pos < line_.length(); ++pos) {
    QChar chr{line_.at(pos)};
    
    if (chr == ':') {
      return pos <= 2 ? begin : applyAutolink(begin, pos);
    }
    
    if (chr == '@') {
      return pos <= 2 ? begin : applyEmailAutolink(begin, pos);
    }
    
    if (chr == '<' ||
	chr == '>' ||
	(!chr.isLetterOrNumber() && chr != '-' && chr != '+')) {
      break;
    }
  }

  return begin;
}

int InlineParser::applyAutolink(int begin, int pos) {
  static const QString autolinkTemplate{"<a href=\"%1\">%2</a>"};

  while (++pos < line_.length()) {
    QChar chr{line_.at(pos)};
    
    if (chr == '<') return begin;
    
    if (chr == '>') {
      QString uri{autolinkTemplate.arg(TextHandler(line_.mid(begin + 1, pos - begin - 1)).convertToPercentEncoding(),
						TextHandler(line_.mid(begin + 1, pos - begin - 1)).convertEntityReferecence())};

      line_.replace(begin, pos - begin + 1, uri);

      return begin + uri.length() + 1;
    }

    if (chr.isSpace()) break;
  }

  return begin;
}

int InlineParser::applyEmailAutolink(int begin, int pos) {
  static const QString emailAutolinkTemplate{"<a href=\"mailto:%1\">%1</a>"};

  while (++pos < line_.length()) {
    QChar chr{line_.at(pos)};
    
    if (chr == '<') return begin;
    
    if (chr == '>') {
      QString address{emailAutolinkTemplate.arg(line_.mid(begin + 1, pos - begin - 1))};

      line_.replace(begin, pos - begin + 1, address);

      return begin + address.length() + 1;
    }

    if (chr.isSpace()) break;
  }

  return begin;
}

int InlineParser::replaceLink(int begin, bool isHTML) {
  if (line_.at(begin) != '[') return begin;

  int lineEnd{line_.length()};
  int count{1};
  QString linkLabel{};

  for (int pos{begin + 1}; pos < lineEnd; ++pos) {
    QChar chr{line_.at(pos)};

    if (chr == '\\') {
      linkLabel.append(chr);
      ++pos;
      linkLabel.append(line_.at(pos));
    } else if (chr == '[') {
      ++count;
      linkLabel.append(chr);
    } else if (chr == ']') {
      if (--count <= 0) {
	return applyLink(begin, pos, linkLabel, isHTML);
      } else if (pos + 1 >= lineEnd ||
		 line_.at(pos + 1) == '(' ||
		 line_.at(pos + 1) == '[') {
	return begin;
      }

      linkLabel.append(chr);
    } else if (chr == '!') {
      if (pos + 1 >= lineEnd) break;

      int lengthOfText{replaceImage(pos, isHTML)};
      linkLabel = line_.mid(begin + 1, lengthOfText - 1);

      if (lengthOfText > 0 && --count <= 0) {
	return applyLink(begin, begin + lengthOfText, linkLabel, isHTML);
      }
    } else if (chr == '<') {
      int tempPos{replaceSquareBrackets(pos)};

      if (tempPos != pos) return tempPos;

      linkLabel.append(chr);
    } else if (chr == '`') {
      linkLabel.append(chr);

      for (;;) {
	if (++pos >= lineEnd) return begin;

	QChar chr{line_.at(pos)};
	linkLabel.append(chr);

	if (chr == '\\') {
	  ++pos;
	  linkLabel.append(line_.at(pos));
	} else if (chr== '`') {
	  break;
	}
      }

      continue;
    } else {
      linkLabel.append(chr);
    }
  }
  
  return begin;
}

int InlineParser::replaceImage(int begin, bool isHTML) {
  static const QString imageReferenceString{"!["};

  if (line_.mid(begin, 2) != imageReferenceString) return begin;

  int lineEnd{line_.length()};
  int count{1};

  for (int pos{begin + 2}; pos < lineEnd; ++pos) {
    QChar chr{line_.at(pos)};

    if (chr == '\\') {
      ++pos; // skip one character
    } else if (chr == '[') {
      ++count;
    } else if (chr == ']') {
      if (--count <= 0) {
	return applyImage(begin, pos, isHTML);
      }
    } else if (chr == '<') {
      int tempPos{replaceSquareBrackets(pos)};

      if (tempPos != pos) return tempPos;
    } else if (chr == '`') {
      for (;;) {
	if (++pos >= lineEnd) return begin;

	QChar chr{line_.at(pos)};

	if (chr == '\\') {
	  ++pos;
	} else if (chr== '`') {
	  break;
	}
      }
    }
  }
  
  return begin;
}

int InlineParser::applyLink(int begin, int pos, const QString& linkLabel, bool isHTML) {
  const int lineEnd{line_.length()};

  if (pos + 1 < lineEnd && line_.at(pos + 1) == '[') {
    return applyFullReferenceLink(begin, pos + 1, linkLabel, isHTML);
  }

  int lengthOfText{applyInlineLink(begin, pos + 1, InlineParser(linkLabel, parser_).textToHTML(), isHTML)};

  return begin + (lengthOfText > 0 ?
		  lengthOfText :
		  applyShortcutReferenceLink(begin, pos, linkLabel, isHTML));
}

int InlineParser::applyImage(int begin, int pos, bool isHTML) {
  const int lineEnd{line_.length()};
  const int labelBegin{begin + 2};
  const int labelEnd{pos};
  const QString linkLabel{line_.mid(labelBegin, labelEnd - labelBegin).trimmed()};

  if (pos + 1 < lineEnd && line_.at(pos + 1) == '[') {
    return applyFullReferenceImage(begin, pos + 1, linkLabel, isHTML);
  }

  int lengthOfText{applyInlineImage(begin, pos + 1, InlineParser(linkLabel, parser_).textToPlain(), isHTML)};

  return begin + (lengthOfText > 0 ?
		  lengthOfText :
		  applyShortcutReferenceImage(begin, pos, linkLabel, isHTML));
}

int InlineParser::applyFullReferenceLink(int begin, int pos, const QString& linkText, bool isHTML) {
  int lineEnd{line_.length()};

  if (pos >= lineEnd || line_.at(pos) != '[') return begin;

  int count{1};
  int labelBegin{pos + 1};

  while (++pos < lineEnd) {
    QChar chr{line_.at(pos)};

    if (chr == '\\') {
      ++pos; // skip one character
    } else if (chr == '[') {
      ++count;
    } else if (chr == ']') {
      if (--count <= 0) {
	QString linkLabel{line_.mid(labelBegin, pos - labelBegin).trimmed()};
	if (linkLabel.isEmpty()) linkLabel = linkText;
	QString text{parser_->getLinkText(linkLabel, linkText)};
	++pos;

	if (text.isEmpty()) return begin;

	if (!isHTML) text = linkLabel;
	line_.replace(begin, pos - begin, text);
	
	return begin + text.length();
      }
    } else if (chr == '<') {
      int tempPos{replaceSquareBrackets(pos)};

      if (tempPos != pos) return tempPos;
    } else if (chr == '`') {
      for (;;) {
	if (++pos >= lineEnd) return begin;

	QChar chr{line_.at(pos)};

	if (chr == '\\') {
	  ++pos;
	} else if (chr== '`') {
	  break;
	}
      }
    }
  }
  
  return begin;
}

int InlineParser::applyFullReferenceImage(int begin, int pos, const QString& description, bool isHTML) {
  int lineEnd{line_.length()};

  if (pos >= lineEnd || line_.at(pos) != '[') return begin;

  int count{1};
  int labelBegin{pos + 1};

  while (++pos < lineEnd) {
    QChar chr{line_.at(pos)};

    if (chr == '\\') {
      ++pos; // skip one character
    } else if (chr == '[') {
      ++count;
    } else if (chr == ']') {
      if (--count <= 0) {
	QString linkLabel{line_.mid(labelBegin, pos - labelBegin).trimmed()};
	if (linkLabel.isEmpty()) linkLabel = description;
	QString text{parser_->getImageText(linkLabel, description)};
	++pos;

	if (text.isEmpty()) return begin;

	if (!isHTML) text = linkLabel;
	line_.replace(begin, pos - begin, text);

	return begin + text.length();
      }
    } else if (chr == '<') {
      int tempPos{replaceSquareBrackets(pos)};

      if (tempPos != pos) return tempPos;
    } else if (chr == '`') {
      for (;;) {
	if (++pos >= lineEnd) return begin;

	QChar chr{line_.at(pos)};

	if (chr == '\\') {
	  ++pos;
	} else if (chr== '`') {
	  break;
	}
      }
    }
  }
  
  return begin;
}

int InlineParser::applyShortcutReferenceLink(int begin, int pos, const QString& linkLabel, bool isHTML) {
  QString text{parser_->getLinkText(linkLabel)};

  if (text.isEmpty()) return 0;

  if (!isHTML) text = linkLabel;
  const int lineEnd{line_.length()};
  int end{pos};
	
  while (++end < lineEnd && line_.at(end) != '\n') {
    if (line_.at(end) != ' ' && line_.at(end) != '\t') {
      end = pos + 1;

      break;
    }
  }

  line_.replace(begin, end - begin, text);

  return text.length();
}

int InlineParser::applyShortcutReferenceImage(int begin, int pos, const QString& linkLabel, bool isHTML) {
  QString text{parser_->getImageText(linkLabel)};

  if (text.isEmpty()) return 0;

  if (!isHTML) text = linkLabel;
  const int lineEnd{line_.length()};
  int end{pos};
	
  while (++end < lineEnd && line_.at(end) != '\n') {
    if (line_.at(end) != ' ' && line_.at(end) != '\t') {
      end = pos + 1;

      break;
    }
  }

  line_.replace(begin, end - begin, text);

  return text.length();
}

int InlineParser::applyInlineLink(int begin, int pos, const QString& linkLabel, bool isHTML) {
  static const QString inlineLinkTemplate1{"<a href=\"%2\">%1</a>"};
  static const QString inlineLinkTemplate2{"<a href=\"%2\" title=\"%3\">%1</a>"};
  QString destination{};

  if ((pos = findLinkDestination(pos, &destination)) == 0) return 0;

  if (line_.at(pos) == ')') {
    QString html{isHTML ?
	inlineLinkTemplate1.arg(linkLabel, TextHandler(destination).convertToPercentEncoding()) :
	linkLabel};

    line_.replace(begin, pos - begin + 1, html);

    return html.length();
  }

  QString title{};
  
  if ((pos = findLinkTitle(pos, &title)) == 0) return 0;
  
  QString html{inlineLinkTemplate2.arg(linkLabel, QString(QUrl(destination).toEncoded()), title)};
  line_.replace(begin, pos - begin + 1, html);
    
  return html.length();
}

int InlineParser::applyInlineImage(int begin, int pos, const QString& linkLabel, bool isHTML) {
  static const QString inlineImageTemplate1{"<img src=\"%2\" alt=\"%1\" />"};
  static const QString inlineImageTemplate2{"<img src=\"%2\" alt=\"%1\" title=\"%3\" />"};
  QString destination{};

  if ((pos = findLinkDestination(pos, &destination)) == 0) return 0;

  if (line_.at(pos) == ')') {
    QString html{isHTML ?
	inlineImageTemplate1.arg(linkLabel, TextHandler(destination).convertToPercentEncoding()) :
	linkLabel};

    line_.replace(begin, pos - begin + 1, html);

    return html.length();
  }

  QString title{};
  
  if ((pos = findLinkTitle(pos, &title)) == 0) return 0;
  
  QString html{inlineImageTemplate2.arg(linkLabel, QString(QUrl(destination).toEncoded()), title)};
  line_.replace(begin, pos - begin + 1, html);
    
  return html.length();
}

int InlineParser::findLinkDestination(int pos, QString* destination) const {
  const int lineEnd{line_.length()};

  if (pos >= lineEnd || line_.at(pos) != '(') return 0;
  
  do {
    if (++pos >= lineEnd) return 0;
  } while (line_.at(pos) == ' ' || line_.at(pos) == '\n');
  
  EscapeChar escape;
  int count{1};
  QChar requiredEndChr{QChar::Null};

  if (line_.at(pos) == '<') {
    if (++pos >= lineEnd) return 0;

    requiredEndChr = '>';
  }
  
  do {
    QChar chr{line_.at(pos)};

    if (chr == ' ' || chr == '\n') {
      do {
	if (++pos >= line_.length()) return pos; // 0;

	if (line_.at(pos) == ')') {
	  return pos;
	}
      } while (line_.at(pos) == ' ' || line_.at(pos) == '\n');

      return pos;
    }

    if (chr == requiredEndChr) return ++pos;

    if (chr == ')') {
      if (--count <= 0) return pos;
    } else if (chr == '(') {
      ++count;
    } else if (!(escape = EscapeChar::get(line_.midRef(pos))).isEmpty()) {
      pos += escape.inputLength() - 1;
      destination->append(escape.output());

      continue;
    }
    
    destination->append(chr);
  } while (++pos < lineEnd);

  return 0;
}

int InlineParser::findLinkTitle(int pos, QString* title) const {
  static const QString quotEntityReference{"&quot;"};
  
  if (pos >= line_.length() - 1) return 0;

  QChar requiredEndChr{line_.at(pos)};

  if (requiredEndChr != '"' && requiredEndChr != '\'') {
    if (requiredEndChr != '(') return 0;

    requiredEndChr = ')';
  }
  
  EscapeChar escape;
  const int lineEnd{line_.length()};

  while (++pos < lineEnd) {
    QChar chr{line_.at(pos)};

    if (chr == '\'' || chr == '"') {
      if (chr == requiredEndChr) {
	while (++pos < lineEnd) {
	  if (line_.at(pos) == ')') return pos;

	  if (line_.at(pos) != ' ' && line_.at(pos) != '\n') return 0;
	}

	return 0;
      }

      title->append(quotEntityReference);
    } else if (chr == requiredEndChr) {
      ++pos;
      
      return (pos < line_.length() && line_.at(pos) == ')') ? pos : 0;
    } else if (!(escape = EscapeChar::get(line_.midRef(pos))).isEmpty()) {
      pos += escape.inputLength() - 1;
      title->append(escape.output());
    } else {
      title->append(chr);
    }
  }

  return 0;
}

int InlineParser::replaceCodeSpan(int begin) {
  static const QString codeTemplate{"<code>%1</code>"};
  static const QRegularExpression re{"[ \n]+"};

  if (line_.at(begin) != '`') return begin;

  int quoteBegin{begin + 1};
  int end{line_.length()};
  int count{1};

  if (quoteBegin >= end) return begin;
  
  while (line_.at(quoteBegin) == '`') {
    ++count;

    if (++quoteBegin >= end) return end;
  }
  
  for (int pos{quoteBegin}; pos < end; ++pos) {
    QChar chr{line_.at(pos)};

    if (chr == '`') {
      int size{pos - quoteBegin};
      int tempCnt{0};

      do {
	if (++tempCnt == count) {
	  if (++pos < end && line_.at(pos) == '`') break; // too many

	  InlineParser code{line_.mid(quoteBegin, size).trimmed(), parser_};
	  QString span{codeTemplate.arg(code.codeToHTML().replace(re," "))};
	  line_.replace(begin, size + 2 * count, span);

	  return begin + span.length();
	}

	if (++pos >= end) return end;
      } while (line_.at(pos) == '`');
    }
  }
  
  return begin;
}

int InlineParser::replaceSpecialCharacter(int begin) {
  int pos{begin};
  EscapeChar escape{EscapeChar::get(line_.midRef(pos))};

  if (!escape.isEmpty()) {
    line_.replace(pos, escape.inputLength(), escape.output());
    pos += escape.output().length();
  } else {
    EntityChar entity{EntityChar::get(line_.at(pos))};

    if (!entity.isEmpty()) {
      line_.replace(pos, 1, entity.output());
      pos += entity.output().length();
    }
  }

  return pos;
}

int InlineParser::replaceWhitespace(int begin) {
  static const QString brTagString{"<br />"};
  int pos{begin};
  
  if (line_.at(pos) == ' ') {
    int count{1};
    int end{line_.length()};

    if (pos + count < end) {
      for (;;) {
	QChar chr{line_.at(pos + count)};
      
	if (chr == ' ' || chr == '\t') {
	  if (pos + ++count < end) continue;
	
	  line_.chop(count);
	  ++pos;
	} else if (chr == '\n') {
	  if (count < 2) {
	    line_.remove(pos, count);
	    ++pos;
	  } else {
	    line_.replace(pos, count, brTagString);
	    pos += brTagString.length();
	  }
	} else {
	  pos += count;
	}
      
	pos = skipWhitespace(pos);
      
	break;
      }
    } else {
      line_.chop(count);
    }
  }

  return pos;
}

int InlineParser::skipEmphasis(int pos, QLinkedList<Precedence>* split, QStack<Precedence*>* pending) const {
  if (pos < line_.length()) {
    QChar chr{line_.at(pos)};
  
    if (chr == '*' || chr == '_') {
      Precedence first{&line_};
      Precedence second{&line_};

      if (((!pending->isEmpty() && pending->last()->isContinued(chr, pos)) ||
	   !closePrecedence(&pos, pending)) &&
	  pos + 1 < line_.length() &&
	  first.open(&pos, &second)) {
	if (second.isEmpty()) {
	  split->append(first);
	  pending->push(&split->last());
	} else {
	  split->append(first);
	  pending->push(&split->last());
	  split->append(second);
	  pending->push(&split->last());
	}
      }

      ++pos;
    }
  }

  return pos;
}
