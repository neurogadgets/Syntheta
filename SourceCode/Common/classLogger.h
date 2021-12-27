// classLogger.h
// Version 2016.10.19

/*
Copyright (c) 2013-2016, NeuroGadgets Inc.
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

// This class provides logging / audit trail services.

#ifndef CLASS_LOGGER_H
#define CLASS_LOGGER_H

#include <chrono>
#include <cstdlib>
#include <mutex>
#include <string>

std::string currentDateYYYYMMDD(const std::chrono::time_point<std::chrono::system_clock>& theTime = std::chrono::system_clock::now());

class Logger {
private:
    std::chrono::time_point<std::chrono::system_clock> startTimePoint_;
	std::mutex myMutex_;
	std::string masterLogfileName_; // assumed to be shared
	std::string runLogfileName_; // assumed to be unique
	std::string lockfileName_; // assumed to be shared
	std::string theCommandLine_;
	std::string programName_;
	std::string userName_;
	std::string hostName_;
	std::string dataID_;
	int numErrorsLogged_;
	int numWarningsLogged_;
	int logLevel_;
	bool notDoneWritingLog_;
	bool firstWriteToRunLog_;
public:
	enum { _debug_, _info_, _warn_, _error_, _numLogLevels_ };
	Logger(const std::string& masterLogfileName, const std::string& runLogfileName, const std::string& dataID, const std::string& commandLine, const std::string& specifiedUser = std::string());
	~Logger();

	const std::string& itsMasterLogfileName() const {
		return masterLogfileName_;
	}
	const std::string& itsRunLogfileName() const {
		return runLogfileName_.empty() ? masterLogfileName_ : runLogfileName_;
	}
	int numErrorsLogged() {
		std::lock_guard<std::mutex> lock(myMutex_);
		return numErrorsLogged_;
	}
	int numWarningsLogged() {
		std::lock_guard<std::mutex> lock(myMutex_);
		return numWarningsLogged_;
	}
	int numIssuesLogged() {
		std::lock_guard<std::mutex> lock(myMutex_);
		return numErrorsLogged_ + numWarningsLogged_;
	}
	
	int getLogLevel() const { return logLevel_; }
	void setLogLevel(int newLevel);
	
	std::string startDateYYYYMMDD() const { return currentDateYYYYMMDD(startTimePoint_); }
	const std::string& theHostName() const { return hostName_; }
	const std::string& getDataID() const { return dataID_; }

	void debugToLog(const std::string& comment, const bool alsoToMasterLog = true);
	void addToLog(const std::string& comment, const bool alsoToMasterLog = true, const int level = _info_);
	void errorToLog(const std::string& comment);
	void warningToLog(const std::string& comment);

	int endLog(int returnCode = 0); // completes the log
	void exitLog(int returnCode = 1) {
		endLog(returnCode);
		std::exit(returnCode);
	} // convenience function
	void addToLogAndExit(const std::string& comment, int returnCode = 1) {
		errorToLog(comment);
		exitLog(returnCode);
	} // convenience function
	void exitWithWarning(const std::string& comment, int returnCode = 1) {
		warningToLog(comment);
		exitLog(returnCode);
	} // convenience function
};

#endif
