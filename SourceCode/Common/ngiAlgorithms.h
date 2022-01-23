// ngiAlgorithms.h
// Version 2022.01.22

/*
Copyright (c) 2005-2022, NeuroGadgets Inc.
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
// * Code snippets freely obtained from elsewhere, as indicated below, are not necessarily subject to these  *
// * rules. They are included in this file for convenience. If there are any objections, the code can be     *
// * separated out to their own separate files.                                                              *
// ***********************************************************************************************************

#ifndef NGI_ALGORITHMS_H
#define NGI_ALGORITHMS_H

#include "classRunningSum.h"
	// included to support pearson()
#include "tupleStringStreamer.h"
	// included to support pair and tuple parsing in extractValueFromString()
#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <future>
#include <iomanip>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <numeric>
#include <regex>
#include <stdexcept>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template<class T> inline T constrainToRange(const T& lo, const T& hi, const T& val)
{ // like std::clamp, but without undefined behaviour, and with argument order like constrainInsideRange below
	if (hi < lo) throw std::invalid_argument("constrainToRange(): hi < lo");
	return (val < lo) ? lo : ((val > hi) ? hi : val);
}

template<class T> inline T constrainInsideRange(const T& lo, const T& hi, const T& val)
{
	static_assert(std::is_floating_point<T>::value, "Type must be floating point");
	if (hi <= lo) throw std::invalid_argument("constrainInsideRange(): hi <= lo");
	return (val <= lo) ? std::nextafter(lo, hi) : ((val >= hi) ? std::nextafter(hi, lo) : val);
}

template<class T> class ConstrainToRange {
private:
	T loBound_;
	T hiBound_;
public:
	ConstrainToRange(const T& lo, const T& hi) : loBound_(lo), hiBound_(hi)
		{ if (hi < lo) throw std::invalid_argument("ConstrainToRange(lo, hi): hi < lo"); }
	void operator()(T& a) const {
		a = constrainToRange(loBound_, hiBound_, a);
	}
};

template<class InIt1, class InIt2> inline
	bool sets_intersect(InIt1 first1, InIt1 last1, InIt2 first2, InIt2 last2) noexcept
{ // Adapted from Plauger et al. 2001
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			++first1;
		} else if (*first2 < *first1) {
			++first2;
		} else {
			return true;
		}
	}
	return false;
}

template<class InIt1, class InIt2> inline
	std::size_t num_intersecting(InIt1 first1, InIt1 last1, InIt2 first2, InIt2 last2) noexcept
{ // adapted from http://en.cppreference.com/w/cpp/algorithm/set_intersection
	std::size_t count = 0;
    while (first1 != last1 && first2 != last2) {
        if (*first1 < *first2) {
            ++first1;
        } else {
            if (!(*first2 < *first1)) {
               ++first1;
			   ++count;
            }
            ++first2;
        }
    }
    return count;
}

template<class T> std::string addCommasToInteger(T i)
{ // Adapted from https://stackoverflow.com/questions/7276826/c-format-number-with-commas
	static_assert(std::is_integral<T>::value, "Type must be integral");
	std::string s(std::to_string(i));
	const int start = s.front() == '-' ? 1 : 0;
	int insertPosition = s.length() - 3;
	while (insertPosition > start) {
		s.insert(insertPosition, 1, ',');
		insertPosition -= 3;
	}
	return s;
}

template<class R, class T> inline R roundToNearestIntegerType(T d) noexcept
{
	static_assert(std::is_floating_point<T>::value, "Argument type must be floating point");
	static_assert(std::is_integral<R>::value, "Return type must be integral");
	static const T half = 0.5;
	return static_cast<R>(d + (d < T(0) ? -half : half));
}

template<class T> inline int roundToNearestInt(T d) noexcept
{
	return roundToNearestIntegerType<int>(d);
}

template<class R, class T> inline R roundIfIntegerResult(T d) noexcept
{
	return std::is_integral<R>::value ? roundToNearestIntegerType<R>(d) : static_cast<R>(d);
}

template<class R, class T> inline R scaleToRange(const R& lo, const R& hi, const T& d)
{
	static_assert(std::is_floating_point<T>::value, "Type T must be floating point");
	if (lo > hi) throw std::invalid_argument("scaleToRange(): lo > hi");
	return roundIfIntegerResult<R, T>(lo + (hi - lo) * d);
}

template<class T> inline void checkForClosedUnitInterval(const T& t, const std::string& argumentName = "argument")
{
	static_assert(std::is_floating_point<T>::value, "Argument type must be floating point");
	if (t < T(0) || t > T(1)) {
		throw std::invalid_argument("checkForClosedUnitInterval(): " + argumentName + " outside range [0,1]");
	}
}

template<class T> inline void checkForOpenUnitInterval(const T& t, const std::string& argumentName = "argument")
{
	static_assert(std::is_floating_point<T>::value, "Argument type must be floating point");
	if (t <= T(0) || t >= T(1)) {
		throw std::invalid_argument("checkForOpenUnitInterval(): " + argumentName + " outside range (0,1)");
	}
}

template<class T> inline void checkForClosedOpenUnitInterval(const T& t, const std::string& argumentName = "argument")
{
	static_assert(std::is_floating_point<T>::value, "Argument type must be floating point");
	if (t < T(0) || t >= T(1)) {
		throw std::invalid_argument("checkForClosedOpenUnitInterval(): " + argumentName + " outside range [0,1)");
	}
}

template<class T> inline void checkForOpenClosedUnitInterval(const T& t, const std::string& argumentName = "argument")
{
	static_assert(std::is_floating_point<T>::value, "Argument type must be floating point");
	if (t <= T(0) || t > T(1)) {
		throw std::invalid_argument("checkForOpenClosedUnitInterval(): " + argumentName + " outside range (0,1]");
	}
}

template<class R, class T> inline R scaleLinearlyToRange(const R& lo, const R& hi, const T& u)
{
	checkForClosedUnitInterval(u);
	return scaleToRange(lo, hi, u);
}

template<class R, class T> inline R scaleQuadraticallyToRange(const R& lo, const R& hi, const T& u)
{
	checkForClosedUnitInterval(u);
	return scaleToRange(lo, hi, u * u);
}

template<class R, class T> inline R scaleInverseQuadraticallyToRange(const R& lo, const R& hi, const T& u)
{
	checkForClosedUnitInterval(u);
	return scaleToRange(lo, hi, std::sqrt(u));
}

template<class T> inline T logisticFn(const T& minVal, const T& val, const T& maxVal, const T& multiplier)
{ // multiplier is positive for increasing S curve, negative for decreasing S curve, typically around 6.
	static const T one = 1;
	if (maxVal <= minVal) throw std::invalid_argument("logisticFn(): maxVal <= minVal");
	const T t = (multiplier + multiplier) * (val - minVal) / (maxVal - minVal) - multiplier;
	return one / (one + std::exp(-t)); // returns a value in the range (0.0,1.0)
}

template<class T> inline T ticktock()
{ // Returns the microsecond-precise time since "epoch time".
	return static_cast<T>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

template<class T> inline T normalizeOpenRange(const T val, const T minReturnValue = 0)
{
	static_assert(std::is_floating_point<T>::value, "Argument type must be floating point");
	static const T one = 1;
	if (val < T(0)) throw std::invalid_argument("normalizeOpenRange(): val < 0.0");
	checkForClosedOpenUnitInterval(minReturnValue); // [0,1)
	return std::max(minReturnValue, one - one / (val + one)); // returns [minReturnValue,1)
}

template<class T> inline T denormalizeOpenRange(const T val)
{
	static_assert(std::is_floating_point<T>::value, "Argument type must be floating point");
	static const T one = 1;
	checkForClosedOpenUnitInterval(val); // [0,1)
	return one / (one - val) - one; // return [0, infinity)
}

template<typename R> R extractValueFromString(const std::string& s)
{
	std::istringstream iss(s);
	R value;
	iss >> value;
	if (!iss) {
		throw std::runtime_error("extractValueFromString(), could not retrieve value");
	}
	std::string residual;
	iss >> residual;
	if (!residual.empty()) {
		throw std::runtime_error("extractValueFromString(), additional data returned");
	}
	return value;
}

template<class S> inline std::string replaceCharacter(const std::string& s, const char c, const S& r)
{ // S is a string or a char
	std::string converted;
	for (const auto& sIt : s) {
		if (sIt == c) {
			converted += r;
		} else {
			converted += sIt;
		}
	}
	return converted;
}
void replaceAll(std::string& toEdit, const std::string& from, const std::string& to);
void replaceAll(std::string& toEdit, const std::string& from, const char to);
void replaceAll(std::string& toEdit, const char from, const std::string& to);
void replaceAll(std::string& toEdit, const char from, const char to);
std::string trimLeadingTrailingAndConsecutiveCharacters(const std::string& str, const char c);
std::string trimConsecutiveCharacters(const std::string& str, const char c);
std::string stripNumbersAndSpaces(const std::string& s);
std::string::size_type longestCommonSubstringLength(const std::string& str1, const std::string& str2);

inline std::string convertToUpperCase(std::string uc)
{
	for (auto& nk : uc) {
		nk = static_cast<char>(std::toupper(static_cast<unsigned char>(nk)));
	}
	return uc;
}

template<typename T, typename U> inline bool beginsWith(const T& sequence, const U& prefix) noexcept
{ // make constexpr once C++20 is available
	if (prefix.size() > sequence.size()) {
		return false;
	}
	return std::mismatch(std::cbegin(prefix), std::cend(prefix), std::cbegin(sequence)).first == std::cend(prefix);
}

template<typename T> inline bool beginsWith(const T& sequence, const char* prefix) noexcept
{ // make constexpr once C++20 is available
	const std::size_t prefixLen = std::strlen(prefix);
	if (prefixLen > sequence.size()) {
		return false;
	}
	return std::mismatch(prefix, &prefix[prefixLen], std::cbegin(sequence)).first == &prefix[prefixLen];
}

template<typename T, typename U> inline bool endsWith(const T& sequence, const U& suffix) noexcept
{ // make constexpr once C++20 is available
	if (suffix.size() > sequence.size()) {
		return false;
	}
	return std::mismatch(std::crbegin(suffix), std::crend(suffix), std::crbegin(sequence)).first == std::crend(suffix);
}

template<typename T> inline bool endsWith(const T& sequence, const char* suffix) noexcept
{ // make constexpr once C++20 is available
	const std::size_t suffixLen = std::strlen(suffix);
	if (suffixLen > sequence.size()) {
		return false;
	}
	return std::mismatch(suffix, &suffix[suffixLen], std::cend(sequence) - suffixLen).first == &suffix[suffixLen];
}

template<typename T> std::string integral_to_hex(T i)
{ // adapted from http://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
  return stream.str();
}

inline int circularArrayIndex(int indexToWrap, int theArraySize) noexcept
{ // accepts positive or negative values for indexToWrap
	const int wrapped = (indexToWrap % theArraySize);
	return (wrapped >= 0 ? wrapped : theArraySize + wrapped);
}

class CircularIndex {
private:
	int arraySize_;
public:
	explicit CircularIndex(int theArraySize) : arraySize_(theArraySize) {
		if (theArraySize < 1) throw std::invalid_argument("CircularIndex::CircularIndex(), array size < 1");
	}
	int operator[](int indexToWrap) noexcept { return circularArrayIndex(indexToWrap, arraySize_); }
};

class PipeExec {
private:
	std::future<std::pair<std::string, int>> myFuture_;
public:
	enum class Out { _capture_, _ignore_ }; // _ignore_ if all we want is the exit status
	PipeExec(const std::string& command, Out doCapture, std::launch sync);
		// sync is either std::launch::async, or std::launch::deferred, or (std::launch::async | std::launch::deferred)
	
	std::pair<std::string, int> getResult() { return myFuture_.get(); }
};

std::string currentTime();
void procrastinate(std::uint32_t secondsIntoTomorrow = 0);
std::string padWithLeadingCharacters(std::string s, const std::size_t toSize, const char c = ' ');
std::string deleteCharacter(const std::string& s, const char c);
std::string formatSecondsIntoDHHMMSS(std::uint64_t secondsLeft);
std::pair<std::string, int> pipe_to_string(const std::string& command, PipeExec::Out captureStyle = PipeExec::Out::_capture_);
void execute(const std::string& command, int expectedStatus = 0, std::ostream* outStream = nullptr); // throws if command returns a status other than expectedStatus
std::vector<std::string> grep(std::basic_istream<char>& stream, const std::regex& pattern, std::size_t maxNum = std::numeric_limits<std::size_t>::max());
std::string grep1(std::basic_istream<char>& stream, const std::regex& pattern);
std::vector<std::string> grepvec(std::vector<std::string> v, const std::regex& pattern);
std::string whichSystemGrep(); // BSD, GNU or Other
std::string itoa(unsigned int number, int minNumDigits);
std::string baseAlpha(int i);

template<typename T> T computeRoundingAdjustment(const std::string& numAsString)
{
	static_assert(std::is_floating_point<T>::value, "computeRoundingAdjustment(): type must be floating point");
	
	// Validate the number:
	if (numAsString.empty() || !std::all_of(numAsString.begin(), numAsString.end(), [](char c) { return c == '.' || std::isdigit(static_cast<unsigned char>(c)); }) || std::count(numAsString.begin(), numAsString.end(), '.') > 1) {
		throw std::runtime_error("computeRoundingAdjustment(): bad number format (" + numAsString + ')');
	}
	const std::string::size_type dotPos = numAsString.find('.');
	if (dotPos == std::string::npos || dotPos == numAsString.length() - 1) { // it's an integer; assume it's precise to the last digit
		return T(1) / T(2);
	}
	const int roundingDigit = numAsString.length() - dotPos; // e.g., for 3.14, the rounding digit is 3
	return T(5) * std::pow(T(10), -roundingDigit); // e.g., for 3.14, this is 5e-3
}

namespace ngi {

template<class InputIt1, class InputIt2, class BinaryPredicate>
std::pair<InputIt1, InputIt2> mismatch(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, BinaryPredicate p) noexcept
{ // from http://en.cppreference.com/w/cpp/algorithm/mismatch
    while (first1 != last1 && first2 != last2 && p(*first1, *first2)) {
        ++first1, ++first2;
    }
    return std::make_pair(first1, first2);
} // Provisionally, until a C++14 compiler is available...

template<typename T> std::string to_string(T val)
{
	#if __cplusplus >= 201703L
	if constexpr (std::is_floating_point<T>::value) {
	#else
	if (std::is_floating_point<T>::value) {
	#endif
		static const std::string fmtStr("%." + std::to_string(std::numeric_limits<T>::digits10) + (std::is_same<long double, typename std::remove_cv<T>::type>::value ? "Lg" : "g"));
		const char* fmt = fmtStr.c_str();
		const int sz = std::snprintf(nullptr, 0, fmt, val) + 1;
		std::vector<char> buf(sz);
		std::snprintf(&buf[0], sz, fmt, val);
		return std::string(buf.data());
	} else {
		return std::to_string(val);
	}
} // To make up for std::to_string()'s deficiencies in representing floating point types

} // namespace ngi


// String catenation, from user syam at: https://stackoverflow.com/questions/18892281/most-optimized-way-of-concatenation-in-strings
namespace syam_detail {
	template<typename> struct string_size_impl;

	template<std::size_t N> struct string_size_impl<const char[N]> {
  		static constexpr std::size_t size(const char (&) [N]) { return N - 1; }
	};

	template<std::size_t N> struct string_size_impl<char[N]> {
		static std::size_t size(char (&s) [N]) { return N ? std::strlen(s) : 0; }
	};

	template<> struct string_size_impl<const char*> {
		static std::size_t size(const char* s) { return s ? std::strlen(s) : 0; }
	};

	template<> struct string_size_impl<char*> {
		static std::size_t size(char* s) { return s ? std::strlen(s) : 0; }
	};

	template<> struct string_size_impl<std::string> {
		static std::size_t size(const std::string& s) { return s.size(); }
	};

	template<> struct string_size_impl<const char> { // Added, to allow individual characters to be included
		static constexpr std::size_t size(const char) { return 1; }
	};

	template<> struct string_size_impl<char> { // Added, to allow individual characters to be included
		static constexpr std::size_t size(char) { return 1; }
	};

	template<typename String> std::size_t string_size(String&& s) {
		using noref_t = typename std::remove_reference<String>::type;
		using string_t = typename std::conditional<std::is_array<noref_t>::value, noref_t, typename std::remove_cv<noref_t>::type>::type;
		return string_size_impl<string_t>::size(s);
	}

	template<typename...> struct concatenate_impl;

	template<typename String> struct concatenate_impl<String> {
		static std::size_t size(String&& s) { return string_size(s); }
		static void concatenate(std::string& result, String&& s) { result += s; }
	};

	template<typename String, typename... Rest> struct concatenate_impl<String, Rest...> {
		static std::size_t size(String&& s, Rest&&... rest) {
			return string_size(s) + concatenate_impl<Rest...>::size(std::forward<Rest>(rest)...);
		}
		static void concatenate(std::string& result, String&& s, Rest&&... rest) {
			result += s;
			concatenate_impl<Rest...>::concatenate(result, std::forward<Rest>(rest)...);
		}
	};
} // namespace syam_detail

template<typename... Strings> std::string concatenate(Strings&&... strings) {
	std::string result;
	result.reserve(syam_detail::concatenate_impl<Strings...>::size(std::forward<Strings>(strings)...));
	syam_detail::concatenate_impl<Strings...>::concatenate(result, std::forward<Strings>(strings)...);
	return result;
} // Use is, e.g.: std::string result = concatenate("hello", ' ', "world");

// from https://www.youtube.com/watch?v=wG49AGqQ5Aw
// Nicholas Ormrod's "Fantastic Algorithms and Where to Find Them"
// Boyer-Moore majority vote algorithm:
// second element of pair is false if a second, counting pass is needed through the data
template <typename IIt> auto BoyerMooreMajorityVote(IIt begin, IIt end)
{
	typename IIt::value_type m;
	std::uint64_t i = 0, n = 0;
	for (; begin != end; ++begin) {
		++n;
		if (i == 0) {
			m = *begin;
			i = 1;
		} else if (m == *begin) {
			++i;
		} else {
			--i;
		}
	}
	return std::make_pair(m, i > n / 2);
}

template<class T> class BoyerMooreMajority {
private:
	std::uint64_t i;
	std::uint64_t n;
	T m;
public:
	BoyerMooreMajority() : i(0), n(0), m{} { }
	void vote(const T& x) {
		++n;
		if (i == 0) {
			m = x;
			i = 1;
		} else if (m == x) {
			++i;
		} else {
			--i;
		}
	}
	void prepareForSecondPass() {
		i = 0;
		n = 0;
	}
	void secondPass(const T& x) {
		++n;
		if (m == x) {
			++i;
		}
	}
	const T& candidate() const { return m; }
	bool isMajority() const { return i > n / 2; }
};
// Also consider Morris Traversal and HyperLogLog


// Basic stats functions:
template<class T, class InIt> inline T median(InIt first, InIt last, T)
{
	if (first == last) return T(0);
	// Copy the data into a vector:
	std::vector<T> vec(first, last);
	// Compute the median using nth_element():
	typename std::vector<T>::iterator vi = vec.begin() + vec.size() / 2;
	std::nth_element(vec.begin(), vi, vec.end());
	return *vi;
}


// The Kahan summation algorithm is much more precise than std::accumulate() and std::inner_product
// See https://en.wikipedia.org/wiki/Kahan_summation_algorithm
// Note: Requires the compiler not to optimize based on arithmetic associativity

template<class T, class InIt> inline T sum(InIt first, InIt last, T)
{ 
	T s = 0;
	T c = 0;
	for (; first != last; ++first) {
		const T y = *first - c;
		const T t = s + y;
		c = (t - s) - y;
		s = t;
	}
	return s;
}

template<class T, class InIt> inline T mean(InIt first, InIt last, T)
{
	if (first == last) return T(0);
	return sum(first, last, T(0)) / std::distance(first, last);
}

template<class InIt, class T> inline T sumOfSquares(InIt first, InIt last, T)
{
	T ss = 0;
	T c = 0;
	for (; first != last; ++first) {
		const T y = *first * *first - c;
		const T t = ss + y;
		c = (t - ss) - y;
		ss = t;
	}
	return ss;
}

template<class InIt, class T> inline T rootMeanSquare(InIt first, InIt last, T)
{
	if (first == last) return T(0);
	return std::sqrt(sumOfSquares(first, last, T(0)) / std::distance(first, last));
}

template<class InIt, class T> inline T variance(InIt first, InIt last, T)
{
	const auto n = std::distance(first, last);
	if (n < 2) return T(0);
	const T s = sum(first, last, T(0));
	return (sumOfSquares(first, last, T(0)) - s * s / n) / (n - 1);	
}

template<class InIt, class T> inline T standardDev(InIt first, InIt last, T)
{
	return std::sqrt(variance(first, last, T(0)));
}

template<class T> T pearson(const std::vector<T>& x, const std::vector<T>& y)
{ // Correlation
	static_assert(std::is_floating_point<T>::value, "Type must be floating point");
	if (x.empty() || x.size() != y.size()) {
		throw std::runtime_error("pearson(), empty input or mismatched vector sizes");
	}
	const T ax = mean(x.begin(), x.end(), T(0));
	const T ay = mean(y.begin(), y.end(), T(0));
	RunningSum<T> syy, sxy, sxx;
	typename std::vector<T>::const_iterator xIt, yIt, xItEnd = x.end();
	for (xIt = x.begin(), yIt = y.begin(); xIt != xItEnd; ++xIt, ++yIt) {
		const T xt = *xIt - ax;
		const T yt = *yIt - ay;
		sxx += xt * xt;
		syy += yt * yt;
		sxy += xt * yt;
	}
	if (sxx.get() == T(0) || syy.get() == T(0)) return T(1); // identical vectors
	return sxy.get() / std::sqrt(sxx.get() * syy.get());
}

template<class T> T mode(std::vector<T> v)
{
	std::sort(v.begin(), v.end());
	T theMode{};
	std::uint32_t count = 0;
	const auto itEnd = v.end();
	for (auto it = v.begin(); it != itEnd;) {
		const auto e = std::upper_bound(it, itEnd, *it);
		const std::uint32_t d = std::distance(it, e);
		if (count < d) { // finds the first mode, ignores the rest
			count = d;
			theMode = *it;
		}
		it = e;
	}
	return theMode;
}

template<class T> std::vector<T> modes(std::vector<T> v)
{
	std::sort(v.begin(), v.end());
	std::vector<T> theModes;
	std::uint32_t count = 0;
	const auto itEnd = v.end();
	for (auto it = v.begin(); it != itEnd;) {
		const auto e = std::upper_bound(it, itEnd, *it);
		const std::uint32_t d = std::distance(it, e);
		if (count < d) {
			count = d;
			theModes.assign(1, *it);
		} else if (count == d) { // a tie
			theModes.push_back(*it);
		}
		it = e;
	}
	return theModes;
}

template<class T> std::pair<T, std::uint32_t> modeWithCount(std::vector<T> v)
{
	std::sort(v.begin(), v.end());
	std::pair<T, std::uint32_t> theMode(T{}, 0);
	const auto itEnd = v.end();
	for (auto it = v.begin(); it != itEnd;) {
		auto e = std::upper_bound(it, itEnd, *it);
		std::uint32_t d = std::distance(it, e);
		if (theMode.second < d) { // finds the first mode, ignores the rest
			theMode.second = d;
			theMode.first = *it;
		}
		it = e;
	}
	return theMode;
}

template<class T, class InIt> inline T euclidianDistanceSquared(InIt first1, InIt last1, InIt first2, T)
{
	T s = 0;
	T c = 0;
	for (; first1 != last1; ++first1, ++first2) {
		const T delta = *first1 - *first2;
		const T y = delta * delta - c;
		const T t = s + y;
		c = (t - s) - y;
		s = t;
	}
	return s;
}

template<class T, class InIt> inline T euclidianDistance(InIt first1, InIt last1, InIt first2, T)
{
	if (first1 == last1) return T(0);
	return std::sqrt(euclidianDistanceSquared(first1, last1, first2, T(0)));
}

// Adapted from Sean Parent's talk, C++ Seasoning from CppCon 2013:
// Move a range of items to position toWhere:
template<typename It> std::pair<It, It> slide(It first, It last, It toWhere)
{
	if (toWhere < first) return { toWhere, std::rotate(toWhere, first, last) };
	if (last < toWhere) return { std::rotate(first, last, toWhere), toWhere };
	return { first, last };
}
// Gather a set of items and place them at position toWhere:
template<typename It, typename UniPred> std::pair<It, It> gather(It first, It last, It toWhere, UniPred p)
{
	using value_type = typename std::iterator_traits<It>::value_type;
	return { std::stable_partition(first, toWhere, [&](const value_type& x){ return !p(x); }), std::stable_partition(toWhere, last, p) };
}

// From https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
// -1 if negative, +1 if positive, 0 if zero
template <typename T> inline constexpr
int signum(T x, std::false_type is_signed) {
    return T(0) < x;
}

template <typename T> inline constexpr
int signum(T x, std::true_type is_signed) {
    return (T(0) < x) - (x < T(0));
}

template <typename T> inline constexpr
int signum(T x) {
    return signum(x, std::is_signed<T>());
}

// From Stack Overflow user Yakk - Adam Nevraumont
// https://stackoverflow.com/questions/52722405/avoid-dangling-reference-for-reverse-range-based-for-loop-implementation
template<class R> struct backwards_t {
  R r;
  constexpr auto begin() const { using std::rbegin; return rbegin(r); }
  constexpr auto begin() { using std::rbegin; return rbegin(r); }
  constexpr auto end() const { using std::rend; return rend(r); }
  constexpr auto end() { using std::rend; return rend(r); }
};
template<class R> constexpr backwards_t<R> backwards(R&& r) { return { std::forward<R>(r) }; }
// use, e.g.:
// for (const auto& v : backwards(getContainer()))

// Adapted from Nathan Reed
// http://reedbeta.com/blog/python-like-enumerate-in-cpp17
#if __cplusplus >= 201703L
template <typename T,
          typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T&& iterable)
{
    struct iterator
    {
        std::size_t i;
        TIter iter;
        bool operator!=(const iterator& other) const { return iter != other.iter; }
        void operator++() { ++i; ++iter; }
        auto operator*() const { return std::tie(i, *iter); }
    };
    struct iterable_wrapper
    {
        T iterable;
        auto begin() { return iterator{ 0, std::begin(iterable) }; }
        auto end() { return iterator{ 0, std::end(iterable) }; }
    };
    return iterable_wrapper{ std::forward<T>(iterable) };
}
/* Example usage:
std::vector<Thing> things;
...
for (auto [i, thing] : enumerate(things))
{
    // i gets the index and thing gets the Thing in each iteration
}
*/
#endif

// formatPercentage() was contributed by Brandon Lieng:
std::string formatPercentage(double percent, int digits);

std::string escape_ascii_for_LaTeX(std::string s);

#endif
