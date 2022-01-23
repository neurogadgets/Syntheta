// ngiFileUtilities.cpp
// Version 2022.01.23

/*
Copyright (c) 2013-2022, NeuroGadgets Inc.
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

#include "ngiFileUtilities.h"
#include "ngiAlgorithms.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>

Lockfile::Lockfile(const std::string& lockfileName, bool wait) :
	myLockfileDescriptor_(-1)
{ // Adapted from http://stackoverflow.com/questions/1599459/optimal-lock-file-method
	do {
		mode_t m = umask(0);
		myLockfileDescriptor_ = open(lockfileName.c_str(), O_RDWR | O_CREAT, 0666);
		umask(m);
		if (myLockfileDescriptor_ >= 0 && flock(myLockfileDescriptor_, LOCK_EX | LOCK_NB) < 0) {
			close(myLockfileDescriptor_);
			myLockfileDescriptor_ = -1;
		}
		if (wait && myLockfileDescriptor_ < 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); // arbitrary delay
		}
	} while (wait && myLockfileDescriptor_ < 0);
}

Lockfile::~Lockfile()
{
	if (hasLock()) {
		close(myLockfileDescriptor_);
	}
}


namespace ngi {

std::time_t fileModificationTime(const std::string& filename)
{
/*
	auto ftime = fs::last_write_time(filename);
	#if __cplusplus >= 201703L
		return decltype(ftime)::clock::to_time_t(ftime); // broken, fixes coming in C++20?
	#else
		return ftime; // Boost last_write_time() returns time_t
	#endif
*/
	#if __cplusplus >= 201703L
		struct stat info;
		if (stat(filename.c_str(), &info) != 0) { // the file does not exist
			throw std::runtime_error("fileModificationTime(): cannot locate " + filename);
		}
		return info.st_mtime;
	#else
		return fs::last_write_time(filename); // Boost last_write_time() returns time_t
	#endif
}

void filedelete(const std::string& filename)
{
	if (std::remove(filename.c_str())) {
		throw std::runtime_error("filedelete(): cannot delete " + filename);
	}
}

void filemove(const std::string& from, std::string to)
{
	if (from == to || from.back() == '/') {
		throw std::runtime_error("filemove(): cannot rename " + from + " to " + to);
	}
	if (to.back() == '/') {
		const std::string::size_type p = from.rfind('/');
		if (p == std::string::npos) {
			to += from;
		} else {
			to += from.substr(p + 1);
		}
	}
	if (std::rename(from.c_str(), to.c_str())) {
		throw std::runtime_error("filemove(): cannot rename " + from + " to " + to + " (" + std::strerror(errno) + ')');
	}
}

void filecopy(const std::string& theOriginal, const std::string& theCopy)
{
	std::ifstream from(openFileAndTest(theOriginal));
	std::ofstream to(theCopy);
	to << from.rdbuf();
}

bool filecopy_or_create(const std::string& theOriginal, const std::string& theCopy)
{
	std::ofstream to(theCopy);
	std::ifstream from(theOriginal);
	if (from.is_open()) {
		to << from.rdbuf();
		return true;
	}
	return false; // created empty file
}

void filecat(const std::vector<std::string>& source, const std::string& sink)
{
	assert(std::find(source.begin(), source.end(), sink) == source.end());
	// Make sure that all of the source files are available:
	std::vector<std::string>::const_iterator vIt, vItEnd = source.end();
	for (vIt = source.begin(); vIt != vItEnd; ++vIt) {
		if (!fs::exists(*vIt)) {
			throw std::runtime_error("filecat(): cannot locate " + *vIt);
				// should prevent the more serious errors below
		}
	}
	std::ofstream sinkFile(sink);
	for (vIt = source.begin(); vIt != vItEnd; ++vIt) {
		std::ifstream sourceFile(openFileAndTest(vIt->c_str()));
		sinkFile << sourceFile.rdbuf();
	}
}

} // namespace ngi

std::string extractFileNameFromPath(const std::string& path)
{ // until filesystem::path is used ###
	const std::string::size_type slashPos = path.rfind('/');
	return slashPos == std::string::npos ? path : path.substr(slashPos + 1);
}

