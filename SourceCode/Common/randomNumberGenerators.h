// randomNumberGenerators.h
// Version 2020.09.17

/*
Copyright (c) 2001-2020, NeuroGadgets Inc.
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

#ifndef RANDOM_NUMBER_GEN_H
#define RANDOM_NUMBER_GEN_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <mutex>
#include <random>
#include <string>

class RandNum {
private:
	static std::mt19937 mersenneTwisterGenerator;
	static std::ranlux24_base laggedFibonacciGenerator;
	static std::uniform_real_distribution<double> uniformDistribution;
	static std::mutex mersenneTwisterMutex;
	static std::mutex laggedFibonacciMutex;
	static std::mutex quickAndDirtyMutex;
	
	template<typename T> static T doMultiply(double r, T multiplier) {
		T result = static_cast<T>(r * static_cast<double>(multiplier));
		assert(result != multiplier || result == T(0));
		return result;
	}
public:
	static void reseedLF(uint32_t newSeed) {
		std::lock_guard<std::mutex> lock(laggedFibonacciMutex);
		laggedFibonacciGenerator.seed(newSeed);
	}
	static double rndLF() {
		std::lock_guard<std::mutex> lock(laggedFibonacciMutex);
		return uniformDistribution(laggedFibonacciGenerator);
	}

	static void reseedMT(uint32_t newSeed) {
		std::lock_guard<std::mutex> lock(mersenneTwisterMutex);
		mersenneTwisterGenerator.seed(newSeed);
	}
	static double rndMT() {
		std::lock_guard<std::mutex> lock(mersenneTwisterMutex);
		return uniformDistribution(mersenneTwisterGenerator);
	}
	static double rndNormalMT(std::normal_distribution<double>& nd)
	{ // nd supplied from outside, since nd has a mean and standard deviation
		std::lock_guard<std::mutex> lock(mersenneTwisterMutex);
		return nd(mersenneTwisterGenerator);
	} // ### deprecated in favour of rndDistributionMT() below:
	template<class D> static double rndDistributionMT(D& distribution)
	{ // distribution supplied from outside
		std::lock_guard<std::mutex> lock(mersenneTwisterMutex);
		return distribution(mersenneTwisterGenerator);
	}
	template<class RandomIt> static void shuffleRange(RandomIt&& a, RandomIt&& b)
	{
		std::lock_guard<std::mutex> lock(mersenneTwisterMutex);
		std::shuffle(a, b, mersenneTwisterGenerator);
	}

	static double rndQD() {
		static const double invTwo32 = 1.0 / std::ldexp(1.0, 32);
		static const uint32_t a = 1664525;
		static const uint32_t c = 1013904223;
		static uint32_t idum = rndLF(c);
		std::lock_guard<std::mutex> lock(quickAndDirtyMutex);
		idum = idum * a + c;
		return static_cast<double>(idum) * invTwo32;
	}

	static bool maybe(double odds = 0.5) { return (rndQD() < odds); }
	
	template<typename T> static T rndLF(T multiplier) {
		return doMultiply(rndLF(), multiplier);
	}
	template<typename T> static T rndMT(T multiplier) {
		return doMultiply(rndMT(), multiplier);
	}
	template<typename T> static T rndQD(T multiplier) {
		return doMultiply(rndQD(), multiplier);
	}
	
	static int randomIntegerWithinInclusiveRange(int lo, int hi);
	static std::string generateRandomDigitString(int minSize, int maxSize);
	static std::string generateRandomLetterString(int minSize, int maxSize);
	static std::string generateRandomAlphanumericString(int minSize, int maxSize);
	static std::string generateRandomPassword(int minLength, int maxLength, bool includeSpecial = false);
};

// from https://www.youtube.com/watch?v=wG49AGqQ5Aw
// Nicholas Ormrod's "Fantastic Algorithms and Where to Find Them"
// Return exactly k random elements:
template<typename IIt> auto reservoir_sampling(IIt begin, IIt end, std::size_t k)
{
	std::vector<typename IIt::value_type> ret;
	ret.reserve(k);
	for (std::size_t n = 1; begin != end; ++begin, ++n) {
		if (ret.size() < k) {
			ret.push_back(*begin);
		} else {
			const std::size_t r = RandNum::rndMT(n);
			if (r < k) {
				ret[r] = *begin;
			}
		}
	}
	return ret;
}

template<typename T> class ReservoirSampler {
private:
	std::size_t k_;
	std::size_t n_;
	std::vector<T> items_;
public:
	explicit ReservoirSampler(std::size_t k) : k_(k), n_(0) { }
	const std::vector<T>& itsItems() const { return items_; }
	void consider(const T& item) {
		++n_;
		if (items_.size() < k_) {
			items_.push_back(item);
		} else {
			const std::size_t r = RandNum::rndMT(n_);
			if (r < k_) {
				items_[r] = item;
			}
		}
	}
	void sort() {
		std::sort(items_.begin(), items_.end());
	}
	void clear() {
		n_ = 0;
		items_.clear();
	}
};

#endif
