// classRunningSum.h
// Version 2019.10.10

/*
Copyright (c) 2017-2019, NeuroGadgets Inc.
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

#ifndef CLASS_RUNNING_SUM_H
#define CLASS_RUNNING_SUM_H

#include "classReader.h"
#include "classWriter.h"
#include <cmath>
#include <type_traits>

template<class T> struct RunningSum {
	static_assert(std::is_floating_point<T>::value, "RunningSum only applies to floating point types.");
	T sum_;
	T c_;

	RunningSum(T init = T()) : sum_(init), c_(T()) { }
	RunningSum& operator+=(const T& val) { // Kahan summation
		const T y = val - c_;
		const T t = sum_ + y;
		c_ = (t - sum_) - y;
		sum_ = t;
		return *this;
	}
	RunningSum& operator-=(const T& val) {
		return operator+=(-val);
	}
	RunningSum& operator*=(const T& val) {
		sum_ *= val;
		c_ *= val;
		return *this;
	}
	T get() const { return sum_; }
	void clear() {
		sum_ = T();
		c_ = T();
	}
};

template<class T> Reader& operator>>(Reader& rdr, RunningSum<T>& rs)
{
	return rdr >> rs.sum_ >> rs.c_;
}

template<class T> Writer& operator<<(Writer& wrtr, const RunningSum<T>& rs)
{
	return wrtr << rs.sum_ << rs.c_;
}


template<class T, typename U = std::uint64_t> struct Sums {
	static_assert(std::is_integral<U>::value, "Sums count must be an integer type.");
	U n;
	RunningSum<T> sumX;
	RunningSum<T> sumX2;
	
	Sums() : n(0) { }
	Sums(std::uint32_t itsN, T itsSumX, T itsSumX2) : n(itsN), sumX(itsSumX), sumX2(itsSumX2) { }
	
	void clear() {
		n = 0;
		sumX.clear();
		sumX2.clear();
	}
	Sums& operator+=(T x) {
		++n;
		sumX += x;
		sumX2 += x * x;
		return *this;
	}
	T mean() const { return n > 0 ? sumX.get() / n : T(); }
	T variance() const { return n > 1 ? (sumX2.get() - sumX.get() * sumX.get() / n) / (n - 1) : T(); }
	T standardDev() const { return std::sqrt(variance()); }
};

template<class T> Reader& operator>>(Reader& rdr, Sums<T>& s)
{
	rdr >> s.n >> s.sumX >> s.sumX2;
	return rdr;
}

template<class T> Writer& operator<<(Writer& wrtr, const Sums<T>& s)
{
	wrtr << s.n << s.sumX << s.sumX2;
	return wrtr;
}

#endif
