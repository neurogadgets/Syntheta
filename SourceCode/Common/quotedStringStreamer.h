// quotedStringStreamer.h
// Version 2020.07.26

/*
Copyright (c) 2014-2020, NeuroGadgets Inc.
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

#ifndef QUOTED_STRING_STREAMER_H
#define QUOTED_STRING_STREAMER_H

#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

// Allow pairs, tuples and other values to be parsed by istream and ostream,
// inspired by Tony Olsson's Stack Overflow post.
// This is different from tupleStringStreamer in that strings are quoted.

class QuotedStringStreamIn {
private:
	std::istream& is_;
public:
	explicit QuotedStringStreamIn(std::istream& is) : is_(is) { }
	std::istream& impl() { return is_; }
};

class QuotedStringStreamOut {
private:
	std::ostream& os_;
public:
	explicit QuotedStringStreamOut(std::ostream& os) : os_(os) { }
	std::ostream& impl() { return os_; }
};

template<typename R> R quotedExtractValueFromString(const std::string& s)
{
	std::istringstream iss(s);
	QuotedStringStreamIn qiss(iss);
	R value;
	qiss >> value;
	if (!qiss.impl()) {
		throw std::runtime_error("quotedExtractValueFromString(), could not retrieve value");
	}
	std::string residual;
	qiss.impl() >> residual;
	if (!residual.empty()) {
		throw std::runtime_error("quotedExtractValueFromString(), additional data returned");
	}
	return value;
}



template<typename Type, std::size_t N, std::size_t Last> struct q_tuple_istreamer
{
	static void q_tuple_in(QuotedStringStreamIn& instream, Type& value) {
		instream >> std::get<N>(value);
		q_tuple_istreamer<Type, N + 1, Last>::q_tuple_in(instream, value);
	}
};

template<typename Type, std::size_t N> struct q_tuple_istreamer<Type, N, N>
{
	static void q_tuple_in(QuotedStringStreamIn& instream, Type& value) {
		instream >> std::get<N>(value);
		// After parsing the last value, make sure the istream is still ok:
		if (!instream.impl()) {
			throw std::runtime_error("Error parsing tuple using operator>>(QuotedStringStreamIn&, tuple<Types...>)");
		}
	}
};

template<typename... Types> QuotedStringStreamIn& operator>>(QuotedStringStreamIn& instream, std::tuple<Types...>& value)
{
	q_tuple_istreamer<std::tuple<Types...>, 0, sizeof...(Types) - 1>::q_tuple_in(instream, value);
	return instream;
}

template<typename A, typename B> QuotedStringStreamIn& operator>>(QuotedStringStreamIn& instream, std::pair<A, B>& value)
{
	instream >> value.first >> value.second;
	if (!instream.impl()) {
		throw std::runtime_error("Error parsing pair using operator>>(QuotedStringStreamIn&, pair<A,B>)");
	}
	return instream;
}

template<typename T> QuotedStringStreamIn& operator>>(QuotedStringStreamIn& instream, T& value)
{
	if constexpr (std::is_same_v<T, std::string>) {
		instream.impl() >> std::quoted(value);
	} else {
		instream.impl() >> value;
	}
	if (!instream.impl()) {
		throw std::runtime_error("QuotedStringStreamIn: Error parsing value using operator>>()");
	}
	return instream;
}


template<typename Type, std::size_t N, std::size_t Last> struct q_tuple_ostreamer
{
	static void q_tuple_out(QuotedStringStreamOut& outstream, const Type& value) {
		outstream << std::get<N>(value);
		outstream.impl() << ' ';
		q_tuple_ostreamer<Type, N + 1, Last>::q_tuple_out(outstream, value);
	}
};

template<typename Type, std::size_t N> struct q_tuple_ostreamer<Type, N, N>
{
	static void q_tuple_out(QuotedStringStreamOut& outstream, const Type& value) {
		outstream << std::get<N>(value);
	}
};

template<typename... Types> QuotedStringStreamOut& operator<<(QuotedStringStreamOut& outstream, const std::tuple<Types...>& value)
{
	q_tuple_ostreamer<std::tuple<Types...>, 0, sizeof...(Types) - 1>::q_tuple_out(outstream, value);
	return outstream;
}

template<typename A, typename B> QuotedStringStreamOut& operator<<(QuotedStringStreamOut& outstream, const std::pair<A, B>& value)
{
	outstream << value.first;
	outstream.impl() << ' ';
	outstream << value.second;
	return outstream;
}

template<typename T> QuotedStringStreamOut& operator<<(QuotedStringStreamOut& outstream, const T& value)
{
	if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, const char*>) {
		outstream.impl() << std::quoted(value);
	} else {
		outstream.impl() << value;
	}
	return outstream;
}

#endif
