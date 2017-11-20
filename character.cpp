// md-parser/character.hpp - character class for markdown parser
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


#include "character.hpp"

#include <algorithm>


///////////////////////
// Escape characters //
///////////////////////

EscapeChar::EscapeChar() : str_(), inputLength_(0) {}

EscapeChar::EscapeChar(const QString& str, int inputLength)
  : str_(str),
    inputLength_(inputLength)
{}

EscapeChar::EscapeChar(QChar chr, int inputLength)
  : str_(), inputLength_(inputLength)
{
  if (chr == '"') {
    str_.append("&quot;");
  } else if (chr == '<') {
    str_.append("&lt;");
  } else if (chr == '>') {
    str_.append("&gt;");
  } else {
    if (chr.unicodeVersion() == QChar::Unicode_Unassigned ||
	chr.isNull()) {
      chr = QChar(0xfffd);
    }

    str_.append(chr);
  }
}

bool EscapeChar::isEmpty() const {
  return this->inputLength_ == 0;
}

int EscapeChar::inputLength() const {
  return this->inputLength_;
}

QString EscapeChar::output() const {
  return this->str_;
}

EscapeChar EscapeChar::get(const QStringRef& text) {
  if (text.length() == 0) return EscapeChar();

  EscapeChar escape{getBackslashEscaped(text)};

  if (!escape.isEmpty()) return escape;

  if (text.at(0) == '&') {
    EscapeChar entity{getEntityWithCode(text)};

    if (entity.isEmpty()) {
      entity = getEntityWithText(text);
    }
    
    return entity;
  }
  
  return EscapeChar();
}


EscapeChar EscapeChar::getBackslashEscaped(const QStringRef& text) {
  static const class BackslashEscapedList {
  public:
    BackslashEscapedList() {
      EscapeChar backslashOnly{"\\", 1};
  
      for (EscapeChar& ptr : list_) {
	ptr = backslashOnly;
      }

      auto insert = [&](unsigned char chr, const char* str) {
	list_[chr] = EscapeChar(str, 2);
      };
  
      insert('\n', "<br />\n");
      insert('!', "!");
      insert('"', "&quot;");
      insert('#', "#");
      insert('$', "$");
      insert('%', "%");
      insert('&', "&amp;");
      insert('\'', "'");
      insert('(', "(");
      insert(')', ")");
      insert('*', "*");
      insert('+', "+");
      insert(',', ",");
      insert('-', "-");
      insert('.', ".");
      insert('/', "/");
      insert(':', ":");
      insert(';', ";");
      insert('<', "&lt;");
      insert('=', "=");
      insert('>', "&gt;");
      insert('?', "?");
      insert('@', "@");
      insert('[', "[");
      insert('\\', "\\");
      insert(']', "]");
      insert('^', "^");
      insert('_', "_");
      insert('`', "`");
      insert('{', "{");
      insert('|', "|");
      insert('}', "}");
      insert('~', "~");
    }

    EscapeChar operator[](QChar chr) const {
      return list_[static_cast<unsigned char>(chr.toLatin1())];
    }

    private:
    EscapeChar list_[0xff];
  } list;
  
  if (text.at(0) != '\\') return EscapeChar();
  
  if (text.length() == 1) return EscapeChar("\\", 1);

  return list[text.at(1)];
}

EscapeChar EscapeChar::getEntityWithCode(const QStringRef& text) {
  if (text.length() >= 4 && text.at(1) == '#') {

    QChar third{text.at(2)};
    
    if (third == 'x' || third == 'X') return getEntityWithHex(text);

    if (third.isDigit()) {
      int begin{3};
      int end{std::min(text.length(), 11)}; // '&#' + 1-8 digits + ';"
      QString decimal{third};
    
      for (int pos{begin}; pos < end; ++pos) {
	QChar chr{text.at(pos)};
      
	if (chr == ';') {
	  return EscapeChar(QChar(decimal.toInt()), decimal.length() + 3);
	} else if (chr.isDigit()) {
	  decimal.append(chr);
	} else {
	  break;
	}
      }
    }
  }

  return EscapeChar();
}

EscapeChar EscapeChar::getEntityWithHex(const QStringRef& text) {
  int begin{3};
  int end{std::min(text.length(), 12)}; // '&#x' + 1-8 digits + ';"
  QString hex{};
    
  for (int pos{begin}; pos < end; ++pos) {
    QChar chr{text.at(pos)};
      
    if (chr == ';') {
      bool ok;
      int number{hex.toInt(&ok, 16)};

      if (!ok) break;
	
      return EscapeChar(QChar(number), hex.length() + 4);
    } else if (chr.isLetterOrNumber()) {
      hex.append(chr);
    } else {
      break;
    }
  }

  return EscapeChar();
}

EscapeChar EscapeChar::getEntityWithText(const QStringRef& text) {
  int begin{1};
  int end{std::min(text.length(), 26)}; // max length of entity references (26)
  QString temp{};
    
  for (int pos{begin}; pos < end; ++pos) {
    QChar chr{text.at(pos)};
      
    if (chr == ';') {
      return temp == "nbsp" ? EscapeChar(QChar::Nbsp, 6) :
	temp == "amp" ? EscapeChar("&amp;", 5) :
	temp == "auml" ? EscapeChar("ä", 6) :
	temp == "ouml" ? EscapeChar("ö", 6) :
	temp == "copy" ? EscapeChar("©", 6) :
	temp == "AElig" ? EscapeChar("Æ", 7) :
	temp == "Dcaron" ? EscapeChar("Ď", 8) :
	temp == "frac34" ? EscapeChar("¾", 8) :
	temp == "HilbertSpace" ? EscapeChar("ℋ", 14) :
	temp == "DifferentialD" ? EscapeChar("ⅆ", 15) :
	temp == "ClockwiseContourIntegral" ? EscapeChar("∲", 26) :
	temp == "ngE" ? EscapeChar("≧̸", 5) :
	EscapeChar();
    } else if (chr.isSpace()) {
      break;
    } else if (chr.isLetterOrNumber()) {
      temp.append(chr);
    }
  }

  return EscapeChar();
}

//////////////////////
// Entity character //
//////////////////////

EntityChar::EntityChar() : str_() {}

EntityChar::EntityChar(const QString& str) : str_(str) {}

bool EntityChar::isEmpty() const {
  return str_.isEmpty();
}

QString EntityChar::output() const {
  return str_;
}

EntityChar EntityChar::get(QChar chr) {
  return EntityChar(chr == '\"' ? "&quot;" :
		    chr == '&'  ? "&amp;" :
		    chr == '<'  ? "&lt;" :
		    chr == '>'  ? "&gt;" :
		    "");
}
