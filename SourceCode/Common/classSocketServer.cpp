// classSocketServer.cpp
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

#include "classSocketServer.h"
#include "ngiAlgorithms.h"
#include "ngiFileUtilities.h"
#include "randomNumberGenerators.h"
#include "classObjectFactory.h"
#include <chrono>
#include <exception>
#include <fstream>
#include <thread>
#include <sys/stat.h>

char translateHex(char hex)
{ // Translate a single hex character; used by decodeURLString()
	return (hex >= 'A') ? (hex & 0xdf) - 'A' + 10 : hex - '0';
}

std::string decodeURLString(const std::string& URLstr)
{
	const std::string::size_type len = URLstr.length();
	std::string result;
	for (std::string::size_type i = 0; i < len; ++i) {
		if (URLstr[i] == '+') {
			result += ' ';
		} else if (URLstr[i] == '%') {
			result += translateHex(URLstr[i + 1]) * 16 + translateHex(URLstr[i + 2]);
			i += 2; // Move past hex code
		} else { // An ordinary character
			result += URLstr[i];
		}
	}
	return result;
}

typedef std::map<std::string, std::string> CGImap;

CGImap parseCGImap(const std::string& query)
{
	const std::string::size_type queryLen = query.length();
	CGImap cgiMap;
	std::string::size_type index = 0;
	while (index < queryLen) {
		std::string::size_type equalSignPos = query.find('=', index);
		if (equalSignPos == std::string::npos) break;
		const std::string name(decodeURLString(query.substr(index, equalSignPos++ - index)));
		index = query.find('&', equalSignPos);
		cgiMap[name] = decodeURLString(query.substr(equalSignPos, index - equalSignPos));
		if (index != std::string::npos) {
			++index;
		}
	}
	return cgiMap;
}

typedef std::shared_ptr<boost::asio::ip::tcp::socket> Socket_ptr;

void sendString(const Socket_ptr& sock, const std::string& s)
{
	const std::string& ns = (s.back() == '\n' ? s : s + '\n'); // Ensure it ends in '\n'
	boost::asio::write(*sock, boost::asio::buffer(ns.c_str(), ns.length()));
}


std::set<short> SocketServer::usedPorts = std::set<short>();

SocketServer::SocketServer(Logger* theLogger, short port, const std::string& htmlHeaderFooterFileName, const bool isHTTPS, const std::string& cmdArgSeparatorTag, const std::string& outResultSeparatorTag, const std::string& inputFieldSeparatorTag, const std::string& webCommandStr) :
	theLogger_(theLogger),
	commandFieldSeparator_(cmdArgSeparatorTag),
	outputFieldSeparator_(outResultSeparatorTag),
	inputFieldSeparator_(inputFieldSeparatorTag),
	webCommandString_(webCommandStr),
	httpType_(isHTTPS ? "https://" : "http://"),
	myURL_(httpType_ + theLogger->theHostName()), // Must be resolvable by the client
	portString_(std::to_string(port)),
	port_(port),
	listening_(true),
	supportsWebRequests_(!htmlHeaderFooterFileName.empty() && htmlHeaderFooterFileName.find("N/A") != 0)
{
	if (!usedPorts.insert(port).second) {
		throw std::runtime_error("SocketServer::SocketServer(): a server listening on port " + portString_ + " was already instantiated.");
	}
	theLogger_->addToLog("SocketServer instantiated with port " + portString_);
	
	if (supportsWebRequests_) {
		// Prepare the HTML header and the HTML footer:
		std::ifstream htmlHeaderFooterFile(openFileAndTest(htmlHeaderFooterFileName));
		const std::string htmlHeaderFooter((std::istreambuf_iterator<char>(htmlHeaderFooterFile)), std::istreambuf_iterator<char>());
		htmlHeaderFooterFile.close();
		const std::string::size_type headerFooterSeparator = htmlHeaderFooter.find("__HFSEPARATOR__");
		if (headerFooterSeparator == std::string::npos) {
			throw std::runtime_error("SocketServer::SocketServer(): bad format in the HTML header/footer file, " + htmlHeaderFooterFileName);
		}
		htmlHeader_ = htmlHeaderFooter.substr(0, headerFooterSeparator);
		htmlFooter_ = htmlHeaderFooter.substr(headerFooterSeparator + 15);
		theLogger_->addToLog("SocketServer has extracted HTML header/footer information from the file " + htmlHeaderFooterFileName);
	} else {
		theLogger_->addToLog("SocketServer has not been configured to accept web requests.");
	}
	
	// Create a tmp folder if it does not already exist:
	if (!fs::exists("tmp")) {
		mkdir("tmp", ngi::rwxrx);
		theLogger_->addToLog("SocketServer has created a tmp directory within the current working directory");
	}
}

