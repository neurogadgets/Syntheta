// tupleStringStreamer.h
// Version 2015.06.14

/*
Copyright (c) 2014-2015, NeuroGadgets Inc.
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

#ifndef TUPLE_STRING_STREAMER_H
#define TUPLE_STRING_STREAMER_H

#include <sstream>
#include <string>
#include <tuple>
#include <utility>

// Allow tuples to be parsed by istream and ostream, inspired by Tony Olsson's Stack Overflow post.
// Also allow pairs to be parsed.

template<typename Type, std::size_t N, std::size_t Last> struct tuple_istreamer
{
	static void tuple_in(std::istream& instream, Type& value) {
		instream >> std::get<N>(value);
		tuple_istreamer<Type, N + 1, Last>::tuple_in(instream, value);
	}
};

template<typename Type, std::size_t N> struct tuple_istreamer<Type, N, N>
{
	static void tuple_in(std::istream& instream, Type& value) {
		instream >> std::get<N>(value);
		// After parsing the last value, make sure the istream is still ok:
		if (!instream) {
			throw std::runtime_error("Error parsing tuple using operator>>(istream&, tuple<Types...>)");
		}
	}
};

template<typename... Types> std::istream& operator>>(std::istream& instream, std::tuple<Types...>& value)
{
	tuple_istreamer<std::tuple<Types...>, 0, sizeof...(Types) - 1>::tuple_in(instream, value);
	return instream;
}

template<typename A, typename B> std::istream& operator>>(std::istream& instream, std::pair<A, B>& value)
{
	instream >> value.first >> value.second;
	if (!instream) {
		throw std::runtime_error("Error parsing pair using operator>>(istream&, pair<A,B>)");
	}
	return instream;
}


template<typename Type, std::size_t N, std::size_t Last> struct tuple_ostreamer
{
	static void tuple_out(std::ostream& outstream, const Type& value) {
		outstream << std::get<N>(value) << ' ';
		tuple_ostreamer<Type, N + 1, Last>::tuple_out(outstream, value);
	}
};

template<typename Type, std::size_t N> struct tuple_ostreamer<Type, N, N>
{
	static void tuple_out(std::ostream& outstream, const Type& value) {
		outstream << std::get<N>(value);
	}
};

template<typename... Types> std::ostream& operator<<(std::ostream& outstream, const std::tuple<Types...>& value)
{
	tuple_ostreamer<std::tuple<Types...>, 0, sizeof...(Types) - 1>::tuple_out(outstream, value);
	return outstream;
}

template<typename A, typename B> std::ostream& operator<<(std::ostream& outstream, const std::pair<A, B>& value)
{
	outstream << value.first << ' ' << value.second;
	return outstream;
}

#endif
