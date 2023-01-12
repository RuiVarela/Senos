#pragma once

#include "Lang.hpp"
#include <string>
#include <sstream>
#include <vector>
#include <limits>

#include "tinyformat.h"

namespace sns
{
	enum class CaseSensitivity
	{
		CaseSensitive,
		CaseInsensitive
	};

	enum class CaseType
	{
		Uppercase,
        Lowercase,
        CamelCase
	};

	#define TRIM_DEFAULT_WHITESPACE " \t\f\v\n\r"
	enum class TrimType
	{
		TrimRight,
		TrimLeft,
		TrimBoth
	};

	template<typename OutputType, typename InputType>
	OutputType lexical_cast(InputType const& value);

	template<typename OutputType, typename InputType>
	OutputType lexical_cast(InputType const& value, OutputType const& default_value);

	typedef std::vector<std::string> StringElements;
	StringElements split(std::string const& input, std::string const& delimiter);

	std::string extractToken(std::string const& input, std::string const& delimiter, int token_number);

	void trimRight(std::string& string, std::string const& spaces = TRIM_DEFAULT_WHITESPACE);
	void trimLeft(std::string& string, std::string const& spaces = TRIM_DEFAULT_WHITESPACE);
	void trim(std::string& string, std::string const& spaces = TRIM_DEFAULT_WHITESPACE);
	std::string trim(TrimType const& trim_type, std::string string, std::string const& spaces = TRIM_DEFAULT_WHITESPACE);

	void uppercase(std::string &string);
	void lowercase(std::string &string);
    void camelcase(std::string &string, bool remove_spaces = false);
	std::string convertCase(CaseType const& case_type, std::string string);

	void replace(std::string &target, std::string const& match, std::string const& replace_string);
    std::string replaceString(std::string target, std::string const& match, std::string const& replace_string);

	bool startsWith(std::string const& string, std::string const& prefix);
	bool endsWith(std::string const& string, std::string const& sufix);
    bool containsText(std::string const& string, std::string const& text);

	bool isNumber(std::string const& string);
	bool isInteger(std::string const& string);
	bool isReal(std::string const& string);
	bool isBoolean(std::string const& string);

	bool isDigit(int character);
	bool isAlphabetic(int character);
	bool isLowercase(int character);
	bool isUppercase(int character);

	int toUppercase(int character);
	int toLowercase(int character);

	bool equalCaseInsensitive(std::string const& lhs, std::string const& rhs);
	bool equalCaseInsensitive(std::string const& lhs, char const* rhs);

    template<typename T>
    std::string separateElements(std::vector<T> const& data, std::string separator);

    template<typename... Args>
    std::string sfmt(const char* fmt, const Args&... args) 
	{
        return tfm::format(fmt, args...);
    }

	//
	// Implementation
	//
	template<typename T>
    std::string separateElements(std::vector<T> const& data, std::string separator)
	{
		std::stringstream stream;

		if (std::numeric_limits<T>::is_specialized)
		{
			stream.precision(std::numeric_limits<T>::digits10 + 1);

			if (!std::numeric_limits<T>::is_integer)
			{
				stream.setf(std::ios::showpoint | std::ios::fixed);
			}
		}

        for (size_t i = 0; i != data.size(); ++i)
		{
			if (i > 0)
			{
				stream << separator;
			}

            stream << data[i];
		}

		return stream.str();
	}

	//
	// Implementation
	//
	template<typename OutputType, typename InputType>
	OutputType lexical_cast(InputType const& value)
	{
		std::stringstream stream;
		//skip leading whitespace characters on input
		stream.unsetf(std::ios::skipws);

		//set the correct precision
		if (std::numeric_limits<OutputType>::is_specialized)
		{
			stream.precision(std::numeric_limits<OutputType>::digits10 + 1);
		}
//		else if (std::numeric_limits<InputType>::is_specialized)
//		{
//			stream.precision(std::numeric_limits<InputType>::digits10 + 1);
//		}

		stream << value;

		OutputType output;
		stream >> output;
		return output;
	}

	template<typename OutputType, typename InputType>
	OutputType lexical_cast(InputType const& value, OutputType const& default_value)
	{
		std::stringstream stream;
		//skip leading whitespace characters on input
		stream.unsetf(std::ios::skipws);

		//set the correct precision
		if (std::numeric_limits<OutputType>::is_specialized)
		{
			stream.precision(std::numeric_limits<OutputType>::digits10 + 1);
		}
//		else if (std::numeric_limits<InputType>::is_specialized)
//		{
//			stream.precision(std::numeric_limits<InputType>::digits10 + 1);
//		}

		stream << value;

		OutputType output;
		stream >> output;

		if (!stream)
		{
			output = default_value;
		}

		return output;
	}
} 