SocketServer::~SocketServer()
{
	try { // Try to shut the server down cleanly
		stopAcceptingConnections();
		// Close any active sessions that were spawned off:
		for (auto& it : mySockets_) {
			Socket_ptr s(it.lock());
			if (s) {
				s->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				s->close();
			}
		}
		usedPorts.erase(port_);
	} catch (std::exception& e) {
		try {
			theLogger_->warningToLog(std::string("SocketServer::~SocketServer(): ") + e.what());
		} catch (...) { }
	} catch (...) {
		try {
			theLogger_->warningToLog("SocketServer::~SocketServer(): unknown error");
		} catch (...) { }
	}
}

void SocketServer::stopAcceptingConnections()
{
	if (listening_) { // if the server() function is still running
		listening_ = false;
		// Wake up the server so that it notices that listening_ is now false:
		try {
			using boost::asio::ip::tcp;
			tcp::resolver resolver_(io_service_);
			tcp::resolver::query q(tcp::v4(), boost::asio::ip::host_name(), portString_);
			tcp::resolver::iterator i(resolver_.resolve(q));
			tcp::socket socket(io_service_);
			boost::asio::connect(socket, i);
		} catch (std::exception& e) {
			try {
				theLogger_->warningToLog(std::string("SocketServer::stopAcceptingConnections(): ") + e.what());
			} catch (...) { }
		} catch (...) {
			try {
				theLogger_->warningToLog("SocketServer::stopAcceptingConnections(): unknown error");
			} catch (...) { }
		}
		// Wait for the server() function to complete:
		std::lock_guard<std::mutex> lock(myServerActiveMutex_);
	}
}

void SocketServer::launchServer(SocketServer::Sync isBlocking)
{
	std::thread t(&SocketServer::server, this);
	if (isBlocking == Sync::blocking) { // single-purpose server application
		t.join(); // run forever
	} else { // application that includes a server
		t.detach(); // run for the lifetime of SocketServer
	}
}

void SocketServer::cleanUpExpiredSockets()
{
	std::list<std::weak_ptr<boost::asio::ip::tcp::socket>>::iterator it = mySockets_.begin();
	while (it != mySockets_.end()) {
		if (it->expired()) {
			it = mySockets_.erase(it);
		} else {
			++it;
		}
	}
}

std::string SocketServer::insertOutputFieldSeparator(std::string prefix, const std::string& s)
{
	return prefix + outputFieldSeparator_ + s;
}

void SocketServer::server()
{
	try {
		boost::asio::ip::tcp::acceptor a(io_service_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_));
		theLogger_->addToLog("SocketServer is listening on port " + portString_);
		std::lock_guard<std::mutex> lock(myServerActiveMutex_);
		for (;;) {
			cleanUpExpiredSockets();
			Socket_ptr sock(std::make_shared<boost::asio::ip::tcp::socket>(io_service_));
			mySockets_.push_back(sock); // list<weak_ptr<socket>>
			a.accept(*sock); // blocks until a client connects to the port
			if (!listening_) break;
			theLogger_->addToLog("SocketServer accepted a connection from " + sock->remote_endpoint().address().to_string() + " on port " + portString_);
			// Spawn a session on a new thread and resume listening:
			std::thread t(&SocketServer::session, this, sock);
			t.detach();
		}
	} catch (std::exception& e) {
		try {
			theLogger_->errorToLog(std::string("SocketServer::server(): ") + e.what());
		} catch (...) { }
	} catch (...) {
		try {
			theLogger_->errorToLog("SocketServer::server(): unknown error");
		} catch (...) { }
	}
	listening_ = false; // explicit, in case there were exceptions
	theLogger_->addToLog("SocketServer is no longer listening on port " + portString_);
}

