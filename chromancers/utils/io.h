#pragma once

#include <iostream>
#include <magic_enum.hpp>

namespace utils::io
{
	// Simple collection of functions for logging
	enum logtype
	{
		INFO,
		WARNING,
		ERROR
	};

	template<typename ...Args>
	void log(logtype type, Args && ...args)
	{
		std::cout << magic_enum::enum_name(type) << ": ";
		(std::cout << ... << args);
		std::cout << std::endl;
	}

	// Display an information message
	template<typename ...Args>
	void info(Args && ...args)
	{
		log(logtype::INFO, args...);
	}

	// Display a warning message
	template<typename ...Args>
	void warn(Args && ...args)
	{
		log(logtype::WARNING, args...);
	}

	// Display an error message
	template<typename ...Args>
	void error(Args && ...args)
	{
		log(logtype::ERROR, args...);
	}
}