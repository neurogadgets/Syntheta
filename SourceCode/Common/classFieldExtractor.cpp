// classFieldExtractor.cpp
// Version 2016.11.11

/*
Copyright (c) 2015-2016, NeuroGadgets Inc.
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

#include "classFieldExtractor.h"
#include <sstream>
#include <stdexcept>

FieldExtractor::FieldExtractor(const std::string& s, const char delimiter, bool parsedByWhitespace, int parseUpToFieldIdx) :
	delimiter_(delimiter)
{
	int idxCount = 0; // saves on having to parse unused fields
	if (parsedByWhitespace) {
		std::istringstream is(s);
		std::string entry;
		is >> entry;
		while (is && ++idxCount <= parseUpToFieldIdx) {
			fields_.push_back(entry);
			is >> entry;
		}
	} else {
		std::string::size_type p = 0;
		std::string::size_type q = s.find(delimiter);
		while (q != std::string::npos && ++idxCount < parseUpToFieldIdx) {
			fields_.push_back(s.substr(p, q - p));
			p = q + 1;
			q = s.find(delimiter, p);
		}
		fields_.push_back(s.substr(p, q - p));
	}
}

std::string FieldExtractor::extractRangeOfFields(std::size_t start, std::size_t end) const
{
	if (start == 0 || start > end || end > fields_.size()) {
		throw std::out_of_range("extractRangeOfFields(), arguments out of range");
	}
	std::string s(fields_[--start]);
	while (++start < end) {
		s += delimiter_ + fields_[start];
	}
	return s;
}

std::string FieldExtractor::extractFieldsInSpecifiedOrder(const std::string& theOrder) const
{
	// UNIX cut command-like list, as in 2-5,6,19-20
	// theOrder is expected to be comma-delimited, possibly with ranges.
	// Sanity check the string:
	if (theOrder.find_first_not_of("0123456789-,") != std::string::npos) {
		throw std::runtime_error("\"" + theOrder + "\" is not a valid list of comma-separated ranges of fields");
	}
	// Start by breaking the string into its components:
	const FieldExtractor f(theOrder, ',');
	// Construct the composite string in the order specified, e.g. 7,2 or even overlapping ranges 1-5,2-6:
	std::string result;
	std::string::size_type p;
	const std::size_t n = f.numberOfFields();
	for (std::size_t i = 1; i <= n; ++i) {
		if (!result.empty()) {
			result += delimiter_;
		}
		const std::string& r = f[i];
		p = r.find('-');
		if (p == std::string::npos) {
			result += at(std::stoi(r));
		} else {
			if (r.find('-', p + 1) != std::string::npos) { // e.g. if 5-7-9
				throw std::runtime_error("\"" + theOrder + "\" is not a valid list of fields");
			}
			result += extractRangeOfFields(std::stoi(r.substr(0, p)), std::stoi(r.substr(p + 1)));
		}
	}
	return result;
}

std::size_t FieldExtractor::find(const std::string& fieldStr)
{
	std::size_t idx = 0;
	for (const auto& s : fields_) {
		++idx;
		if (s == fieldStr) return idx;
	}
	return 0;
}
