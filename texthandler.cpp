// md-parser/texthandler.cpp - a text handler for markdown parser
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


#include "texthandler.hpp"

#include <QUrl>
#include "character.hpp"


TextHandler::TextHandler(const QString& text) : text_(&text) {
}

TextHandler::TextHandler(const QStringRef& text) : text_(text) {
}

bool TextHandler::isAutolink() const {
  int pos{0};
  
  if (text_.length() <= 7 || // minimum length = 7 ('<xx:xx>')
      text_.at(pos) != '<' ||
      text_.at(pos + 1).isSpace() ||
      text_.at(pos + 1) == '/') return false;
  
  while (++pos < text_.length()) {
    QChar chr{text_.at(pos)};
    
    if (chr == ':' || chr == '@') return true;
    if (!chr.isLetterOrNumber() && chr != '+' && chr != '-') break;
  }
    
  return false;
}

QString TextHandler::findLinkLabel(int* begin) const {
  int pos{skipWhitespace(*begin)};
  int length{text_.length()};
  QString label{};

  if (pos >= length) return QString{};

  if (text_.at(pos) == '[') {
    ++pos;

    while (text_.at(pos).isSpace()) {
      if (++pos >= length) return QString();
    }

    for (; pos < length; ++pos) {
      QChar chr{text_.at(pos)};

      if (chr == '[') {
	QString result{findLinkLabel(&pos)};

	if (result.isEmpty() && *begin < pos) {
	  *begin = pos;
	}

	return result;
      } else if (chr == ']') {
	*begin = pos + 1;
	
	return label;
      } else if (chr == '\\') {
	label.append(chr);
	++pos;
	label.append(text_.at(pos));
      } else if (chr == '`') {
	label.append(chr);

	for (;;) {
	  if (++pos >= length) return QString();

	  QChar chr{text_.at(pos)};
	  label.append(chr);

	  if (chr == '\\') {
	    ++pos;
	    label.append(text_.at(pos));
	  } else if (chr== '`') {
	    break;
	  }
	}
      } else if (chr.isSpace()) {
	label.append(' ');

	do {
	  if (++pos >= length) return QString();
	} while (text_.at(pos).isSpace());
	
	--pos;
      } else {
	label.append(chr);
      }
    }
  }

  return QString();
}

QString TextHandler::findLinkLabel(int* begin, QChar nextChar) const {
  int pos{*begin};
  QString label{findLinkLabel(&pos)};

  if (pos == *begin ||
      pos >= text_.length() ||
      text_.at(pos) != nextChar) return QString{};

  *begin = pos + 1;

  return label;
}

QString TextHandler::findLinkReference(int *begin) const {
  int pos{skipWhitespace(*begin)};

  int length{text_.length()};
  QString temp{};
  EscapeChar escape{};

  while (pos < length) {
    QChar chr{text_.at(pos)};
    
    if (chr.isSpace()) {
      if (temp.startsWith('<') && temp.endsWith('>')) {
	temp.remove(0, 1);
	temp.chop(1);
      }
      
      break;
    }
    
    escape = EscapeChar::get(text_.mid(pos));

    if (!escape.isEmpty()) {
      temp.append(escape.output());
      pos += escape.inputLength();
    } else {
      temp.append(chr);
      ++pos;
    }
  }

  QString reference{QUrl(temp).toEncoded()};
  *begin = pos;

  return reference;
}

QString TextHandler::findLinkTitle(int* begin, bool* ok) const {
  int pos{skipWhitespace(*begin, ok)};
  *begin = pos;

  int length{text_.length()};
  QString title{};
  EscapeChar escape{};

  if (length > pos + 1) {
    QChar chr1{text_.at(pos)};
    
    if (chr1 == '\'' || chr1 == '"') {
      ++pos;
    
      do {
	QChar chr2{text_.at(pos)};
      
	if (chr2 == chr1) {
	  while (++pos < length) {
	    QChar chr3{text_.at(pos)};

	    if (chr3 == '\n') {
	      ++pos;

	      break;
	    } else if (chr3 != ' ' && chr3 != '\t') {
	      return QString{};
	    }
	  }

	  *ok = true;
	  *begin = pos;
	
	  return title;
	}
      
	escape = EscapeChar::get(text_.mid(pos));

	if (!escape.isEmpty()) {
	  title.append(escape.output());
	  pos += escape.inputLength();
	} else {
	  title.append(chr2);
	  ++pos;
	}
      } while (pos < length);
    
      *ok = false;
    
      return QString{};
    }
  }

  *ok = true;

  return QString{};
}

