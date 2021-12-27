// commandLineApplicationSupport.cpp
// Version 2021.12.25

/*
Copyright (c) 2015-2021, NeuroGadgets Inc.
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

#include "commandLineApplicationSupport.h"
#include "classLogger.h"
#include "ngiAlgorithms.h"
#include <iostream>

void logUsageErrorAndAbort(const std::string& masterLogfileName, const std::string& commandLine, const std::string& programName, const std::string& usageError, const std::string& specifiedUser)
{
	if (masterLogfileName.empty()) {
		printUsage(programName, true);
	} else {
		Logger log(masterLogfileName, "", "", commandLine, specifiedUser);
		if (!programName.empty()) printUsage(programName, false);
		log.addToLogAndExit(usageError);
	}
}

void usageExceptionHandler(const std::string& theCommandLineString, const std::string& programName, const std::string& theMasterLogfileName, const std::string& theSpecifiedUser, const std::exception& e)
{
	try {
		logUsageErrorAndAbort(theMasterLogfileName, theCommandLineString, programName, e.what(), theSpecifiedUser);
	} catch (std::exception& f) {
		std::cerr << "Cannot log usage error (" << e.what() << "): " << f.what() << std::endl;
	} catch (...) {
		std::cerr << "Cannot log usage error (" << e.what() << "): Unknown exception" << std::endl;
	}
	std::exit(1);
}

void genericExceptionHandler(Logger* logger, const std::string& theCommandLineString, const std::string& theMasterLogfileName, const std::string& theSpecifiedUser, const std::exception& e)
{
	try {
		if (logger) {
			logger->addToLogAndExit(e.what());
		} else if (!theMasterLogfileName.empty()) {
			Logger adHocLogger(theMasterLogfileName, "", "", theCommandLineString, theSpecifiedUser);
			adHocLogger.addToLogAndExit(e.what());
		}
	} catch (...) { }
	std::cerr << e.what() << std::endl;
		// if logger is nullptr and theMasterLogfileName is empty, or an exception was thrown
}

int concludingMessage(Logger* logger)
{
	if (logger) {
		const int numIssuesLogged = logger->numIssuesLogged();
		if (numIssuesLogged > 0) {
			if (numIssuesLogged == 1) {
				std::cerr << "An error or warning was";
				logger->addToLog(version() + " completed with an error or warning");
			} else {
				std::cerr << numIssuesLogged << " errors or warnings were";
				logger->addToLog(version() + " completed with " + std::to_string(numIssuesLogged) + " errors or warnings");
			}
			std::cerr << " logged in the file " << logger->itsRunLogfileName();
		} else {
			std::cout << version() << " completed successfully." << std::endl;
			const std::string& runlogName = logger->itsRunLogfileName();
			std::cout << "Please see the file ";
			if (beginsWith(runlogName, "/root/")) {
				// We seem to be in a Docker container, so it's not "/root/" from the user's perspective
				std::cout << runlogName.substr(6);
			} else {
				std::cout << runlogName;
			}
			std::cout << " for details about this run.\n";
			logger->addToLog(version() + " completed successfully.");
		}
		return logger->endLog();
	} else {
		std::cerr << version() << " did not complete successfully, and no log of this failed run could be produced." << std::endl;
	}
	return 1;
}
