// randomNumberGenerators.cpp
// Version 2019.01.11

/*
Copyright (c) 2001-2019, NeuroGadgets Inc.
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

#include "randomNumberGenerators.h"

std::random_device rd;
std::mt19937 RandNum::mersenneTwisterGenerator(rd());
std::ranlux24_base RandNum::laggedFibonacciGenerator(rd());
std::uniform_real_distribution<double> RandNum::uniformDistribution; // defaults to [0.0,1.0)

std::mutex RandNum::mersenneTwisterMutex;
std::mutex RandNum::laggedFibonacciMutex;
std::mutex RandNum::quickAndDirtyMutex;

static const char* characters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@%+/!#$^?:,(){}[]~`-_."; // The last 22 are from Oracle's list of special characters supported for passwords, omitting the single quote and the backslash from their list

int RandNum::randomIntegerWithinInclusiveRange(int lo, int hi) {
	if (lo == hi) return lo;
	if (lo > hi) std::swap(lo, hi);
	return lo + rndMT(hi - lo + 1);
}

std::string RandNum::generateRandomDigitString(int minSize, int maxSize)
{
	const int itsLength = randomIntegerWithinInclusiveRange(minSize, maxSize);
	std::string result;
	for (int i = 0; i < itsLength; ++i) {
		result += characters[rndMT(10)];
	}
	return result;
}

std::string RandNum::generateRandomLetterString(int minSize, int maxSize)
{
	const int itsLength = randomIntegerWithinInclusiveRange(minSize, maxSize);
	std::string result;
	for (int i = 0; i < itsLength; ++i) {
		result += characters[10 + rndMT(52)];
	}
	return result;
}

std::string RandNum::generateRandomAlphanumericString(int minSize, int maxSize)
{
	const int itsLength = randomIntegerWithinInclusiveRange(minSize, maxSize);
	std::string result;
	for (int i = 0; i < itsLength; ++i) {
		result += characters[rndMT(62)];
	}
	return result;
}

std::string RandNum::generateRandomPassword(int minLength, int maxLength, bool includeSpecial)
{
	if (minLength < 4 || maxLength < 4) {
		throw std::runtime_error("generateRandomPassword(), minimum password length is 4");
	}
	const int length = randomIntegerWithinInclusiveRange(minLength, maxLength);

	const int accommodateSpecial = includeSpecial ? 1 : 0;
	const int num_az = rndMT(length - 2 - accommodateSpecial) + 1;
	const int num_AZ = rndMT(length - num_az - 1 - accommodateSpecial) + 1;
	const int num_09 = rndMT(length - num_az - num_AZ - accommodateSpecial) + 1;
	const int num_Special = length - num_az - num_AZ - num_09;
	assert(num_az > 0 && num_AZ > 0 && num_09 > 0 && (!includeSpecial || num_Special > 0));
	
	std::string password;
	int i;
	for (i = 0; i < num_az; ++i) {
		password += characters[RandNum::rndMT(26) + 10];
	}
	for (i = 0; i < num_AZ; ++i) {
		password += characters[RandNum::rndMT(26) + 36];
	}
	for (i = 0; i < num_09; ++i) {
		password += characters[RandNum::rndMT(10)];
	}
	for (i = 0; i < num_Special; ++i) {
		password += characters[RandNum::rndMT(22) + 62];
	}
	// mix up the characters:
	std::shuffle(password.begin(), password.end(), mersenneTwisterGenerator);
	return password;
}
