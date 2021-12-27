// classLogger.cpp
// Version 2021.12.25

/*
Copyright (c) 2013-2021, NeuroGadgets Inc.
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

#include "classLogger.h"
#include "ngiAlgorithms.h"
#include "ngiFileUtilities.h"
#include <cassert>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <pwd.h>
#include <unistd.h>

Logger::Logger(const std::string& masterLogfileName, const std::string& runLogfileName, const std::string& dataID, const std::string& commandLine, const std::string& specifiedUser) :
	startTimePoint_(std::chrono::system_clock::now()),
	masterLogfileName_(masterLogfileName),
	runLogfileName_(runLogfileName),
	lockfileName_(masterLogfileName + ".lock"), // Co-located with the log
	theCommandLine_(commandLine),
	programName_(commandLine.substr(0, commandLine.find(' '))),
	dataID_(dataID),
	numErrorsLogged_(0),
	numWarningsLogged_(0),
	logLevel_(_info_), // default
	notDoneWritingLog_(true),
	firstWriteToRunLog_(false)
{
	passwd* pwdReal = getpwuid(getuid());
	if (pwdReal) {
		userName_ = pwdReal->pw_name;
	} else {
		std::cerr << "Cannot set the logfile username... aborting" << std::endl;;
	}
	if (!specifiedUser.empty()) {
		// Note: user X running as user Y will not be able to override userName_.
		// If the login name is the same as the effective user, we have permission to override userName_.
		// For example, a web server daemon can set the user name of the user that authenticated on its web page.
		passwd* pwdEffective = getpwuid(geteuid());
		if (pwdEffective && pwdEffective->pw_name == userName_) {
			// only if we are not running as another user, such as in a controlled environment
			userName_ = specifiedUser;
		} else {
			std::cerr << "Cannot set the logfile username as " << specifiedUser << "... aborting" << std::endl;;
		}
	}

	if (masterLogfileName_ == runLogfileName_) {
		throw std::runtime_error(runLogfileName_ + " must not be the same as " + masterLogfileName_);
	}
	if (dataID_.empty()) {
		dataID_ = "N/A";
	}
	char theHostName[256];
	gethostname(theHostName, sizeof(theHostName)); // from unistd.h
	hostName_ = theHostName;
	if (!runLogfileName.empty()) {
		firstWriteToRunLog_ = !fs::exists(runLogfileName_);
	}
	addToLog("Launched " + commandLine);
}

Logger::~Logger()
{
	if (notDoneWritingLog_) {
		warningToLog("Logger object destroyed without implicitly or explicitly calling Logger::endLog()");
		endLog(1);
	}
}

void Logger::addToLog(const std::string& comment, const bool alsoToMasterLog, const int level)
{
	if (level < logLevel_) return;
	assert(notDoneWritingLog_);
	std::lock_guard<std::mutex> lock(myMutex_);
	// Append to the log file: date, time & time zone, hostname, username, sampleID, program name, comment (separated by tabs)
	const std::chrono::time_point<std::chrono::system_clock> currentTimePoint = std::chrono::system_clock::now();
    const std::time_t currentTime = std::chrono::system_clock::to_time_t(currentTimePoint);
	bool gotAuditTrailLock = true;
	std::ostringstream ost;
	ost << std::put_time(std::localtime(&currentTime), "%Y-%m-%d%t%T %Z") << '\t' << hostName_ << '\t' << userName_ << '\t' << dataID_ << '\t' << programName_ << '\t';
		// The time format specified above is e.g. 2021-12-31 [tab] 16:00:00 EST
	if (alsoToMasterLog) {
		Lockfile lf(lockfileName_);
		gotAuditTrailLock = lf.hasLock(); // Should always be true, if not, add warnings to both logs
		// Write to the audit trail even if the lock wasn't obtained, because contention is rare and omitting entries is not an option
		std::ofstream masterLog(masterLogfileName_, std::ios::app);
		if (!masterLog.is_open()) {
			std::cerr << "Cannot open " << masterLogfileName_ << "... aborting." << std::endl;
			std::exit(1);
		}
		if (firstWriteToRunLog_) {
			masterLog << ost.str() << "Run log set to " << runLogfileName_ << std::endl;
		}
		if (!gotAuditTrailLock) {
			masterLog << ost.str() << "**WARNING** Cannot flock() " << lockfileName_ << std::endl;
		}
		masterLog << ost.str() << comment << std::endl;
	}
	if (!runLogfileName_.empty()) {
		std::ofstream logFile(runLogfileName_, std::ios::app);
		if (!logFile.is_open()) {
			std::cerr << "Cannot open " << runLogfileName_ << "... aborting." << std::endl;
			std::exit(1);
		}
		if (firstWriteToRunLog_) { // Need to write the header.
			logFile << "Date\tTime\tHost Name\tUser Name\tData ID\tProgram Name\tComment" << std::endl;
			firstWriteToRunLog_ = false;
		}
		logFile << ost.str() << comment;
		if (!gotAuditTrailLock) {
			logFile << " **WARNING** Entry may not have been logged in master logfile";
		}
		logFile << std::endl;
	}
}

void Logger::debugToLog(const std::string& comment, const bool alsoToMasterLog)
{
	addToLog(comment, alsoToMasterLog, _debug_);
}

void Logger::errorToLog(const std::string& comment)
{
	addToLog("***ERROR*** " + comment, true, _error_);
	std::cerr << comment << std::endl;
	std::lock_guard<std::mutex> lock(myMutex_);
	++numErrorsLogged_;
}

void Logger::warningToLog(const std::string& comment)
{
	addToLog("**WARNING** " + comment, true, _warn_);
	std::cerr << comment << std::endl;
	std::lock_guard<std::mutex> lock(myMutex_);
	++numWarningsLogged_;
}

int Logger::endLog(int returnCode)
{
	std::ostringstream ost;
	ost << "Finished run";
	if (returnCode) ost << " with warning or error";
	// Record the elapsed time:
    const std::chrono::time_point<std::chrono::system_clock> endTimePoint = std::chrono::system_clock::now();
    int elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(endTimePoint - startTimePoint_).count();
	ost << "; Elapsed time: " << elapsed_seconds << "s = ";
	const int days = elapsed_seconds / 86400;
	elapsed_seconds -= days * 86400;
	const int hours = elapsed_seconds / 3600;
	elapsed_seconds -= hours * 3600;
	const int minutes = elapsed_seconds / 60;
	elapsed_seconds -= minutes * 60;
	ost << days << ':' << std::setfill('0') << std::setw(2) << hours << ':' << std::setw(2) << minutes << ':' << std::setw(2) << elapsed_seconds << '\n';
	addToLog(ost.str());
	notDoneWritingLog_ = false; // done logging
	return returnCode;
}

void Logger::setLogLevel(int newLevel)
{
	if (newLevel < 0 || newLevel >= _numLogLevels_) {
		throw std::runtime_error("Logger::setLogLevel(), bad log level " + std::to_string(newLevel) + " specified");
	}
	logLevel_ = newLevel;
}

std::string currentDateYYYYMMDD(const std::chrono::time_point<std::chrono::system_clock>& theTime)
{
	const std::time_t currentTime = std::chrono::system_clock::to_time_t(theTime);
	std::ostringstream ost;
	ost << std::put_time(std::localtime(&currentTime), "%Y%m%d");
	return ost.str();
}
