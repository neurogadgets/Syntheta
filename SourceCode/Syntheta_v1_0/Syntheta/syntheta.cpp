// Syntheta
// syntheta.cpp
// Version 2021.12.27

/*
Copyright (c) 2014-2021, NeuroGadgets Inc.
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

#include "classCmdLineArgParser.h"
#include "classLogger.h"
#include "classMind.h"
#include "classSelectable.h"
#include "commandLineApplicationSupport.h"
#include "ngiAlgorithms.h"
#include "ngiFileUtilities.h"
#include "SynthetaTypes.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

std::string applicationRootDirectory; // including trailing slash
// Subdirectories that must be present within a given installation directory are:
//	bin/ (containing this application)
//	Configuration/ (including the file GeneticAlgorithmConfig.json)
//	Logs/
//	Save/
//  Temp/

std::string version() { return "Syntheta v1.0"; } // for display in, e.g., the run log
int currentSaveRestoreVersion() { return 1; } // increment whenever the system is upgraded in a way that impacts the JSON file
	// Note: declared in SynthetaTypes.h, and defined here.

void printUsage(const std::string& programName, bool doExit)
{
	std::cout << "Usage:\n";
	std::cout << programName << " [ -j json_config ] [ -p server_port ] [ -a affective_display_update_frequency_in_seconds ] [ -m master_logfile_name ] [ -l logfile_name ] [ -u specified_user ]\n";
	std::cout << "Note: -j is required when upgrading to a new major version" << std::endl;
	if (doExit) std::exit(1);
}

int main(const int argc, const char* argv[])
{
	CmdLineArgParser options(argc, argv);
	if (argc == 1) { // Asking for usage
		printUsage(options.programName(), true);
	}
	const std::string theCommandLineString(commandLineArgsToString(argc, argv));
	std::string jsonConfigFileName, masterLogfileName, logfileName, specifiedUser;
	std::unique_ptr<Logger> logger;
	short serverPort = 1997; // default, may be overridden by optional command-line argument
	int displayUpdateFrequency = 0;
	try {
		const std::string applicationDirectory(currentWorkingDirectory()); // without trailing slash
		// By convention, Syntheta's root directory is one up from this:
		const std::string::size_type slashPos = applicationDirectory.rfind('/');
		if (slashPos != std::string::npos) {
			applicationRootDirectory = applicationDirectory.substr(0, slashPos + 1); // including trailing slash
		} else {
			throw std::runtime_error("Cannot deduce the application's root directory");
		}
		jsonConfigFileName = applicationRootDirectory + "Save/Syntheta_v" + std::to_string(currentSaveRestoreVersion()) + ".json";
			// default, unless -j is specified on the command line
		masterLogfileName = applicationRootDirectory + "Logs/SynthetaLog.txt";
			// default, unless -m is specified on the command line
		try {
			options.parse("-j", &jsonConfigFileName); // optional override of default
			options.parse("-p", &serverPort); // optional override of default
			options.parse("-a", &displayUpdateFrequency); // optional, for printing affective data on-screen
			options.parse("-m", &masterLogfileName); // optional override of default
			options.parse("-l", &logfileName); // optional run-specific log
			options.parse("-u", &specifiedUser); // optional: the name to use in the logfile, default is actual user
			if (options.hasExtraneousArguments()) {
				throw std::runtime_error("Extraneous arguments on command line");
			}
		} catch (std::exception& e) {
			usageExceptionHandler(theCommandLineString, options.programName(), masterLogfileName, specifiedUser, e);
		} catch (...) {
			usageExceptionHandler(theCommandLineString, options.programName(), masterLogfileName, specifiedUser, std::runtime_error("Unknown exception"));
		}
		Selectable::setGeneticAlgorithmConstants(applicationRootDirectory + "Configuration/GeneticAlgorithmConfig.json");

		logger = std::make_unique<Logger>(masterLogfileName, logfileName, "Syntheta", theCommandLineString, specifiedUser);
		logger->addToLog(version());

		std::ifstream jsonConfigFile(openFileAndTest(jsonConfigFileName));
		std::unique_ptr<Mind> syntheta = std::make_unique<Mind>(logger.get(), serverPort, jsonConfigFile);
		jsonConfigFile.close();

		syntheta->run(displayUpdateFrequency);
	} catch (std::exception& e) {
		genericExceptionHandler(logger.get(), theCommandLineString, masterLogfileName, specifiedUser, e);
	} catch (...) {
		genericExceptionHandler(logger.get(), theCommandLineString, masterLogfileName, specifiedUser, std::runtime_error("Unknown exception"));
	}
	// Report any issues that came up during the run:
	return concludingMessage(logger.get());
}
