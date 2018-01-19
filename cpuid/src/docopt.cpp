// from https://github.com/docopt/docopt.cpp, dual licensed under MIT and Boost license

#include "stdafx.h"

//
//  docopt.cpp
//  docopt
//
//  Created by Jared Grubb on 2013-11-03.
//  Copyright (c) 2013 Jared Grubb. All rights reserved.
//

#include "docopt.h"
#include "docopt_util.h"
#include "docopt_private.h"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <string>
#include <iostream>
#include <cassert>
#include <cstddef>

#pragma region Parsing stuff
namespace {
	
	struct Tokens {
		Tokens(std::vector<std::string> tokens, bool isParsingArgv = true) noexcept
			: fTokens(std::move(tokens)),
			fIsParsingArgv(isParsingArgv)
		{}

		explicit operator bool() const noexcept {
			return fIndex < fTokens.size();
		}

		static Tokens from_pattern(std::string const& source) {
			static const docopt::regex re_separators{
				"(?:\\s*)" // any spaces (non-matching subgroup)
				"("
				"[\\[\\]\\(\\)\\|]" // one character of brackets or parens or pipe character
				"|"
				"\\.\\.\\."  // elipsis
				")" };

			static const docopt::regex re_strings{
				"(?:\\s*)" // any spaces (non-matching subgroup)
				"("
				"\\S*<.*?>"  // strings, but make sure to keep "< >" strings together
				"|"
				"[^<>\\s]+"     // string without <>
				")" };

			// We do two stages of regex matching. The '[]()' and '...' are strong delimeters
			// and need to be split out anywhere they occur (even at the end of a token). We
			// first split on those, and then parse the stuff between them to find the string
			// tokens. This is a little harder than the python version, since they have regex.split
			// and we dont have anything like that.

			std::vector<std::string> tokens;
			std::for_each(docopt::sregex_iterator{ source.begin(), source.end(), re_separators },
				docopt::sregex_iterator{},
				[&](docopt::smatch const& match)
			{
				// handle anything before the separator (this is the "stuff" between the delimeters)
				if(match.prefix().matched) {
					std::for_each(docopt::sregex_iterator{ match.prefix().first, match.prefix().second, re_strings },
						docopt::sregex_iterator{},
						[&](docopt::smatch const& m)
					{
						tokens.push_back(m[1].str());
					});
				}

				// handle the delimter token itself
				if(match[1].matched) {
					tokens.push_back(match[1].str());
				}
			});

			return Tokens(tokens, false);
		}

		std::string const& current() const {
			if(*this)
				return fTokens[fIndex];

			static std::string const empty;
			return empty;
		}

		std::string the_rest() const {
			if(!*this)
				return {};
			return join(fTokens.begin() + static_cast<std::ptrdiff_t>(fIndex),
				fTokens.end(),
				" ");
		}

		std::string pop() {
			return std::move(fTokens.at(fIndex++));
		}

		bool isParsingArgv() const noexcept { return fIsParsingArgv; }

		struct OptionError : std::runtime_error { using runtime_error::runtime_error; };

	private:
		std::vector<std::string> fTokens;
		size_t fIndex = 0;
		bool fIsParsingArgv;
	};

	// Get all instances of 'T' from the pattern
	template <typename T>
	std::vector<T*> flat_filter(docopt::Pattern& pattern) {
		std::vector<docopt::Pattern*> flattened = pattern.flat([](docopt::Pattern const* p) noexcept -> bool {
			return dynamic_cast<T const*>(p) != nullptr;
		});

		// now, we're guaranteed to have T*'s, so just use static_cast
		std::vector<T*> ret;
		std::transform(flattened.begin(), flattened.end(), std::back_inserter(ret), [](docopt::Pattern* p) noexcept {
			return static_cast<T*>(p);
		});
		return ret;
	}

