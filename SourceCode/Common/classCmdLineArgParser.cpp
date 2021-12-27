// classCmdLineArgParser.cpp
// Version 2021.12.24

/*
Copyright (c) 2001-2021, NeuroGadgets Inc.
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

#include "classCmdLineArgParser.h"
#include <algorithm>
#include <cctype>

CmdLineArgParser::CmdLineArgParser(int argc, const char* argv[]) :
	unusedArgs_(argc, true)
{
	unusedArgs_[0] = false; // the program name
	theArgs_.reserve(argc);
	for (int i = 0; i < argc; ++i) {
		theArgs_.push_back(std::string(argv[i]));
	}
}

std::string CmdLineArgParser::pathlessProgramName() const
{
	const std::string::size_type slashPos = theArgs_.front().rfind('/');
	return (slashPos == std::string::npos ? theArgs_.front() : theArgs_.front().substr(slashPos + 1));
}

bool CmdLineArgParser::hasExtraneousArguments() const
{
	return std::find(unusedArgs_.cbegin(), unusedArgs_.cend(), true) != unusedArgs_.cend();
}

int CmdLineArgParser::parse(const std::string& key, std::string* theString, bool needArg)
{
	auto vi = std::find(theArgs_.begin(), theArgs_.end(), key);
	if (vi == theArgs_.end()) {
		if (needArg) throw std::runtime_error("CmdLineArgParser::parse(), " + key + " not found");
		return 0; // key not found.
	}
	if (++vi == theArgs_.end()) {
		throw std::runtime_error("CmdLineArgParser::parse(), no argument specified for " + key);
	}
	const std::string& s = *vi; // for brevity, below
	if (s[0] == '-' && (s.length() < 2 || !std::isdigit(s[1]))) {
		// Checks for keys (format being e.g. -a or --a) or stray -, but e.g. -56 is okay
		throw std::runtime_error("CmdLineArgParser::parse(), invalid argument (" + s + ") specified for " + key);
	} // Likely the argument is missing
	*theString = s;
	const auto idx = std::distance(theArgs_.begin(), vi);
	if (!(unusedArgs_[idx] && unusedArgs_[idx - 1])) { // if either was already used
		throw std::runtime_error("CmdLineArgParser::parse(string, string*), argument re-parsing error.");
	}
	unusedArgs_[idx] = false;
	unusedArgs_[idx - 1] = false;
	return 1; // Success.
}

int CmdLineArgParser::parse(const std::string& key, bool* theBool, bool needArg)
{
	std::string stringRep;
	int resultCode = parse(key, &stringRep, needArg);
	if (resultCode == 1) {
		if (stringRep == "T") {
			*theBool = true;
		} else if (stringRep == "F") {
			*theBool = false;
		} else {
			throw std::runtime_error("CmdLineArgParser::parse(string, bool), expected T/F for " + key + " but found " + stringRep);
		}
	}
	return resultCode;
}

int CmdLineArgParser::parse(const std::string& key, bool needArg)
{
	auto vi = std::find(theArgs_.begin(), theArgs_.end(), key);
	if (vi == theArgs_.end()) {
		if (needArg) throw std::runtime_error("CmdLineArgParser::parse(string), " + key + " not found");
		return 0;
	}
	const auto idx = std::distance(theArgs_.begin(), vi);
	if (!unusedArgs_[idx]) {
		throw std::runtime_error("CmdLineArgParser::parse(string), argument re-parsing error.");
	}
	unusedArgs_[idx] = false;
	return 1;
}


std::string commandLineArgsToString(int argc, const char* argv[])
{
	std::ostringstream ost;
	ost << argv[0];
	for (int i = 1; i < argc; ++i) {
		ost << ' ' << argv[i];
	}
	return ost.str();
}

std::string redactArgument(std::string toRedact, const std::string& flag, const std::string& replaceWith)
{
	const std::string::size_type f = toRedact.find(' ' + flag + ' ');
	if (f != std::string::npos) { // no effect if the flag isn't specified (e.g. if optional)
		std::string::size_type a = f + 2 + flag.length();
		while (a < toRedact.length() && toRedact[a] == ' ') ++a;
		const std::string argument(toRedact.substr(a, toRedact.find(' ', a) - a));
		if (argument.empty() || (argument.front() == '-' && (argument.length() < 2 || !std::isdigit(argument[1])))) {
			throw std::runtime_error("redactArgument(): " + flag + " has no argument");
		}
		toRedact.replace(a, argument.length(), replaceWith);
	}
	return toRedact;
}
