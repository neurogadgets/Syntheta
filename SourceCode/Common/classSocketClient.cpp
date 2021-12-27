// classSocketClient.cpp
// Version 2021.12.24

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

#include "classSocketClient.h"
#include "ngiFileUtilities.h"
#include <exception>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

bool SocketClient::connect(const std::string& hostname, const std::string& port)
{
	using boost::asio::ip::tcp;
	try {
		tcp::resolver resolver_(io_service_);
		tcp::resolver::query q(tcp::v4(), hostname, port);
		tcp::resolver::iterator i(resolver_.resolve(q));
		boost::asio::connect(socket_, i);
        hostname_ = hostname;
        portString_ = port;
		isLocalHost_ = (hostname == "localhost");
		// We need to authenticate by proving that we can retrieve a file from the server that is only
		// readable by the user owning that file:
		const std::string remoteFileToRetrieve(retrieveString("AuthStep1", "please"));
		const std::string::size_type slashPos = remoteFileToRetrieve.find_last_of('/');
		if (slashPos == std::string::npos) {
			throw std::runtime_error("SocketClient::connect(), bad authorization command retrieved from server");
		}
		const std::string auth1filename(remoteFileToRetrieve.substr(slashPos + 1));
		retrieveFiles(std::vector<std::string>{auth1filename}, remoteFileToRetrieve.substr(0, slashPos), "./");
		std::ifstream authFile(auth1filename);
		if (!authFile.is_open()) {
			throw std::runtime_error("SocketClient::connect(), could not authorize connection");
		}
		std::string authString;
		authFile >> authString;
		authFile.close();
		std::remove(auth1filename.c_str());
		if (retrieveString("AuthStep2", authString) != "ok") {
			throw std::runtime_error("SocketClient::connect(), could not authorize connection.");
		}
		isConnected_ = true;
	} catch (std::exception& e) {
		disconnect();
		throw;
	} catch (...) {
		disconnect();
		throw;
	}
	return isConnected_;
}

void SocketClient::disconnect()
{
	socket_.close();
	isConnected_ = false;
}

void SocketClient::sendFiles(const std::vector<std::string>& localFileNames, std::string localSourceFolder, std::string remoteDestinationFolder)
{ // Requires this user to have read access to the local file paths and write access to the destination folder
	assert(!localFileNames.empty());
	if (localSourceFolder.back() != '/') localSourceFolder += '/';
	if (isLocalHost_) { // cp, requires that the user can write to remoteDestinationFolder
		if (remoteDestinationFolder.back() != '/') remoteDestinationFolder += '/';
		for (const auto& f : localFileNames) {
			ngi::filecopy(localSourceFolder + f, remoteDestinationFolder + f);
		}
	} else { // scp, requires that this user can authenticate to the remote host
		std::string filesToCopy(localSourceFolder);
		if (localFileNames.size() > 1) filesToCopy += "\\{";
		for (const auto& f : localFileNames) {
			filesToCopy += f;
			filesToCopy.push_back(',');
		}
		filesToCopy.pop_back(); // terminal ','
		if (localFileNames.size() > 1) filesToCopy += "\\}";
		const int scpExitCode = pipe_to_string("scp -q " + filesToCopy + ' ' + hostname_ + ':' + remoteDestinationFolder).second;
		if (scpExitCode) {
			throw std::runtime_error("SocketClient::sendFiles(), scp error code: " + std::to_string(scpExitCode));
		}
	}
}

void SocketClient::retrieveFiles(const std::vector<std::string>& remoteFileNames, std::string remoteSourceFolder, std::string localDestinationFolder)
{ // Requires this user to have read access to the remote file paths and write access to the local folder
	assert(!remoteSourceFolder.empty());
	if (remoteSourceFolder.back() != '/') remoteSourceFolder += '/';
	if (isLocalHost_) { // cp, requires that the user can read the remoteFileNames
		if (localDestinationFolder.back() != '/') localDestinationFolder += '/';
		for (const auto& f : remoteFileNames) {
			ngi::filecopy(remoteSourceFolder + f, localDestinationFolder + f);
		}
	} else { // scp, requires that this user can authenticate to the remote host
		std::string filesToCopy(remoteSourceFolder);
		if (remoteFileNames.size() > 1) filesToCopy += "\\{";
		for (const auto& f : remoteFileNames) {
			filesToCopy += f;
			filesToCopy.push_back(',');
		}
		filesToCopy.pop_back(); // terminal ','
		if (remoteFileNames.size() > 1) filesToCopy += "\\}";
		const int scpExitCode = pipe_to_string("scp -q " + hostname_ + ':' + filesToCopy + ' ' + localDestinationFolder).second;
		if (scpExitCode) {
			throw std::runtime_error("SocketClient::retrieveFiles(), scp error code: " + std::to_string(scpExitCode));
		}
	}
}


void SocketClient::sendCommandAndString(const std::string& command, const std::string& argument)
{
	const std::string cs(command + commandFieldSeparator_ + argument + '\n');
    boost::asio::write(socket_, boost::asio::buffer(cs.c_str(), cs.length()));
}

std::string SocketClient::receiveString()
{
	boost::asio::streambuf b;
    boost::asio::read_until(socket_, b, '\n');
	std::istream is(&b);
	std::string theString;
	std::getline(is, theString);
	if (theString.find("Error:") != std::string::npos) { // Assumes that SocketServer recognizes this specific "Error:" string
		throw std::runtime_error(theString);
	}
	return theString;
}

std::string SocketClient::receiveString(const std::string& tag)
{
	const std::string receivedString(receiveString());
	const std::string::size_type p = receivedString.find(receiveStringFieldSeparator_);
	if (p == std::string::npos || receivedString.substr(0, p) != tag) {
		throw std::runtime_error("Error: SocketClient::receiveString(" + tag + ") returned " + receivedString);
	}
	return receivedString.substr(p + receiveStringFieldSeparator_.length());
}

std::string SocketClient::retrieveString(const std::string& command, const std::string& argument, const std::string& expectedResponse)
{
	sendCommandAndString(command, argument);
	const std::string response(receiveString(command));
	if (!expectedResponse.empty() && response != expectedResponse) {
		throw std::runtime_error("Request \"" + command + ' ' + argument + "\" returned: " + response);
	}
	return response;
}


void checkResponseIsOK(const std::string& request, const std::string& response, const std::string& expectedResponse)
{
	if (response != expectedResponse) {
		throw std::runtime_error("Request \"" + request + "\" returned \"" + response + '\"');
	}
}