	std::vector<std::string> parse_section(std::string const& name, std::string const& source) {
		// ECMAScript regex only has "?=" for a non-matching lookahead. In order to make sure we always have
		// a newline to anchor our matching, we have to avoid matching the final newline of each grouping.
		// Therefore, our regex is adjusted from the docopt Python one to use ?= to match the newlines before
		// the following lines, rather than after.
		docopt::regex const re_section_pattern{
			"(?:^|\\n)"  // anchored at a linebreak (or start of string)
			"("
			"[^\\n]*" + name + "[^\\n]*(?=\\n?)" // a line that contains the name
			"(?:\\n[ \\t].*?(?=\\n|$))*"         // followed by any number of lines that are indented
			")",
			docopt::regex::icase
		};

		std::vector<std::string> ret;
		std::for_each(docopt::sregex_iterator(source.begin(), source.end(), re_section_pattern),
			docopt::sregex_iterator(),
			[&](docopt::smatch const& match)
		{
			ret.push_back(trim(match[1].str()));
		});

		return ret;
	}

	bool is_argument_spec(std::string const& token) {
		if(token.empty())
			return false;

		if(token[0] == '<' && token[token.size() - 1] == '>')
			return true;

		if(std::all_of(token.begin(), token.end(), &::isupper))
			return true;

		return false;
	}

	template <typename I>
	std::vector<std::string> longOptions(I iter, I end) {
		std::vector<std::string> ret;
		std::transform(iter, end, std::back_inserter(ret),
			[](typename I::reference opt) { return opt->longOption(); });
		return ret;
	}

	docopt::PatternList parse_long(Tokens& tokens, std::vector<docopt::Option>& options)
	{
		// long ::= '--' chars [ ( ' ' | '=' ) chars ] ;
		std::string longOpt, equal;
		docopt::value val;
		std::tie(longOpt, equal, val) = partition(tokens.pop(), "=");

		assert(starts_with(longOpt, "--"));

		if(equal.empty()) {
			val = docopt::value{};
		}

		// detect with options match this long option
		std::vector<docopt::Option const*> similar;
		for(auto const& option : options) {
			if(option.longOption() == longOpt)
				similar.push_back(&option);
		}

		// maybe allow similar options that match by prefix
		if(tokens.isParsingArgv() && similar.empty()) {
			for(auto const& option : options) {
				if(option.longOption().empty())
					continue;
				if(starts_with(option.longOption(), longOpt))
					similar.push_back(&option);
			}
		}

		docopt::PatternList ret;

		if(similar.size() > 1) { // might be simply specified ambiguously 2+ times?
			std::vector<std::string> prefixes = longOptions(similar.begin(), similar.end());
			std::string error = "'" + longOpt + "' is not a unique prefix: ";
			error.append(join(prefixes.begin(), prefixes.end(), ", "));
			throw Tokens::OptionError(std::move(error));
		} else if(similar.empty()) {
			int argcount = equal.empty() ? 0 : 1;
			options.emplace_back("", longOpt, argcount);

			auto o = std::make_shared<docopt::Option>(options.back());
			if(tokens.isParsingArgv()) {
				o->setValue(argcount ? docopt::value{ val } : docopt::value{ true });
			}
			ret.push_back(o);
		} else {
			auto o = std::make_shared<docopt::Option>(*similar[0]);
			if(o->argCount() == 0) {
				if(!docopt::is_empty(val)) {
					std::string error = o->longOption() + " must not have an argument";
					throw Tokens::OptionError(std::move(error));
				}
			} else {
				if(docopt::is_empty(val)) {
					auto const& token = tokens.current();
					if(token.empty() || token == "--") {
						std::string error = o->longOption() + " requires an argument";
						throw Tokens::OptionError(std::move(error));
					}
					val = tokens.pop();
				}
			}
			if(tokens.isParsingArgv()) {
				o->setValue(!docopt::is_empty(val) ? std::move(val) : docopt::value{ true });
			}
			ret.push_back(o);
		}

		return ret;
	}

