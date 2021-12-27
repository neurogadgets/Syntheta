// classFieldExtractor.h
// Version 2018.03.23

/*
Copyright (c) 2015-2018, NeuroGadgets Inc.
Author: Robert L. Charlebois
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of NeuroGadgets Inc. nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CLASS_FIELD_EXTRACTOR_H
#define CLASS_FIELD_EXTRACTOR_H

#include <limits>
#include <string>
#include <vector>

// Note: for indexing, the fields are numbered starting at 1, not 0, so that this acts like UNIX 'cut'

class FieldExtractor {
private:
	std::vector<std::string> fields_;
	char delimiter_;
public:
	FieldExtractor() : delimiter_('\t') { }
	FieldExtractor(const std::string& s, const char delimiter, bool parsedByWhitespace = false, int parseUpToFieldIdx = std::numeric_limits<int>::max());
		// If parsedByWhitespace is true, fields are read as whitespace-delimited, otherwise by <delimiter>
		// <delimiter> is anyway used for output of multiple fields
	
	std::size_t numberOfFields() const { return fields_.size(); }
	std::vector<std::string>& extractVector() { return fields_; }
	const std::vector<std::string>& extractVector() const { return fields_; }
	
	std::string& at(std::size_t field) {
		return fields_.at(field - 1);
	}
	const std::string& at(std::size_t field) const {
		return fields_.at(field - 1);
	}
	std::string& operator[](std::size_t field) {
		return fields_[field - 1];
	}
	const std::string& operator[](std::size_t field) const {
		return fields_[field - 1];
	}

	std::string extractRangeOfFields(std::size_t start, std::size_t end) const; // inclusive range
	std::string extractNthAndSubsequentFields(std::size_t field) const {
		return extractRangeOfFields(field, fields_.size());
	}
	std::string extractFieldsInSpecifiedOrder(const std::string& theOrder) const;
		// UNIX cut command-like list, as in 2-5,6,19-20,3
	std::size_t find(const std::string& fieldStr); // returns 0 if not found
};

#endif
