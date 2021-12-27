// classObjectFactory.h
// Version 2021.11.27

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

// Generic object factory templates taking any number of arguments. See the bottom of the file for usage examples.

#ifndef CLASS_GENERIC_OBJECT_FACTORY_H
#define CLASS_GENERIC_OBJECT_FACTORY_H

#include <functional>
#include <memory>
#include <map>
#include <string>

template<class T, typename... Args> class ObjectFactory {
private:
	std::map<std::string, std::function<std::unique_ptr<T>(Args&...)>> dispatch_;

	ObjectFactory() { }
public:
	ObjectFactory(const ObjectFactory&) = delete;
	ObjectFactory& operator=(const ObjectFactory&) = delete;

	static ObjectFactory& Instance() {
		static ObjectFactory factory;
		return factory;
	}
	bool Register(const std::string& key, std::function<std::unique_ptr<T>(Args&...)> f) {
		return dispatch_.emplace(key, f).second; // once
	}
	std::unique_ptr<T> CreateObject(const std::string& key, Args&... args) {
		const auto& it = dispatch_.find(key);
		if (it != dispatch_.end()) {
			return it->second(args...);
		} else {
			throw std::runtime_error("ObjectFactory::CreateObject(), Error: object with key " + key + " not yet registered.");
		}
	}
};

template<class T, typename... Args> class FunctionRegistry {
private:
	std::map<std::string, std::function<T(Args&...)>> dispatch_;

	FunctionRegistry() { }
public:
	FunctionRegistry(const FunctionRegistry&) = delete;
	FunctionRegistry& operator=(const FunctionRegistry&) = delete;

	static FunctionRegistry& Instance() {
		static FunctionRegistry registry;
		return registry;
	}
	bool Register(const std::string& key, std::function<T(Args&...)> f) {
		return dispatch_.emplace(key, f).second; // once
	}
	T operator()(const std::string& key, Args&... args) {
		const auto& it = dispatch_.find(key);
		if (it != dispatch_.end()) {
			return it->second(args...);
		} else {
			throw std::runtime_error("FunctionRegistry::operator(), Error: function with key " + key + " not yet registered.");
		}
	}
};

/*
To register a creator function for a derived class object, add the following to the source (.cpp) file for the derived class, e.g.:

namespace {
	std::unique_ptr<BaseClass> CreateDC(int i, float f) { return std::make_unique<DerivedClass>(i, f); }
	const bool registeredDC = ObjectFactory<BaseClass, int, float>::Instance().Register("DC", CreateDC);
}

To create an object using the factory, do, e.g.:
auto dc = ObjectFactory<BaseClass, int, float>::Instance().CreateObject("DC", 42, 3.14159F);


To register a function, add the following to the source (.cpp) file defining the function, e.g.:

namespace {
	const bool registeredFn = FunctionRegistry<std::string, const std::string&>::Instance().Register("myFn", theFn);
}

std::string theFn(const std::string& arg)
{
	return "Hello " + arg;
}

To use the function, e.g.
std::cout << FunctionRegistry<std::string, const std::string&>::Instance()("myFn", "world") << std::endl;
*/

#endif
