// classSocketClient.h
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

#ifndef CLASS_REMOTE_DAEMON_CLIENT_H
#define CLASS_REMOTE_DAEMON_CLIENT_H

#include "ngiAlgorithms.h"
#include "tupleStringStreamer.h"
#include <string>
#include <boost/asio.hpp>

class SocketClient {
private:
	boost::asio::io_service io_service_;
	boost::asio::ip::tcp::socket socket_;
    std::string hostname_;
    std::string portString_;
	std::string commandFieldSeparator_;
	std::string receiveStringFieldSeparator_;
	std::string argumentFieldSeparator_;
	std::string receiveString();
	std::string receiveString(const std::string& tag);
	bool isConnected_;
	bool isLocalHost_;
public:
	SocketClient(const std::string& cmdFieldSeparator = "__+__", const std::string& receivedStrFieldSeparator = "__$__", const std::string& argFieldSeparator = "__*__") :
		socket_(io_service_),
		commandFieldSeparator_(cmdFieldSeparator),
		receiveStringFieldSeparator_(receivedStrFieldSeparator),
		argumentFieldSeparator_(argFieldSeparator),
		isConnected_(false),
		isLocalHost_(false)
		{ }
	SocketClient(const SocketClient&) = delete;
	SocketClient& operator=(const SocketClient&) = delete;
	SocketClient(SocketClient&&) = delete;
	SocketClient& operator=(SocketClient&&) = delete;

	~SocketClient() { disconnect(); }

	const std::string& itsHostName() const { return hostname_; }
	const std::string& itsPort() const { return portString_; }
	const std::string& itsArgumentFieldSeparator() const { return argumentFieldSeparator_; }
	bool isConnected() const { return isConnected_; }

	bool connect(const std::string& hostname, const std::string& port);
	bool connect(const std::pair<std::string, std::string>& p) {
		return connect(p.first, p.second);
	}
	void disconnect();
	
	void sendFiles(const std::vector<std::string>& localFileNames, std::string localSourceFolder, std::string remoteDestinationFolder);
	void retrieveFiles(const std::vector<std::string>& remoteFileNames, std::string remoteSourceFolder, std::string localDestinationFolder);

	void sendCommandAndString(const std::string& command, const std::string& argument); // No return value

	std::string retrieveString(const std::string& command, const std::string& argument, const std::string& expectedResponse = std::string()); // Can include whitespace
	
	template<typename R> R retrieveSingleValue(const std::string& command, const std::string& argument)
	{
		sendCommandAndString(command, argument);
		return extractValueFromString<R>(receiveString(command));
	} // Call as e.g. retrieveSingleValue<float>(command, argument);

	template<typename R> std::vector<R> retrieveValueVector(const std::string& command, const std::string& argument, const std::size_t expectedNumValues = 0)
	{
		sendCommandAndString(command, argument);
		std::istringstream s(receiveString(command));
		std::vector<R> values;
		if (expectedNumValues > 0) {
			values.reserve(expectedNumValues);
		}
		R datum;
		s >> datum;
		while (s) {
			values.push_back(datum);
			s >> datum;
		}
		if (expectedNumValues > 0 && values.size() != expectedNumValues) {
			throw std::runtime_error("SocketClient::retrieveValueVector(), expected " + std::to_string(expectedNumValues) + " values but instead retrieved " + std::to_string(values.size()));
		}
		return values;
	} // Call as e.g. retrieveValueVector<double>(command, argument, 12);
};

#endif