std::string readFileIntoString(const std::string& filename)
{
	std::ifstream infile(openFileAndTest(filename));
	return std::string((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
}

void convertToUnixLineEndings(const std::string& filename)
{ // no effect if the line endings are already UNIX
	// Three styles: '\r', "\r\n" and/or '\n', maybe mixed
	// The strategy then is to convert all '\r' into '\n' and to suppress the '\n' in "\r\n":
	const std::string s(readFileIntoString(filename));
	std::string::size_type r = s.find('\r');
	if (r != std::string::npos) { // if it does not already have only UNIX line endings
		std::ofstream outfile(filename); // overwrite
		bool wasR = false;
		for (const auto& c : s) {
			if (c == '\r') {
				outfile.put('\n');
				wasR = true;
			} else {
				if (!wasR || c != '\n') {
					outfile.put(c);
				}
				wasR = false;
			}
		}
	}
}

std::string deduceApplicationDirectory(const std::string& app)
{
	const std::string::size_type slashPos = app.rfind('/');
	if (slashPos == std::string::npos) {
		// It looks like the program was accessed using an alias, so resolve its path:
		char buf[1024];
		int lenBuf;
		if ((lenBuf = readlink(app.c_str(), buf, sizeof(buf) - 1)) != -1) {
			buf[lenBuf] = '\0';
			const std::string pathToApplication(buf);
			const std::string::size_type p = pathToApplication.rfind('/');
			if (p == std::string::npos) {
				throw std::runtime_error("Cannot deduce the path in which " + app + " is running");
			}
			return pathToApplication.substr(0, p); // Note: no trailing /
		} else {
			throw std::runtime_error("Cannot deduce the path in which " + app + " is running");
		}
	}
	return app.substr(0, slashPos); // Note: no trailing /
}

std::string currentWorkingDirectory()
{
	std::string cwd(fs::current_path().string());
	if (cwd.back() == '/') cwd.pop_back(); // ### Test, then remove if redundant
	return cwd;
}

void checkForValidPath(const fs::path& fileName)
{
	const fs::path path(fileName.parent_path());
	if (!fs::is_directory(path)) {
		throw std::runtime_error("checkForValidPath(), fileName path " + path.string() + " is invalid");
	}
}

std::string checksum(const std::string& filename)
{
	// Use shasum -a 512 to compute the checksum:
	const std::string checksumCommand("shasum -a 512 " + filename);
	std::pair<std::string, int> checksumResult(pipe_to_string(checksumCommand));
	if (checksumResult.second) {
		throw std::runtime_error("checksum(), " + checksumCommand + " returned error code: " + std::to_string(checksumResult.second));
	}
	std::string& result = checksumResult.first; // for convenience, below
	const std::string::size_type spacePos = result.find(' ');
	if (spacePos != 128) {
		std::transform(result.begin(), result.end(), result.begin(), [](char c) { return c == '\n' ? ' ' : c; });
		throw std::runtime_error("checksum(), " + checksumCommand + " returned error: " + result);
	} // else it's a checksum
	return result.substr(0, spacePos);
}

std::uint64_t countLinesInFile(const std::string& filename)
{
	std::ifstream infile(openFileAndTest(filename));
	// Note std::count is an alternative, but may miss the last line if it ends in EOF instead of '\n'?
	// Also, count's return type might be too small.
	//    return std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
	std::uint64_t count = 0;
	std::string theLine;
	while (std::getline(infile, theLine, '\n')) {
		++count;
	}
	return count;
}

std::ifstream openFileAndTest(const std::string& name)
{
	std::ifstream file(name);
	if (!file.is_open()) {
		throw std::runtime_error("Cannot open " + name);
	}
	return std::move(file); // ifstream has a move constructor, but not a copy constructor.
}

template<class DirIt> std::vector<fs::path> listFilesInDirectory(const fs::path& directory, const std::regex& pattern)
{
	std::vector<fs::path> listing;
	for (const auto& p : DirIt(directory)) {
		if (fs::is_regular_file(p.status()) && std::regex_search(p.path().string(), pattern)) {
			listing.push_back(p.path());
		}
	}
	return listing;
}

std::vector<fs::path> listFilesInDirectory(const fs::path& directory, bool recursive, const std::regex& pattern)
{
	return recursive ?
		listFilesInDirectory<fs::recursive_directory_iterator>(directory, pattern) :
		listFilesInDirectory<fs::directory_iterator>(directory, pattern);
}

std::vector<fs::path> listDirectoriesInDirectory(const fs::path& directory, const std::regex& pattern)
{
	std::vector<fs::path> listing;
	for (const auto& p : fs::directory_iterator(directory)) {
		if (fs::is_directory(p.status()) && std::regex_search(p.path().string(), pattern)) {
			listing.push_back(p.path());
		}
	}
	return listing;
}

std::vector<fs::path> autocompleteFilesystemName(const std::string& filenamePrefix)
{
	const std::string::size_type p = filenamePrefix.rfind('/');
	const std::string partialName(p == std::string::npos ? filenamePrefix : filenamePrefix.substr(p + 1));
	const std::string directory(p == std::string::npos ? std::string("./") : filenamePrefix.substr(0, p));
	if (partialName.empty()) {
		return listFilesInDirectory(directory);
	}
	std::vector<fs::path> names;
	for (const auto& p : fs::directory_iterator(directory)) {
		const std::string theFileName(p.path().filename().string());
		if (theFileName.find(partialName) == 0) {
			names.push_back(p.path());
		}
	}
	return names;
}
