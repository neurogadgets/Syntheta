// classSocketServer.h
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

#ifndef CLASS_REMOTE_DAEMON_SERVER_H
#define CLASS_REMOTE_DAEMON_SERVER_H

#include "classLogger.h"
#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <boost/asio.hpp>

class Logger;

class SocketServer {
private:
	static std::set<short> usedPorts;
		// allows multiple servers to coexist within an application, if they listen on different ports
		// Note: this does not currently verify that those ports are unused system-wide! ###

	boost::asio::io_service io_service_;
	std::list<std::weak_ptr<boost::asio::ip::tcp::socket>> mySockets_;
	std::mutex myServerActiveMutex_;
	Logger* theLogger_; // non-owning pointer
	std::string htmlHeader_;
	std::string htmlFooter_;
	std::string commandFieldSeparator_;
	std::string outputFieldSeparator_;
	std::string inputFieldSeparator_;
	std::string webCommandString_;
	std::string httpType_; // "http://" or "https://"
	std::string myURL_;
	std::string portString_;
	short port_;
	std::atomic<bool> listening_;
	bool supportsWebRequests_;
	
	void server();
	void session(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
	void cleanUpExpiredSockets();
	void stopAcceptingConnections();
	std::string applyHTMLFormatting(const std::string& s, const std::string& title);
	std::string insertOutputFieldSeparator(std::string prefix, const std::string& s);
public:
	enum class Sync { blocking, non_blocking };

	SocketServer(Logger* theLogger, short port, const std::string& htmlHeaderFooterFileName, const bool isHTTPS = false, const std::string& cmdArgSeparatorTag = "__+__", const std::string& outResultSeparatorTag = "__$__", const std::string& inputFieldSeparatorTag = "__*__", const std::string& webCommandStr = "WebCommand");
	SocketServer(const SocketServer&) = delete;
	SocketServer& operator=(const SocketServer&) = delete;
	~SocketServer();

	short itsPort() const { return port_; }
	const std::string& itsURL() const { return myURL_; }
	const std::string& itsCommandFieldSeparator() const { return commandFieldSeparator_; }
	const std::string& itsOutputFieldSeparator() const { return outputFieldSeparator_; }
	const std::string& itsInputFieldSeparator() const { return inputFieldSeparator_; }
	const std::string& httpType() const { return httpType_; }
	void launchServer(Sync isBlocking);
};

#endif
