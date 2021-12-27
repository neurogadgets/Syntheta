// ngiAlgorithms.cpp
// Version 2021.12.25

/*
Copyright (c) 2005-2021, NeuroGadgets Inc.
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

// ***********************************************************************************************************
// * Important note: the above copyright and license is for NeuroGadgets Inc.'s original code herein.        *
// * Code snippets freely obtained from elsewhere, as indicated, are not necessarily subject to these rules. *
// * They are included in this file for convenience. If there are any objections, the code can be separated  *
// * out to their own separate files.                                                                        *
// ***********************************************************************************************************

#include "ngiAlgorithms.h"
#include "final_act.h"
#include <cerrno>
#include <iomanip>
#include <iostream>
#include <thread>

std::string currentTime()
{
	const std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::ostringstream ost;
	ost << std::put_time(std::localtime(&currentTime), "%Y%m%d");
	return ost.str();
}

void procrastinate(std::uint32_t secondsIntoTomorrow) 
{ // Adapted from: http://www.cplusplus.com/reference/thread/this_thread/sleep_until/
	// Why do today what can be put off until tomorrow?
	std::time_t theTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm ptm = *std::localtime(&theTime);
	ptm.tm_mday += secondsIntoTomorrow / 86400 + 1;
	ptm.tm_hour = (secondsIntoTomorrow % 86400) / 3600;
	ptm.tm_min = (secondsIntoTomorrow % 3600) / 60;
	ptm.tm_sec = (secondsIntoTomorrow % 60);
	std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(std::mktime(&ptm)));
}

std::string padWithLeadingCharacters(std::string s, const std::size_t toSize, const char c)
{
    if (toSize > s.size()) {
		s.insert(0, toSize - s.size(), c);
	}
	return s;
}

std::string deleteCharacter(const std::string& s, const char c)
{
	std::string converted;
	std::copy_if(s.begin(), s.end(), std::back_inserter(converted), [&c](const char& sIt) { return sIt != c; });
	return converted;
}

void replaceAll(std::string& toEdit, const std::string& from, const std::string& to)
{ // mutable reference in order to avoid unnecessary string copies
	const std::size_t fromSize = from.size();
	const std::size_t toSize = to.size();
	std::string::size_type n = 0;
	while ((n = toEdit.find(from, n)) != std::string::npos) {
		toEdit.replace(n, fromSize, to);
		n += toSize;
	}
}

void replaceAll(std::string& toEdit, const std::string& from, const char to)
{
	const std::size_t fromSize = from.size();
	std::string::size_type n = 0;
	while ((n = toEdit.find(from, n)) != std::string::npos) {
		toEdit.replace(n, fromSize, 1, to);
		++n;
	}
}

void replaceAll(std::string& toEdit, const char from, const std::string& to)
{
	const std::size_t toSize = to.size();
	std::string::size_type n = 0;
	while ((n = toEdit.find(from, n)) != std::string::npos) {
		toEdit.replace(n, 1, to);
		n += toSize;
	}
}

void replaceAll(std::string& toEdit, const char from, const char to)
{
	for (char& c : toEdit) {
		if (c == from) {
			c = to;
		}
	}
}

std::string trimLeadingTrailingAndConsecutiveCharacters(const std::string& str, const char c)
{
	std::string trimmed;
	for (const auto& s : str) {
		if (s != c || (!trimmed.empty() && trimmed.back() != c)) {
			trimmed += s;
		}
	}
	if (trimmed.back() == c) {
		trimmed.pop_back();
	}
	return trimmed;
}

std::string trimConsecutiveCharacters(const std::string& str, const char c)
{
	std::string trimmed;
	for (const auto& s : str) {
		if (s != c || trimmed.empty() || trimmed.back() != c) {
			trimmed += s;
		}
	}
	return trimmed;
}

std::string stripNumbersAndSpaces(const std::string& s)
{
	std::string result;
	for (const auto& c : s) {
		if (std::isalpha(static_cast<unsigned char>(c))) result += c;
	}
	return result;
}

std::string::size_type longestCommonSubstringLength(const std::string& str1, const std::string& str2)
{
	const std::string::size_type sz1 = str1.size();
	const std::string::size_type sz2 = str2.size();
	std::vector<std::string::size_type> curr(sz2, 0);
	std::vector<std::string::size_type> prev(sz2, 0);
	std::string::size_type maxSubstr = 0;
	for (std::string::size_type i = 0; i < sz1; ++i) {
		for (std::string::size_type j = 0; j < sz2; ++j) {
			if (str1[i] != str2[j]) {
				curr[j] = 0;
			} else {
				if (i == 0 || j == 0) {
					curr[j] = 1;
				} else {
					curr[j] = 1 + prev[j - 1];
				}
				if (maxSubstr < curr[j]) {
					maxSubstr = curr[j];
				}
			}
		}
		curr.swap(prev);
	}
	return maxSubstr;
}

std::string formatSecondsIntoDHHMMSS(std::uint64_t secondsLeft)
{
	std::string timeLeft, unit;
	const std::uint64_t daysLeft = secondsLeft / 86400;
	if (daysLeft > 0) {
		timeLeft += std::to_string(daysLeft) + ':';
		unit = " days";
	}
	secondsLeft -= daysLeft * 86400;
	const std::uint64_t hoursLeft = secondsLeft / 3600;
	if (hoursLeft > 0 || !timeLeft.empty()) {
		if (timeLeft.empty()) {
			timeLeft += std::to_string(hoursLeft) + ':';
			unit = " hours";
		} else {
			timeLeft += padWithLeadingCharacters(std::to_string(hoursLeft), 2, '0') + ':';
		}
	}
	secondsLeft -= hoursLeft * 3600;
	const std::uint64_t minutesLeft = secondsLeft / 60;
	if (minutesLeft > 0 || !timeLeft.empty()) {
		if (timeLeft.empty()) {
			timeLeft += std::to_string(minutesLeft) + ':';
			unit = " minutes";
		} else {
			timeLeft += padWithLeadingCharacters(std::to_string(minutesLeft), 2, '0') + ':';
		}
	}
	secondsLeft -= minutesLeft * 60;
	if (timeLeft.empty()) {
		timeLeft += std::to_string(secondsLeft) + " seconds";
	} else {
		timeLeft += padWithLeadingCharacters(std::to_string(secondsLeft), 2, '0') + unit;
	}
	return timeLeft;
}

PipeExec::PipeExec(const std::string& command, Out doCapture, std::launch sync)
{
	myFuture_ = std::async(sync, pipe_to_string, command, doCapture);
}


// Adapted from JLBorges, http://www.cplusplus.com/forum/beginner/117874/
std::pair<std::string, int> pipe_to_string(const std::string& command, PipeExec::Out captureStyle)
{ // Commented out features not supported by GCC 4.8.4
//	std::FILE* pipe = std::popen(command.c_str(), "r");
	std::FILE* pipe = popen(command.c_str(), "r");
	if (pipe) {
		const auto _ = gsl::finally([&pipe](){ if (pipe) pclose(pipe); }); // closes pipe upon exception
		std::string s;
		constexpr std::size_t maxLineSize = 1024; // arbitrary
		char line[maxLineSize];
		const bool doCapture = (captureStyle == PipeExec::Out::_capture_);
		while (std::fgets(line, maxLineSize, pipe)) {
			if (doCapture) s += line; // otherwise we are only interested in the exit status
		}
//		const int exitStatus = std::pclose(pipe);
		const int exitStatus = pclose(pipe);
		pipe = nullptr;
		return std::make_pair(s, exitStatus);
	} else {
		const int theError = errno;
		throw std::runtime_error("pipe_to_string(): popen() failed for command " + command + ": " + std::strerror(theError));
	}
}

void execute(const std::string& command, int expectedStatus, std::ostream* outStream)
{
	const auto result(pipe_to_string(command, outStream ? PipeExec::Out::_capture_ : PipeExec::Out::_ignore_));
	if (outStream && !result.first.empty()) {
		*outStream << result.first;
		if (result.first.back() != '\n') {
			*outStream << '\n';
		}
	}
	if (result.second != expectedStatus) {
		throw std::runtime_error("Exit status " + std::to_string(result.second) + " was returned from executing: " + command);
	}
}

std::vector<std::string> grep(std::basic_istream<char>& stream, const std::regex& pattern, std::size_t maxNum)
{ // stream can be an ifstream, or an istringstream, etc.
	std::vector<std::string> out;
	std::string theLine;
	std::size_t numFound = 0;
	while (numFound < maxNum && std::getline(stream, theLine, '\n')) {
		if (std::regex_search(theLine, pattern)) {
			out.push_back(theLine);
			++numFound;
		}
	}
	return out;
}

std::string grep1(std::basic_istream<char>& stream, const std::regex& pattern)
{ // stream can be an ifstream, or an istringstream, etc.
	std::string theLine;
	while (std::getline(stream, theLine, '\n')) {
		if (std::regex_search(theLine, pattern)) {
			return theLine;
		}
	}
	return std::string();
}

std::vector<std::string> grepvec(std::vector<std::string> v, const std::regex& pattern)
{
	v.erase(std::stable_partition(v.begin(), v.end(), [&pattern](const std::string& s) { return std::regex_search(s, pattern); }), v.end());
	return v;
}

std::string whichSystemGrep()
{
	const auto grepInfo(pipe_to_string("grep -V | head -n 1"));
	if (grepInfo.second) {
		throw std::runtime_error("whichSystemGrep(), failure to execute \"grep -V | head -n 1\"");
	} else {
		return grepInfo.first.find("BSD") != std::string::npos ? "BSD" : (grepInfo.first.find("GNU") != std::string::npos ? "GNU" : "Other");
	}
}

// Convert an unsigned integer to a string representation prepended with padding '0' characters,
// e.g. for minNumDigits == 3, 1 becomes "001", 1000 stays at "1000":
std::string itoa(unsigned int number, int minNumDigits)
{
	std::ostringstream ost;
	ost << std::setw(minNumDigits) << std::setfill('0') << number;
	return ost.str();
}

std::string baseAlpha(int i)
{
	std::string alpha(1, 'a' + static_cast<char>(i % 26));
	i /= 26;
	while (i > 0) {
		alpha += 'a' + static_cast<char>(--i % 26);
		i /= 26;
	}
	std::reverse(alpha.begin(), alpha.end());
	return alpha; // a-z, then aa-zz, then aaa-zzz, etc.
}

// The following function was contributed by Brandon Lieng:
std::string formatPercentage(double percent, int digits)
{
	assert(digits > 0);
	if (percent < 0.0 || percent > 100.0) {
		throw std::invalid_argument("formatPercentage(), percent argument " + ngi::to_string(percent) + " is out of the range [0,100]");
	}
	if (percent == 0.0) {
		return "0";
	} else if (percent == 100.0) {
		return "100";
	} else if (digits == 1) {
		if (percent < 10.0) {
			return std::to_string(static_cast<int>(percent + 0.5));
		} else {
			return std::to_string(static_cast<int>(percent / 10.0 + 0.5) * 10);
		}
	} else {
		std::ostringstream oss;
		oss << std::setprecision(digits) << percent;
			// precision is about setting the appropriate number of significant digits
		return oss.str();
	}
}

std::string escape_ascii_for_LaTeX(std::string s)
{
	// Start by replacing backslashes and curly braces, since we might be introducing new ones below:
	const auto originalSize = s.length();
	replaceAll(s, '\\', "\\textbackslashTEMP"); // requires two passes, see below
	const bool hadBackslashes = (originalSize != s.length());
	replaceAll(s, '{', "\\{");
	replaceAll(s, '}', "\\}");
	if (hadBackslashes) {
		replaceAll(s, "textbackslashTEMP", "textbackslash{}"); // add the curly braces
	}
	// Then substitute ASCII characters that have special meaning in LaTeX:
	std::string::size_type p = s.find_first_of("\"#$%&<>[]^_`|~\n\t");
	while (p != std::string::npos) {
		switch (s[p]) {
			case '"':
				s.replace(p, 1, "{''}");
				p += 4;
				break;
			case '#':
				s.replace(p, 1, "\\#");
				p += 2;
				break;
			case '$':
				s.replace(p, 1, "\\$");
				p += 2;
				break;
			case '%':
				s.replace(p, 1, "\\%");
				p += 2;
				break;
			case '&':
				s.replace(p, 1, "\\&");
				p += 2;
				break;
			case '<':
				s.replace(p, 1, "\\textless{}");
				p += 11;
				break;
			case '>':
				s.replace(p, 1, "\\textgreater{}");
				p += 14;
				break;
			case '[':
				s.replace(p, 1, "{[}");
				p += 3;
				break;
			case ']':
				s.replace(p, 1, "{]}");
				p += 3;
				break;
			case '^':
				s.replace(p, 1, "\\textasciicircum{}");
				p += 18;
				break;
			case '_':
				s.replace(p, 1, "\\_");
				p += 2;
				break;
			case '`':
				s.replace(p, 1, "{}`");
				p += 3;
				break;
			case '|':
				s.replace(p, 1, "\\textbar{}");
				p += 10;
				break;
			case '~':
				s.replace(p, 1, "\\textasciitilde{}");
				s += 17;
				break;
			case '\n':
				s.replace(p, 1, "\\\\");
				p += 2;
				break;
			case '\t':
				s[p] = ' ';
				++p;
				break;
			default:
				++p;
				break;
		}
		p = s.find_first_of("\"#$%&<>[]^_`|~\n\t", p);
	}
	return s;
}
