// classCmdLineArgParser.h
// Version 2015.11.22

/*
Copyright (c) 2001-2015, NeuroGadgets Inc.
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

#ifndef CLASS_CMDLINEARGPARSER_H
#define CLASS_CMDLINEARGPARSER_H

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class CmdLineArgParser {
private:
	std::vector<std::string> theArgs_;
	std::vector<bool> unusedArgs_;
public:
	CmdLineArgParser(int argc, const char* argv[]);

	const std::string& programName() const { return theArgs_.front(); } // may include path
	std::string pathlessProgramName() const;
	
	bool hasExtraneousArguments() const;
	
	int parse(const std::string& key, std::string* theString, bool needArg = false);	
	int parse(const std::string& key, bool* theBool, bool needArg = false); // Assumes T or F.
	int parse(const std::string& key, bool needArg = false);
	template<typename T> int parse(const std::string& key, T* argument, bool needArg = false) {
		std::string stringRep;
		int resultCode = parse(key, &stringRep, needArg);
		if (resultCode == 1) {
			try {
				std::istringstream s(stringRep);
				s >> *argument;
				if (!s) throw 1;
				std::string residual;
				s >> residual;
				if (!residual.empty()) throw 2;
			} catch (std::runtime_error& e) {
				throw;
			} catch (...) {
				throw std::runtime_error("CmdLineArgParser::parse(), problem parsing " + key + ' ' + stringRep);
			}
		}
		return resultCode;
	}
};

std::string commandLineArgsToString(int argc, const char* argv[]);
std::string redactArgument(std::string toRedact, const std::string& flag, const std::string& replaceWith = "==REDACTED==");
	// e.g., to hide passwords when printing commandLineArgsToString()

#endif
