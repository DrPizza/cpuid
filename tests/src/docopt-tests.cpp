#include "stdafx.h"

#include <CppUnitTest.h>

#include "docopt/docopt.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(docopt_tests) {
public:
	TEST_METHOD(test_docopt_parsing) {
		wchar_t dir[260];
		::GetCurrentDirectoryW(260, dir);

		std::ifstream fin("../../../tests/data/docopt.testcases");
		std::stringstream buffer;
		buffer << fin.rdbuf();
		const std::string whole_file = buffer.str();

		const std::regex comment_pattern(" *#.*?\n");
		const std::string no_comments = std::regex_replace(whole_file, comment_pattern, "");

		const std::string section_start = "r\"\"\"";
		auto start = no_comments.find(section_start);
		auto end   = no_comments.find(section_start, start + section_start.size());

		std::vector<std::string> raw_sections;
		for(; end != std::string::npos; start = end, end = no_comments.find(section_start, start + section_start.size())) {
			raw_sections.push_back(std::string(no_comments.data() + start + section_start.size(), no_comments.data() + end));
		}
		raw_sections.push_back(no_comments.substr(start + section_start.size()));

		struct docopt_test_invocation
		{
			std::vector<std::string> argv;
			std::string expected_result;
		};
		struct docopt_test_data
		{
			std::string usage;
			std::vector<docopt_test_invocation> tests;
		};

		std::vector<docopt_test_data> command_lines;
		const std::string section_end = "\"\"\"";
		const std::string invocation_start = "$ prog";
		const std::regex  newline("\n");
		for(std::string const& section : raw_sections) {
			const std::string usage_section   = section.substr(0, section.find(section_end));
			const std::string all_invocations = section.substr(section.find(section_end) + section_end.size());

			std::vector<std::string> raw_invocations;
			auto s = all_invocations.find(invocation_start);
			auto e = all_invocations.find(invocation_start, s + invocation_start.size());
			for(; e != std::string::npos; s = e, e = all_invocations.find(invocation_start, s + invocation_start.size())) {
				raw_invocations.push_back(std::string(all_invocations.data() + s + invocation_start.size(), all_invocations.data() + e));
			}
			raw_invocations.push_back(all_invocations.substr(s + invocation_start.size()));

			docopt_test_data data = {
				usage_section
			};

			for(std::string const& inv : raw_invocations) {
				const std::string args     = inv.substr(0, inv.find("\n"));
				const std::string expected = std::regex_replace(inv.substr(inv.find("\n") + 1), newline, "");
				docopt_test_invocation invocation = {
					{}, expected
				};
				boost::algorithm::split(invocation.argv, args, boost::is_any_of(" "));

				invocation.argv.erase(std::remove(std::begin(invocation.argv), std::end(invocation.argv), ""), std::end(invocation.argv));
				data.tests.push_back(invocation);
			}
			command_lines.push_back(data);
		}
		
		const auto results_to_json = [&] (const std::map<std::string, docopt::value>& results) {
			bool first = true;
			std::string json;
			json += "{";
			for(auto const& arg : results) {
				if(first) {
					first = false;
				} else {
					json += ", ";
				}
				json += '"';
				json += arg.first;
				json += "\": ";
				json += std::visit(docopt::value_printer, arg.second);
			}
			json += "}";
			return json;
		};

		for(const docopt_test_data& data : command_lines) {
			for(const docopt_test_invocation& inv : data.tests) {
				try {
					auto result = docopt::docopt_parse(data.usage, inv.argv, false, false);
					const std::string json = results_to_json(result);
					Logger::WriteMessage(json.c_str());
					Assert::AreEqual(inv.expected_result, json);
				} catch(std::exception&) {
					Assert::AreEqual(inv.expected_result, std::string("\"user-error\""));
				}
			}
		}
	}
};
