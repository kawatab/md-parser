// md-parser/precedence.cpp - an element for markdown parser
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


#include "precedence.hpp"

#include <QRegularExpression>


Precedence::Precedence(const QString* text)
  : delimiter_(),
    count_(),
    tag_(),
    text_(text),
    begin_(-1),
    end_(-1) {}

Precedence::Precedence(QChar delimiter, int count, const QString* tag, const QString* text, int begin)
  : delimiter_(delimiter),
    count_(count),
    tag_(tag),
    text_(text),
    begin_(begin),
    end_(-1)
{}

void Precedence::reset() {
  delimiter_ = QChar();
  count_= 0;
  tag_ = nullptr;
  begin_ = -1;
  end_ = -1;
}

void Precedence::init(QChar delimiter, const QString* tag, int begin) {
  delimiter_ = delimiter;
  count_= 1;
  tag_ = tag;
  begin_ = begin;
  end_ = -1;
}

int Precedence::begin() const {
  return begin_;
}

int Precedence::end() const {
  return end_;
}

bool Precedence::isEmpty() const {
  return begin_ < 0;
}

bool Precedence::isIncomplete() const {
  return end_ < 0;
}
 
bool Precedence::isAheadOf(int pos) const {
  return pos > begin_;
}

bool Precedence::isSameDelimiterAs(QChar delimiter) const {
  return this->delimiter_ == delimiter;
}

bool Precedence::isContinued(QChar delimiter, int pos) const {
  return this->delimiter_ == delimiter &&
    this->begin_ + 1 == pos;
}

bool Precedence::isContinued(const Precedence& other) const {
  return this->delimiter_ == other.delimiter_ &&
    this->begin_ + 1 == other.begin_;
}

int Precedence::htmlLeftPart(QString* applied, int lastPos) const {
  applied->append(text_->mid(lastPos, begin_ - lastPos));
  applied->append('<');
  applied->append(*tag_);
  applied->append('>');

  return begin_ + count_;
}

int Precedence::htmlRightPart(QString* applied, int lastPos) const {
  applied->append(text_->mid(lastPos, end_ -  count_ - lastPos));
  applied->append("</");
  applied->append(*tag_);
  applied->append('>');

  return end_;
}

int Precedence::plainTextLeftPart(QString* applied, int lastPos) const {
  applied->append(text_->mid(lastPos, begin_ - lastPos));
  // applied->append('<');
  // applied->append(*tag_);
  // applied->append('>');

  return begin_ + count_;
}

int Precedence::plainTextRightPart(QString* applied, int lastPos) const {
  applied->append(text_->mid(lastPos, end_ -  count_ - lastPos));
  // applied->append("</");
  // applied->append(*tag_);
  // applied->append('>');

  return end_;
}

bool Precedence::close(int pos) {
  const int singleSize{1};
  
  if (text_->at(pos) == '*') {
    if (pos + 1 < text_->length() && text_->at(pos + 1) == '*') {
      for (int i{pos + 2}; i < text_->length(); ++i) {
	if (text_->at(i) != '*') return false;
      }
    }
    
    if (delimiter_ == text_->at(pos) &&
	isRightFlankingDelimiterRun(pos, singleSize)) {
      end_ = pos + count_;
      
      return true;
    }
  } else if (text_->at(pos) == '_') {
    if (pos + 1 < text_->length() && text_->at(pos + 1) == '_') {
      for (int i{pos + 2}; i < text_->length(); ++i) {
	if (text_->at(i) != '_') return false;
      }
    }
    
    if (delimiter_ == text_->at(pos) &&
	isRightFlankingDelimiterRun(pos, singleSize) &&
	(!isLeftFlankingDelimiterRun(pos, singleSize) ||
	 (pos + singleSize < text_->length() && text_->at(pos + singleSize).isPunct()))) {
      end_ = pos + count_;
      
      return true;
    }
  }

  return false;
}

bool Precedence::close(int* pos, Precedence* inner) {
  static const QString tag{"strong"};
  const int singleSize{1};
  const int doubleSize{2};
  
  if (text_->at(*pos) == '*') {
    if (*pos + 1 < text_->length() &&
	text_->at(*pos + 1) == '*' &&
	this->delimiter_ == '*' && inner->delimiter_ == '*' &&
	this->begin_ + 1 == inner->begin_ &&
	isRightFlankingDelimiterRun(*pos, doubleSize)) {
      inner->end_ = -1;
      ++this->count_;
      this->end_ = *pos + 2;
      this->tag_ = &tag;
      ++(*pos);
      
      return true;
    }
  } else if (text_->at(*pos) == '_') {
    if (*pos + 1 < text_->length() &&
	text_->at(*pos + 1) == '_' &&
	this->delimiter_ == '_' && inner->delimiter_ == '_' &&
	this->begin_ + 1 == inner->begin_ &&
	isRightFlankingDelimiterRun(*pos, doubleSize) &&
	(!isLeftFlankingDelimiterRun(*pos, doubleSize) ||
	 (*pos + singleSize < text_->length() && text_->at(*pos + doubleSize).isPunct()))) {
      inner->end_ = -1;
      ++this->count_;
      this->end_ = *pos + 2;
      this->tag_ = &tag;
      ++(*pos);
      
      return true;
    }
  }

  return false;
}

bool Precedence::open(int *pos, Precedence* second) {
  static const QString tag{"em"};
  const int singleSize{1};
  const int doubleSize{2};
  
  if (text_->at(*pos) == '*') {
    if (text_->at(*pos + 1) == '*') {
      if (isLeftFlankingDelimiterRun(*pos, doubleSize)) {
	this->init('*', &tag, *pos);
	second->init('*', &tag, ++(*pos));

	return true;
      } else {
	++(*pos);
      }
    } else if (isLeftFlankingDelimiterRun(*pos, singleSize)) {
      this->init('*', &tag, *pos);
      second->reset();

      return true;
    }
  } else if (text_->at(*pos) == '_') {
    if (text_->at(*pos + 1) == '_') {
      if (isLeftFlankingDelimiterRun(*pos, doubleSize) &&
	  (!isRightFlankingDelimiterRun(*pos, doubleSize) ||
	   (*pos > 0 && text_->at(*pos - 1).isPunct()))) {
	this->init('_', &tag, *pos);
	second->init('_', &tag, ++(*pos));

	return true;
      } else {
	++(*pos);
      }
    } else if (isLeftFlankingDelimiterRun(*pos, singleSize) &&
	       (!isRightFlankingDelimiterRun(*pos, singleSize) ||
		(*pos > 0 && text_->at(*pos - 1).isPunct()))) {
      this->init('_', &tag, *pos);
      second->reset();

      return true;;
    }
  }
  
  this->reset();
  second->reset();

  return false;
}

bool Precedence::isLeftFlankingDelimiterRun(int pos, int size) {
  if (pos + size >= text_->length()) return false;

  QChar followed{text_->at(pos + size)};
  
  if (followed.isSpace()) return false;
  
  if (!followed.isPunct() || pos == 0) return true;
    
  QChar preceded{text_->at(pos - 1)};
  
  return preceded.isSpace() || preceded.isPunct();
}

bool Precedence::isRightFlankingDelimiterRun(int pos, int size) {
  if (pos == 0) return false;
  
  QChar preceded{text_->at(pos - 1)};
  
  if (preceded.isSpace()) return false;
  
  if (!preceded.isPunct() || pos + size >= text_->length()) return true;

  QChar followed{text_->at(pos + size)};

  return followed.isSpace() || followed.isPunct();
}