void SocketServer::session(Socket_ptr sock)
{
	// Note: for non-POST requests, the first requests need to be about authentication, to verify that the client user is the server user.
	std::string sessionAuthorizationFile, sessionAuthorizationStr;
	std::uint64_t authenticationStep = 0;
	while (listening_) {
		std::string theString, command;
		bool isPOST = false;
		try {
			boost::system::error_code error;
			boost::asio::streambuf b;
			boost::asio::read_until(*sock, b, '\n', error);
			if (error == boost::asio::error::eof) {
				break; // the client disconnected cleanly
			} else if (error) {
				throw boost::system::system_error(error);
			}
			std::istream is(&b);
			std::getline(is, theString);
			const std::string::size_type p = theString.find(commandFieldSeparator_); // e.g. __+__
			// Two scenarios: a POST from a web form, or a structured command__+__argument string
			if (p != std::string::npos) {
				command = theString.substr(0, p);
				if (command == "AuthStep1") { // The client is attempting to reconnect
					authenticationStep = 0;
				}
				switch (++authenticationStep) {
					case 1:
						if (command != "AuthStep1") {
							throw std::runtime_error("Client at " + sock->remote_endpoint().address().to_string() + " did not authenticate");
						} else {
							sessionAuthorizationFile = "tmp/auth_" + RandNum::generateRandomAlphanumericString(10, 16); // arbitrary file name length
							sessionAuthorizationStr = RandNum::generateRandomAlphanumericString(64, 128); // arbitrary length
							std::ofstream authFile(sessionAuthorizationFile);
							authFile << sessionAuthorizationStr << std::endl;
							authFile.close();
							if (chmod(sessionAuthorizationFile.c_str(), ngi::rw)) {
								throw std::runtime_error("Cannot set user-specific access for AuthStep1 authentication file");
							} // To be able to read this file, the SocketClient must be this same user.
							// The client needs the path to the file:
							auto result = pipe_to_string("pwd");
							if (result.second) {
								throw std::runtime_error("Cannot determine the current working directory");
							}
							if (result.first.back() == '\n') result.first.pop_back();
							sendString(sock, insertOutputFieldSeparator(command, result.first + '/' + sessionAuthorizationFile));
						}
						break;
					case 2:
						if (theString != "AuthStep2" + commandFieldSeparator_ + sessionAuthorizationStr) {
							throw std::runtime_error("Client at " + sock->remote_endpoint().address().to_string() + " did not send the secret string");
						} else {
							std::remove(sessionAuthorizationFile.c_str());
							sessionAuthorizationFile.clear();
							sendString(sock, insertOutputFieldSeparator(command, "ok"));
						}
						break;
					default:
						// The client has authenticated, so proceed with commands:
						sendString(sock, insertOutputFieldSeparator(command, FunctionRegistry<std::string, const std::string&>::Instance()(command, theString.substr(p + commandFieldSeparator_.length())))); // argument substring
				}
			} else if (theString.find("POST /") != std::string::npos) {
				isPOST = true;
				if (!supportsWebRequests_) {
					throw std::runtime_error("SocketServer was not configured to accept web requests!");
				}
				boost::asio::read_until(*sock, b, "Submit+This+Form", error);
				if (error == boost::asio::error::eof) {
					break; // the client disconnected cleanly
				} else if (error) {
					throw boost::system::system_error(error);
				}
				// Parse out the argument string:
				while (is && theString.find("Submit+This+Form") == std::string::npos) {
					std::getline(is, theString);
				}
				if (!is) {
					throw std::runtime_error("Error parsing POST query string");
				}
				const CGImap cgim(parseCGImap(theString));
				// Process web requests using the set of key-value pairs where value = cgim[key]
				auto webCommand = cgim.find(webCommandString_);
				if (webCommand == cgim.cend()) {
					throw std::runtime_error("The key \"" + webCommandString_ + "\" was not found within the POST string: \"" + theString + "\"");
				}
				sendString(sock, applyHTMLFormatting(FunctionRegistry<std::string, const CGImap&>::Instance()(webCommand->second, cgim), webCommand->second));
				break; // done with the POST request
			} else {
				throw std::runtime_error("Neither the field separator \"" + commandFieldSeparator_ + "\", nor \"POST /\", were found within: \"" + theString + "\"");
			}
		} catch (std::exception& e) {
			try {
				const std::string errMsg(std::string("SocketServer::session(): ") + e.what());
				theLogger_->errorToLog(errMsg);
				if (isPOST) {
					sendString(sock, applyHTMLFormatting(errMsg, "ERROR"));
					break; // done with the POST request
				} else if (!command.empty()) {
					sendString(sock, insertOutputFieldSeparator(command, std::string("Error: ") + errMsg));
				} // else no reply
			} catch (...) {
				break; // terminate the session
			}
		} catch (...) {
			try {
				const std::string errMsg("SocketServer::session(): unknown error, with string \"" + theString + "\"");
				theLogger_->errorToLog(errMsg);
				if (isPOST) {
					sendString(sock, applyHTMLFormatting(errMsg, "ERROR"));
					break; // done with the POST request
				} else if (!command.empty()) {
					sendString(sock, insertOutputFieldSeparator(command, std::string("Error: ") + errMsg));
				} // else no reply
			} catch (...) {
				break; // terminate the session
			}
		}
	}
	if (!sessionAuthorizationFile.empty()) { // in case of an exception
		std::remove(sessionAuthorizationFile.c_str());
	}
}

std::string SocketServer::applyHTMLFormatting(const std::string& s, const std::string& title)
{
	// Create the response page, starting with the HTML header:
	std::string page;
	if (supportsWebRequests_) {
		page = htmlHeader_;
	} else { // Minimal HTML5 header:
		page = "<!DOCTYPE html>\n";
		page += "<html lang=\"en\">\n";
		page += "<meta charset=\"utf-8\">\n";
		page += "<title>" + title + "</title>\n";
		page += "<body>\n";
	}
	// Add the dynamic page contents, or the error message:
	page += s;
	// Add the HTML footer information:
	if (supportsWebRequests_) {
		page += htmlFooter_;
	} else { // Minimal footer:
		page += "</body>\n";
		page += "</html>\n";
	}
	return page;
}