	docopt::PatternList parse_short(Tokens& tokens, std::vector<docopt::Option>& options)
	{
		// shorts ::= '-' ( chars )* [ [ ' ' ] chars ] ;

		auto token = tokens.pop();

		assert(starts_with(token, "-"));
		assert(!starts_with(token, "--"));

		auto i = token.begin();
		++i; // skip the leading '-'

		docopt::PatternList ret;
		while(i != token.end()) {
			std::string shortOpt = { '-', *i };
			++i;

			std::vector<docopt::Option const*> similar;
			for(auto const& option : options) {
				if(option.shortOption() == shortOpt)
					similar.push_back(&option);
			}

			if(similar.size() > 1) {
				std::string error = shortOpt + " is specified ambiguously "
					+ std::to_string(similar.size()) + " times";
				throw Tokens::OptionError(std::move(error));
			} else if(similar.empty()) {
				options.emplace_back(shortOpt, "", 0);

				auto o = std::make_shared<docopt::Option>(options.back());
				if(tokens.isParsingArgv()) {
					o->setValue(docopt::value{ true });
				}
				ret.push_back(o);
			} else {
				auto o = std::make_shared<docopt::Option>(*similar[0]);
				docopt::value val;
				if(o->argCount()) {
					if(i == token.end()) {
						// consume the next token
						auto const& ttoken = tokens.current();
						if(ttoken.empty() || ttoken == "--") {
							std::string error = shortOpt + " requires an argument";
							throw Tokens::OptionError(std::move(error));
						}
						val = tokens.pop();
					} else {
						// consume all the rest
						val = std::string{ i, token.end() };
						i = token.end();
					}
				}

				if(tokens.isParsingArgv()) {
					o->setValue(!docopt::is_empty(val) ? std::move(val) : docopt::value{ true });
				}
				ret.push_back(o);
			}
		}

		return ret;
	}

	docopt::PatternList parse_expr(Tokens& tokens, std::vector<docopt::Option>& options);

	docopt::PatternList parse_atom(Tokens& tokens, std::vector<docopt::Option>& options)
	{
		// atom ::= '(' expr ')' | '[' expr ']' | 'options'
		//             | long | shorts | argument | command ;

		std::string const& token = tokens.current();

		docopt::PatternList ret;

		if(token == "[") {
			tokens.pop();

			auto expr = parse_expr(tokens, options);

			auto trailing = tokens.pop();
			if(trailing != "]") {
				throw docopt::language_error("Mismatched '['");
			}

			ret.emplace_back(std::make_shared<docopt::Optional>(std::move(expr)));
		} else if(token == "(") {
			tokens.pop();

			auto expr = parse_expr(tokens, options);

			auto trailing = tokens.pop();
			if(trailing != ")") {
				throw docopt::language_error("Mismatched '('");
			}

			ret.emplace_back(std::make_shared<docopt::Required>(std::move(expr)));
		} else if(token == "options") {
			tokens.pop();
			ret.emplace_back(std::make_shared<docopt::OptionsShortcut>());
		} else if(starts_with(token, "--") && token != "--") {
			ret = parse_long(tokens, options);
		} else if(starts_with(token, "-") && token != "-" && token != "--") {
			ret = parse_short(tokens, options);
		} else if(is_argument_spec(token)) {
			ret.emplace_back(std::make_shared<docopt::Argument>(tokens.pop()));
		} else {
			ret.emplace_back(std::make_shared<docopt::Command>(tokens.pop()));
		}

		return ret;
	}

	docopt::PatternList parse_seq(Tokens& tokens, std::vector<docopt::Option>& options)
	{
		// seq ::= ( atom [ '...' ] )* ;"""

		docopt::PatternList ret;

		while(tokens) {
			auto const& token = tokens.current();

			if(token == "]" || token == ")" || token == "|")
				break;

			auto atom = parse_atom(tokens, options);
			if(tokens.current() == "...") {
				ret.emplace_back(std::make_shared<docopt::OneOrMore>(std::move(atom)));
				tokens.pop();
			} else {
				std::move(atom.begin(), atom.end(), std::back_inserter(ret));
			}
		}

		return ret;
	}

	std::shared_ptr<docopt::Pattern> maybe_collapse_to_required(docopt::PatternList&& seq)
	{
		if(seq.size() == 1) {
			return std::move(seq[0]);
		}
		return std::make_shared<docopt::Required>(std::move(seq));
	}

	std::shared_ptr<docopt::Pattern> maybe_collapse_to_either(docopt::PatternList&& seq)
	{
		if(seq.size() == 1) {
			return std::move(seq[0]);
		}
		return std::make_shared<docopt::Either>(std::move(seq));
	}

