// classWriter.h
// Version 2016.11.20
/*
Copyright (c) 1998-2016, NeuroGadgets Inc.
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

#ifndef CLASS_WRITER_TEXT_SERIALIZER_H
#define CLASS_WRITER_TEXT_SERIALIZER_H

// Portable, though may lead to overflow/underflow if the reader's type sizes are different

#include <array>
#include <bitset>
#include <cctype>
#include <deque>
#include <fstream>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <valarray>
#include <vector>

#include <iostream>

class Writer {
private:
	std::string myFileName_;
	std::ofstream fileOut_;
public:
	explicit Writer(const std::string& fileName, bool append = false) :
		myFileName_(fileName)
	{
		fileOut_.open(myFileName_, append ? std::ios::app : std::ios::out);
		if (!fileOut_.is_open()) {
			throw std::runtime_error("Cannot open " + myFileName_);
		}
	}
	const std::string& itsFileName() const { return myFileName_; }

	template<class T> Writer& operator<<(const T& data) { // built-in types
		if (std::is_floating_point<T>::value) {
			fileOut_.precision(std::numeric_limits<T>::max_digits10);
		}
		fileOut_ << ' ' << data;
		if (!fileOut_) {
			throw std::runtime_error("Write error: " + myFileName_);
		}
		return *this; 
	}

	Writer& operator<<(const std::string& data) {
		fileOut_ << ' ' << std::quoted(data);
		if (!fileOut_) {
			throw std::runtime_error("Write error: " + myFileName_);
		}
		return *this; 
	}
	Writer& operator<<(const char* data) {
		return operator<<(std::string(data));
	}
};


template<class T, class U>
inline Writer& operator<<(Writer& wrtr, const std::pair<T, U>& data)
{
	return wrtr << data.first << data.second;
}


template<std::size_t N>
inline Writer& operator<<(Writer& wrtr, const std::bitset<N>& data)
{
	wrtr << data.to_string();
	return wrtr;
}

template<class T, std::size_t size>
inline Writer& operator<<(Writer& wrtr, const std::array<T, size>& data)
{
	// No need to store the size, since it's built into the type.
	for (const auto& d : data) {
		wrtr << d;
	}
	return wrtr;
}

template<class T, class A>
inline Writer& operator<<(Writer& wrtr, const std::deque<T, A>& data)
{
	wrtr << data.size();
	for (const auto& d : data) {
		wrtr << d;
	}
	return wrtr;
}

template<class T, class A>
inline Writer& operator<<(Writer& wrtr, const std::list<T, A>& data)
{
	wrtr << data.size();
	for (const auto& d : data) {
		wrtr << d;
	}
	return wrtr;
}

template<class T, class U, class A>
inline Writer& operator<<(Writer& wrtr, const std::map<T, U, std::less<T>, A>& data)
{
	wrtr << data.size();
	for (const auto& d : data) {
		wrtr << d.first << d.second;
	}
	return wrtr;
}

template<class T, class U, class A>
inline Writer& operator<<(Writer& wrtr, const std::multimap<T, U, std::less<T>, A>& data)
{
	wrtr << data.size();
	for (const auto& d : data) {
		wrtr << d.first << d.second;
	}
	return wrtr;
}

template<class T, class A>
inline Writer& operator<<(Writer& wrtr, const std::set<T, std::less<T>, A>& data)
{
	wrtr << data.size();
	for (const auto& d : data) {
		wrtr << d;
	}
	return wrtr;
}

template<class T, class A>
inline Writer& operator<<(Writer& wrtr, const std::multiset<T, std::less<T>, A>& data)
{
	wrtr << data.size();
	for (const auto& d : data) {
		wrtr << d;
	}
	return wrtr;
}

template<class T, class A>
inline Writer& operator<<(Writer& wrtr, const std::vector<T, A>& data)
{
	wrtr << data.size();
	for (const auto& d : data) {
		wrtr << d;
	}
	return wrtr;
}

template<class T>
inline Writer& operator<<(Writer& wrtr, const std::valarray<T>& data)
{
	const std::size_t s = data.size();
	wrtr << s;
	for (std::size_t i = 0; i < s; ++i) {
		wrtr << data[i];
	}
	return wrtr;
}

#endif
