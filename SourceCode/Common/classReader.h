// classReader.h
// Version 2021.11.05

/*
Copyright (c) 1998-2021, NeuroGadgets Inc.
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

#ifndef CLASS_READER_TEXT_SERIALIZER_H
#define CLASS_READER_TEXT_SERIALIZER_H

#include <array>
#include <bitset>
#include <deque>
#include <fstream>
#include <iomanip>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <valarray>
#include <vector>

class Reader {
private:
	std::string myFileName_;
	std::ifstream fileIn_;
public:
	Reader() { } // not associated with any file
	Reader(const std::string& fileName, int& isOpen) :
		myFileName_(fileName)
	{
		fileIn_.open(myFileName_);
		isOpen = fileIn_.is_open();
	}
	explicit Reader(const std::string& fileName) : // throws if cannot open
		myFileName_(fileName)
	{
		fileIn_.open(myFileName_);
		if (!fileIn_.is_open()) {
			throw std::runtime_error("Cannot open " + fileName);
		}
	}

	bool isOpen() const { return fileIn_.is_open(); }
	const std::string& itsFileName() const { return myFileName_; }
	void close() { fileIn_.close(); }
	
	template<class T> T readOneValue() {
		T theValue;
		*this >> theValue;
		return theValue;
	}

	template<class T> Reader& operator>>(T& data) { // built-in types
		fileIn_ >> data;
		if (!fileIn_) {
			throw std::runtime_error("Read error (T): " + myFileName_);
		}
		return *this;
	}

	Reader& operator>>(std::string& data) {
		fileIn_ >> std::quoted(data); // Quoted string possibly containing escape characters
		if (!fileIn_) {
			throw std::runtime_error("Read error (string): " + myFileName_);
		}
		return *this;
	}
};


template<class T, class U>
inline Reader& operator>>(Reader& rdr, std::pair<T, U>& rhs)
{
	return rdr >> rhs.first >> rhs.second;
}

template<std::size_t N>
inline Reader& operator>>(Reader& rdr, std::bitset<N>& data)
{
	std::string temp;
	rdr >> temp;
	data = std::bitset<N>(temp);
	return rdr;
}

template<class T, std::size_t size>
inline Reader& operator>>(Reader& rdr, std::array<T, size>& data)
{
	// No need to clear the array, since it's a fixed size and is being overwritten.
	for (auto& d : data) {
		rdr >> d;
	}
	return rdr;
}

template<class T, class A>
inline Reader& operator>>(Reader& rdr, std::deque<T, A>& data)
{
	std::size_t s;
	rdr >> s;
	data.clear();
	T element;
	for (std::size_t i = 0; i < s; ++i) {
		rdr >> element;
		data.push_back(element);
	}
	return rdr;
}

template<class T, class A>
inline Reader& operator>>(Reader& rdr, std::list<T, A>& data)
{
	std::size_t s;
	rdr >> s;
	data.clear();
	T element;
	for (std::size_t i = 0; i < s; ++i) {
		rdr >> element;
		data.push_back(element);
	}
	return rdr;
}

template<class T, class U, class A>
inline Reader& operator>>(Reader& rdr, std::map<T, U, std::less<T>, A>& data)
{
	std::size_t s;
	rdr >> s;
	data.clear();
	T key;
	U element;
	for (std::size_t i = 0; i < s; ++i) {
		rdr >> key >> element;
		data.emplace(key, element);
	}
	return rdr;
}

template<class T, class U, class A>
inline Reader& operator>>(Reader& rdr, std::multimap<T, U, std::less<T>, A>& data)
{
	std::size_t s;
	rdr >> s;
	data.clear();
	T key;
	U element;
	for (std::size_t i = 0; i < s; ++i) {
		rdr >> key >> element;
		data.emplace(key, element);
	}
	return rdr;
}

template<class T, class A>
inline Reader& operator>>(Reader& rdr, std::set<T, std::less<T>, A>& data)
{
	std::size_t s;
	rdr >> s;
	data.clear();
	T element;
	for (std::size_t i = 0; i < s; ++i) {
		rdr >> element;
		data.insert(element);
	}
	return rdr;
}

template<class T, class A>
inline Reader& operator>>(Reader& rdr, std::multiset<T, std::less<T>, A>& data)
{
	std::size_t s;
	rdr >> s;
	data.clear();
	T element;
	for (std::size_t i = 0; i < s; ++i) {
		rdr >> element;
		data.insert(element);
	}
	return rdr;
}

template<class T, class A>
inline Reader& operator>>(Reader& rdr, std::vector<T, A>& data)
{
	data.clear();
	std::size_t s;
	rdr >> s;
	data.reserve(s);
	T element;
	for (std::size_t i = 0; i < s; ++i) {
		rdr >> element;
		data.push_back(element);
	}
	return rdr;
}

template<class T>
inline Reader& operator>>(Reader& rdr, std::valarray<T>& data)
{
	std::size_t s;
	rdr >> s;
	data.resize(s);
	for (std::size_t i = 0; i < s; ++i) {
		rdr >> data[i];
	}
	return rdr;
}

#endif