QString TextHandler::rest(int begin) const {
  return text_.mid(begin).toString();
}

int TextHandler::skipWhitespace(int begin, bool* hasLinebreak) const {
  int pos{begin};
  int length{text_.length()};
  int numberOfLinebreak{0};

  while (pos < length) {
    QChar chr{text_.at(pos)};
    
    if (chr == '\t' || chr == ' ') {
      ++pos;
    } else if (chr == '\n') {
      ++pos;
      ++numberOfLinebreak;
    } else {
      break;
    }
  }

  if (hasLinebreak) {
    *hasLinebreak = numberOfLinebreak > 0;
  }

  return pos;
}

QString TextHandler::convertToPercentEncoding() const {
  static const QByteArray percentEncodingException{"@#%()&*/:+=?,"};
  const char* amp{"&"};
  const char* ampRef{"&amp;"};

  return text_.toUtf8().toPercentEncoding(percentEncodingException).replace(amp, ampRef);
}

QString TextHandler::convertEntityReferecence() const {
  // EscapeChar escape;
  EntityChar entity;
  QString temp{text_.mid(skipWhitespace(0)).toString()};
  
  for (int pos{0}; pos < text_.length(); ++pos) {
    if (!(entity = EntityChar::get(temp.at(pos))).isEmpty()) {
      temp.replace(pos, 1, entity.output());
      pos += entity.output().length() - 1;
    }
  }

  return temp;
}

int TextHandler::skipHTMLBlock(int begin) const {
  if (begin >= text_.length() || text_.at(begin) != '<') return begin;

  int pos{begin + 1};
  int temp;
  
  if (pos == (temp = skipOpenTag(pos)) &&
      pos == (temp = skipCloseTag(pos)) &&
      pos == (temp = skipHTMLComment(pos)) &&
      pos == (temp = skipProcessingInstruction(pos)) &&
      pos == (temp = skipDeclaration(pos)) &&
      pos == (temp = skipCDATASection(pos))) {
    return begin;
  }

  return temp;
}

int TextHandler::skipHTMLComment(int begin) const {
  int end{text_.length()};

  if (begin + 4 >= end ||
      text_.at(begin) != '!' ||
      text_.at(begin + 1) != '-' ||
      text_.at(begin + 2) != '-' || 
      text_.at(begin + 3) == '>' ||
      (text_.at(begin + 3) == '-' && text_.at(begin + 4) == '>')) return begin;

  int count{0};

  for (int pos{begin + 3}; pos < end; ++pos) {
    if (count == 2) {
      return text_.at(pos) == '>' ? pos + 1 : begin;
    } else {
      count = text_.at(pos) == '-' ? count + 1 : 0;
    }
  }

  return begin;
}

int TextHandler::skipProcessingInstruction(int begin) const {
  int end{text_.length()};

  if (begin + 1 >= end || text_.at(begin) != '?') return begin;

  for (int pos{begin + 1}; pos < end; ++pos) {
    if (text_.at(pos) == '?' &&
	pos + 1 < end &&
	text_.at(pos + 1) == '>') {
      return pos + 2;
    }
  }

  return begin;
}

int TextHandler::skipDeclaration(int begin) const {
  int end{text_.length()};

  if (begin + 1 >= end || text_.at(begin) != '!') return begin;

  int pos{begin + 1};

  if (!text_.at(pos).isUpper()) return begin;

  while (++pos < end) {
    if (!text_.at(pos).isUpper()) break;
  }

  if (!text_.at(pos).isSpace()) return begin;

  pos = skipWhitespace(++pos);

  for (; pos < end; ++pos) {
    if (text_.at(pos) == '>') return pos + 1;
  }

  return begin;
}

int TextHandler::skipCDATASection(int begin) const {
  int end{text_.length()};

  if (begin + 7 >= end ||
      text_.at(begin) != '!' ||
      text_.at(begin + 1) != '[' ||
      text_.at(begin + 2) != 'C' || 
      text_.at(begin + 3) != 'D' ||
      text_.at(begin + 4) != 'A' ||
      text_.at(begin + 5) != 'T' ||
      text_.at(begin + 6) != 'A' ||
      text_.at(begin + 7) != '[') {
    return begin;
  }

  for (int pos{begin + 8}; pos < end; ++pos) {
    if (text_.at(pos) == ']') {
      if (pos + 2 >= end) return begin;

      if (text_.at(pos + 1) == ']' &&
	  text_.at(pos + 2) == '>') {
	return pos + 3;
      }
    }
  }

  return begin;
}

