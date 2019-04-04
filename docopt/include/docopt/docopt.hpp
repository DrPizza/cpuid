// from https://github.com/docopt/docopt.cpp, dual licensed under MIT and Boost license

//
//  docopt.h
//  docopt
//
//  Created by Jared Grubb on 2013-11-03.
//  Copyright (c) 2013 Jared Grubb. All rights reserved.
//

#ifndef docopt__docopt_h_
#define docopt__docopt_h_

#include <map>
#include <vector>
#include <string>
#include <variant>
#include <iosfwd>
#include <stdexcept>

#ifdef DOCOPT_HEADER_ONLY
    #define DOCOPT_INLINE inline
    #define DOCOPT_API
#else 
    #define DOCOPT_INLINE

    // With Microsoft Visual Studio, export certain symbols so they 
    // are available to users of docopt.dll (shared library). The DOCOPT_DLL
    // macro should be defined if building a DLL (with Visual Studio),
    // and by clients using the DLL. The CMakeLists.txt and the
    // docopt-config.cmake it generates handle this.
    #ifdef DOCOPT_DLL
        // Whoever is *building* the DLL should define DOCOPT_EXPORTS.
        // The CMakeLists.txt that comes with docopt does this.
        // Clients of docopt.dll should NOT define DOCOPT_EXPORTS.
        #ifdef DOCOPT_EXPORTS
            #define DOCOPT_API __declspec(dllexport)
        #else
            #define DOCOPT_API __declspec(dllimport)
        #endif
    #else
        #define DOCOPT_API
    #endif
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 26447) // warning C26447: The function is declared 'noexecpt' but calls function '%s' which may throw exceptions (f.6).
#endif

namespace docopt {
	
	// Usage string could not be parsed (ie, the developer did something wrong)
	struct language_error : std::logic_error
	{
		using logic_error::logic_error;
	};
	
	// Arguments passed by user were incorrect (ie, developer was good, user is wrong)
	struct argument_error : std::runtime_error
	{
		using runtime_error::runtime_error;
	};
	
	// Arguments contained '--help' and parsing was aborted early
	struct exit_help : std::runtime_error
	{
		exit_help() noexcept : std::runtime_error("docopt --help argument encountered") {
		}
	};

	// Arguments contained '--version' and parsing was aborted early
	struct exit_version : std::runtime_error
	{
		exit_version() noexcept : std::runtime_error("docopt --version argument encountered") {
		}
	};
	
	/// A generic type to hold the various types that can be produced by docopt.
	///
	/// This type can be one of: {bool, unsigned long long, string, vector<string>}, or empty.

	using value = std::variant<std::monostate, bool, std::string, unsigned long long, std::vector<std::string> >;

	/// Parse user options from the given option string.
	///
	/// @param doc   The usage string
	/// @param argv  The user-supplied arguments
	/// @param help  Whether to end early if '-h' or '--help' is in the argv
	/// @param version Whether to end early if '--version' is in the argv
	/// @param options_first  Whether options must precede all args (true), or if args and options
	///                can be arbitrarily mixed.
	///
	/// @throws language_error if the doc usage string had errors itself
	/// @throws exit_help if 'help' is true and the user has passed the '--help' argument
	/// @throws exit_version if 'version' is true and the user has passed the '--version' argument
	/// @throws argument_error if the user's argv did not match the usage patterns
	std::map<std::string, docopt::value> DOCOPT_API docopt_parse(std::string const& doc,
	                                                             std::vector<std::string> const& argv,
	                                                             bool help = true,
	                                                             bool version = true,
	                                                             bool options_first = false);
	
	/// Parse user options from the given string, and exit appropriately
	///
	/// Calls 'docopt_parse' and will terminate the program if any of the exceptions above occur:
	///  * language_error - print error and terminate (with exit code -1)
	///  * exit_help - print usage string and terminate (with exit code 0)
	///  * exit_version - print version and terminate (with exit code 0)
	///  * argument_error - print error and usage string and terminate (with exit code -1)
	std::map<std::string, docopt::value> DOCOPT_API docopt(std::string const& doc,
	                                                       std::vector<std::string> const& argv,
	                                                       bool help = true,
	                                                       std::string const& version = {},
	                                                       bool options_first = false);

	std::vector<std::string> DOCOPT_API crack_argv(const std::string& command_line);
	
	namespace
	{
		template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
		template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

		inline std::string escape_string(const std::string& input) {
			std::string output;
			for(const char ch : input) {
				switch(ch) {
				case '\\':
				case '"':
					output += '\\';
					output += ch;
					break;
				default:
					output += ch;
					break;
				}
			}
			return output;
		}
	}

	inline static const auto value_printer = overloaded{
		[] (std::monostate)                      -> std::string { return "null"; },
		[] (bool arg)                            -> std::string { return arg ? "true" : "false"; },
		[] (const std::string& s)                -> std::string { return "\"" + escape_string(s) + "\""; },
		[] (unsigned long long v)                -> std::string { return std::to_string(v); },
		[] (const std::vector<std::string>& vec) -> std::string {
			std::string rv("[");
			bool first = true;
			for(const std::string& s : vec) {
				if(first) {
					first = false;
				} else {
					rv += ", ";
				}
				rv += "\"" + escape_string(s) + "\"";
			}
			return rv + "]"; 
		}
	};
}

namespace std {
	inline std::ostream& operator<<(std::ostream& os, const docopt::value& val) {
		return os << std::visit(docopt::value_printer, val);
	}
}
#ifdef DOCOPT_HEADER_ONLY
    #include "docopt.cpp"
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif /* defined(docopt__docopt_h_) */