	docopt::PatternList parse_expr(Tokens& tokens, std::vector<docopt::Option>& options)
	{
		// expr ::= seq ( '|' seq )* ;

		auto seq = parse_seq(tokens, options);

		if(tokens.current() != "|")
			return seq;

		docopt::PatternList ret;
		ret.emplace_back(maybe_collapse_to_required(std::move(seq)));

		while(tokens.current() == "|") {
			tokens.pop();
			seq = parse_seq(tokens, options);
			ret.emplace_back(maybe_collapse_to_required(std::move(seq)));
		}

		return { maybe_collapse_to_either(std::move(ret)) };
	}

	docopt::Required parse_pattern(std::string const& source, std::vector<docopt::Option>& options)
	{
		auto tokens = Tokens::from_pattern(source);
		auto result = parse_expr(tokens, options);

		if(tokens)
			throw docopt::language_error("Unexpected ending: '" + tokens.the_rest() + "'");

		assert(result.size() == 1 && "top level is always one big");
		return docopt::Required{ std::move(result) };
	}


	std::string formal_usage(std::string const& section) {
		std::string ret = "(";

		const auto i = section.find(':') + 1;  // skip past "usage:"
		auto parts = split(section, i);
		for(size_t ii = 1; ii < parts.size(); ++ii) {
			if(parts[ii] == parts[0]) {
				ret += " ) | (";
			} else {
				ret.push_back(' ');
				ret += parts[ii];
			}
		}

		ret += " )";
		return ret;
	}

	docopt::PatternList parse_argv(Tokens tokens, std::vector<docopt::Option>& options, bool options_first)
	{
		// Parse command-line argument vector.
		//
		// If options_first:
		//    argv ::= [ long | shorts ]* [ argument ]* [ '--' [ argument ]* ] ;
		// else:
		//    argv ::= [ long | shorts | argument ]* [ '--' [ argument ]* ] ;

		docopt::PatternList ret;
		while(tokens) {
			auto const& token = tokens.current();

			if(token == "--") {
				// option list is done; convert all the rest to arguments
				while(tokens) {
					ret.emplace_back(std::make_shared<docopt::Argument>("", tokens.pop()));
				}
			} else if(starts_with(token, "--")) {
				auto&& parsed = parse_long(tokens, options);
				std::move(parsed.begin(), parsed.end(), std::back_inserter(ret));
			} else if(token[0] == '-' && token != "-") {
				auto&& parsed = parse_short(tokens, options);
				std::move(parsed.begin(), parsed.end(), std::back_inserter(ret));
			} else if(options_first) {
				// option list is done; convert all the rest to arguments
				while(tokens) {
					ret.emplace_back(std::make_shared<docopt::Argument>("", tokens.pop()));
				}
			} else {
				ret.emplace_back(std::make_shared<docopt::Argument>("", tokens.pop()));
			}
		}

		return ret;
	}

	std::vector<docopt::Option> parse_defaults(std::string const& doc) {
		// This pattern is a delimiter by which we split the options.
		// The delimiter is a new line followed by a whitespace(s) followed by one or two hyphens.
		static docopt::regex const re_delimiter{
			"(?:^|\\n)[ \\t]*"  // a new line with leading whitespace
			"(?=-{1,2})"        // [split happens here] (positive lookahead) ... and followed by one or two hyphes
		};

		std::vector<docopt::Option> defaults;
		for(auto s : parse_section("options:", doc)) {
			s.erase(s.begin(), s.begin() + static_cast<std::ptrdiff_t>(s.find(':')) + 1); // get rid of "options:"

			for(const auto& opt : regex_split(s, re_delimiter)) {
				if(starts_with(opt, "-")) {
					defaults.emplace_back(docopt::Option::parse(opt));
				}
			}
		}

		return defaults;
	}

	bool isOptionSet(docopt::PatternList const& options, std::string const& opt1, std::string const& opt2 = "") {
		return std::any_of(options.begin(), options.end(), [&](std::shared_ptr<docopt::Pattern const> const& opt) -> bool {
			auto const& name = opt->name();
			if(name == opt1 || (!opt2.empty() && name == opt2)) {
				return opt->hasValue();
			}
			return false;
		});
	}

	void extras(bool help, bool version, docopt::PatternList const& options) {
		if(help && isOptionSet(options, "-h", "--help")) {
			throw docopt::exit_help();
		}

		if(version && isOptionSet(options, "--version")) {
			throw docopt::exit_version();
		}
	}