int TextHandler::skipOpenTag(int begin) const {
  int pos{begin};
  int endOfTagName{skipTagName(pos)};

  if (pos == endOfTagName) return begin;
  
  pos = endOfTagName;
  int end{text_.length()};

  while (pos < end) {
    QChar chr{text_.at(pos)};
    
    if (chr == '<' || chr == '\\') break;

    int endOfAttribute{skipAttribute(pos)};

    if (endOfAttribute == pos) {
      if (chr == '/') {
	if (++pos >= end || text_.at(pos) != '>') break;
      
	return pos + 1;
      }
      
      if (chr == '>') {
	return pos + 1;
      }

      break;
    } else {
      pos = endOfAttribute;
    }
  }

  return begin;
}

int TextHandler::skipCloseTag(int begin) const {
  int pos{begin};
  int end{text_.length()};

  if (pos >= end || text_.at(pos) != '/') return begin;
  
  ++pos;
  int endOfTagName{skipTagName(pos)};

  return (pos != endOfTagName &&
	  endOfTagName < end &&
	  text_.at(endOfTagName) == '>') ?
    endOfTagName + 1: begin;
}

int TextHandler::skipTagName(int begin) const {
  int pos{begin};
  int end{text_.length()};
  
  if (pos >= end || !text_.at(pos).isLetter()) return begin;
  
  while (++pos < end) {
    QChar chr{text_.at(pos)};
    
    if (chr.isSpace()) return skipWhitespace(++pos);

    if (chr == '>' || chr == '/') return pos;

    if (!chr.isLetterOrNumber() && chr != '-') break;
  }

  return begin;
}

int TextHandler::skipAttribute(int begin) const {
  int end{text_.length()};
  int pos{begin};
  int temp{skipAttributeName(pos)};

  if (temp == pos || temp >= end) return begin;

  pos = temp;
  QChar chr{text_.at(pos)};

  if (chr == '/' || chr == '>') return pos;
  
  if (text_.at(pos) != '=') return pos;

  while (++pos < end) {
    if (!text_.at(pos).isSpace()) {
      if (pos != (temp = skipUnquotedAttributeValue(pos))) return temp;
      
      if (pos != (temp = skipSingleQuotedAttributeValue(pos))) return temp;
      
      return pos == (temp = skipDoubleQuotedAttributeValue(pos)) ? begin : temp;
    }
  }

  return begin;
}

int TextHandler::skipAttributeName(int begin) const {
  int end{text_.length()};
  
  if (begin >= end) return begin;

  int pos{begin};
  QChar chr{text_.at(pos)};

  if (!chr.isLetter() && chr != '_' && chr != ':') return begin;

  while (++pos < end) {
    chr = text_.at(pos);

    if (chr.isSpace()) return skipWhitespace(++pos);

    if (chr == '=' || chr == '>' || chr == '/') return pos;

    if (!chr.isLetterOrNumber() &&
	chr != '_' &&
	chr != '.' &&
	chr != ':' &&
	chr != '-') {
      break;
    }
  }

  return begin;
}

int TextHandler::skipUnquotedAttributeValue(int begin) const {
  int end{text_.length()};
  int pos{begin};

  for (; pos < end; ++pos) {
    QChar chr{text_.at(pos)};

    if (chr.isSpace()) return skipWhitespace(++pos);

    if (chr == '>' || chr == '/') return pos;

    if (!chr.isLetterOrNumber() &&
	(chr == '"' ||
	 chr == '\'' ||
	 chr == '=' ||
	 chr == '<' ||
	 chr == '.')) {
      break;
    }
  }

  return begin;
}

int TextHandler::skipQuotedAttributeValue(int begin, QChar delimiter) const {
  int end{text_.length()};
  
  if (begin >= end) return begin;

  int pos{begin};
  QChar chr{text_.at(pos)};

  if (chr != delimiter) return begin;

  while (++pos < end) {
    chr = text_.at(pos);

    if (chr == delimiter) {
      if (++pos >= end) return begin;
      
      chr = text_.at(pos);

      return chr.isSpace() ? skipWhitespace(++pos) :
	(chr == '/' || chr == '>') ? pos :
	begin;
    }
  }

  return begin;
}

int TextHandler::skipSingleQuotedAttributeValue(int begin) const {
  return skipQuotedAttributeValue(begin, '\'');
}

int TextHandler::skipDoubleQuotedAttributeValue(int begin) const {
  return skipQuotedAttributeValue(begin, '"');
}
 
