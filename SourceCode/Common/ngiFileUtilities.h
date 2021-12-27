// ngiFileUtilities.h
// Version 2019.01.30

/*
Copyright (c) 2013-2019, NeuroGadgets Inc.
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

#ifndef NGI_FILE_UTILITIES
#define NGI_FILE_UTILITIES

#include <ctime>
#include <iosfwd>
#include <regex>
#include <string>
#include <vector>

#if __cplusplus >= 201703L
	#include <filesystem>
	namespace fs = std::filesystem;
#else
	#include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#endif

#include <sys/stat.h>
#include <sys/types.h>


class Lockfile { // An RAII file locker based on flock()
private:
	std::string myLockfileName_;
	int myLockfileDescriptor_;
public:
	explicit Lockfile(const std::string& lockfileName, bool wait = true);
	~Lockfile();
	bool hasLock() const { return myLockfileDescriptor_ >= 0; } // only useful if wait was false
};

namespace ngi {
	 // For setting permissions of newly created files:
	const mode_t rw = (S_IRUSR | S_IWUSR);
	const mode_t rwr = (S_IRUSR | S_IWUSR | S_IRGRP);
	// For setting permissions of newly created folders:
	const mode_t rwxrx = (S_IRWXU | S_IRGRP | S_IXGRP);

	std::time_t fileModificationTime(const std::string& filename);
	inline bool localFileMissingOrEmpty(const std::string& filename) {
		return (!fs::exists(filename) || fs::is_empty(filename));	
	}
	inline bool fileIsNewerThanTime(const std::string& filename, const std::time_t theTime) {
		return (fileModificationTime(filename) > theTime);
	}
	void filedelete(const std::string& filename);
	void filemove(const std::string& from, std::string to);
	void filecopy(const std::string& theOriginal, const std::string& theCopy);
	bool filecopy_or_create(const std::string& theOriginal, const std::string& theCopy);
		// returns true if copied, false if created (empty)
	void filecat(const std::vector<std::string>& source, const std::string& sink);
} // namespace ngi

std::string extractFileNameFromPath(const std::string& path);
std::string readFileIntoString(const std::string& filename);
void convertToUnixLineEndings(const std::string& filename);
std::string deduceApplicationDirectory(const std::string& app);
std::string currentWorkingDirectory();
void checkForValidPath(const fs::path& fileName);

std::uint64_t countLinesInFile(const std::string& filename);
std::ifstream openFileAndTest(const std::string& name);
	// Call as, e.g.: std::ifstream infile(openFileAndTest(infilename));
	// uses ifstream's move constructor, throws if the file is not open

std::string checksum(const std::string& filename);

std::vector<fs::path> listFilesInDirectory(const fs::path& directory, bool recursive = false, const std::regex& pattern = std::regex(""));
std::vector<fs::path> listDirectoriesInDirectory(const fs::path& directory, const std::regex& pattern = std::regex(""));
std::vector<fs::path> autocompleteFilesystemName(const std::string& filenamePrefix);

#endif