	// Parse the doc string and generate the Pattern tree
	std::pair<docopt::Required, std::vector<docopt::Option>> create_pattern_tree(std::string const& doc)
	{
		auto usage_sections = parse_section("usage:", doc);
		if(usage_sections.empty()) {
			throw docopt::language_error("'usage:' (case-insensitive) not found.");
		}
		if(usage_sections.size() > 1) {
			throw docopt::language_error("More than one 'usage:' (case-insensitive).");
		}

		std::vector<docopt::Option> options = parse_defaults(doc);
		docopt::Required pattern = parse_pattern(formal_usage(usage_sections[0]), options);

		std::vector<docopt::Option const*> pattern_options = flat_filter<docopt::Option const>(pattern);

		using UniqueOptions = std::unordered_set<docopt::Option const*, docopt::PatternHasher, docopt::PatternPointerEquality>;
		UniqueOptions const uniq_pattern_options{ pattern_options.begin(), pattern_options.end() };

		// Fix up any "[options]" shortcuts with the actual option tree
		for(auto& options_shortcut : flat_filter<docopt::OptionsShortcut>(pattern)) {
			std::vector<docopt::Option> doc_options = parse_defaults(doc);

			// set(doc_options) - set(pattern_options)
			UniqueOptions uniq_doc_options;
			for(auto const& opt : doc_options) {
				if(uniq_pattern_options.count(&opt))
					continue;
				uniq_doc_options.insert(&opt);
			}

			// turn into shared_ptr's and set as children
			docopt::PatternList children;
			std::transform(uniq_doc_options.begin(), uniq_doc_options.end(), std::back_inserter(children),
				[](docopt::Option const* opt) {
				return std::make_shared<docopt::Option>(*opt);
			});
			options_shortcut->setChildren(std::move(children));
		}

		return { std::move(pattern), std::move(options) };
	}
}

DOCOPT_INLINE
std::map<std::string, docopt::value>
docopt::docopt_parse(std::string const& doc,
		     std::vector<std::string> const& argv,
		     bool help,
		     bool version,
		     bool options_first)
{
	Required pattern;
	std::vector<Option> options;
	try {
		std::tie(pattern, options) = create_pattern_tree(doc);
	} catch(Tokens::OptionError const& error) {
		throw language_error(error.what());
	}

	PatternList argv_patterns;
	try {
		argv_patterns = parse_argv(Tokens(argv), options, options_first);
	} catch(Tokens::OptionError const& error) {
		throw argument_error(error.what());
	}

	extras(help, version, argv_patterns);

	std::vector<std::shared_ptr<LeafPattern>> collected;
	const bool matched = pattern.fix().match(argv_patterns, collected);
	if(matched && argv_patterns.empty()) {
		std::map<std::string, value> ret;

		// (a.name, a.value) for a in (pattern.flat() + collected)
		for(const auto* p : pattern.leaves()) {
			ret[p->name()] = p->getValue();
		}

		for(auto const& p : collected) {
			ret[p->name()] = p->getValue();
		}

		return ret;
	}

	if(matched) {
		std::string leftover = join(argv.begin(), argv.end(), ", ");
		throw argument_error("Unexpected argument: " + leftover);
	}

	throw argument_error("Arguments did not match expected patterns"); // BLEH. Bad error.
}

DOCOPT_INLINE
std::map<std::string, docopt::value>
docopt::docopt(std::string const& doc,
	       std::vector<std::string> const& argv,
	       bool help,
	       std::string const& version,
	       bool options_first) noexcept
{
	try {
		return docopt_parse(doc, argv, help, !version.empty(), options_first);
	} catch(exit_help const&) {
		std::cout << doc << std::endl;
		std::exit(0);
	} catch(exit_version const&) {
		std::cout << version << std::endl;
		std::exit(0);
	} catch(language_error const& error) {
		std::cerr << "Docopt usage string could not be parsed" << std::endl;
		std::cerr << error.what() << std::endl;
		std::exit(-1);
	} catch(argument_error const& error) {
		std::cerr << error.what();
		std::cout << std::endl;
		std::cout << doc << std::endl;
		std::exit(-1);
	} /* Any other exception is unexpected: let std::terminate grab it */
}

#pragma endregion
