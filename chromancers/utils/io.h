#pragma once

#include <iostream>
#include <magic_enum.hpp>

namespace utils::io
{
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

	template<typename ...Args>
	void info(Args && ...args)
	{
		log(logtype::INFO, args...);
	}

	template<typename ...Args>
	void warn(Args && ...args)
	{
		log(logtype::WARNING, args...);
	}

	template<typename ...Args>
	void error(Args && ...args)
	{
		log(logtype::ERROR, args...);
	}
}