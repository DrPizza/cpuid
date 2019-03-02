#include "stdafx.h"

#include "docopt/docopt.hpp"

namespace
{
	struct docopt_test_data
	{
		std::string usage;
		std::vector<std::string> argv;
		std::string expected_result;
		std::size_t test_number;
		std::size_t subtest_number;
	};

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

	std::vector<docopt_test_data> load_data() {
		wchar_t dir[260];
		::GetModuleFileNameW(nullptr, dir, 260);
		wchar_t* i = dir + std::wcslen(dir);
		while(*--i != L'\\') {
			*i = '\0';
		}
		::SetCurrentDirectoryW(dir);

		std::ifstream fin("../../../docopt/tests/data/docopt.testcases");
		std::stringstream buffer;
		buffer << fin.rdbuf();
		const std::string whole_file = buffer.str();

		const std::regex comment_pattern(" *#.*?\n");
		const std::string no_comments = std::regex_replace(whole_file, comment_pattern, "");

		const std::string section_start = "r\"\"\"";
		auto start = no_comments.find(section_start);
		auto end = no_comments.find(section_start, start + section_start.size());

		std::vector<std::string> raw_sections;
		for(; end != std::string::npos; start = end, end = no_comments.find(section_start, start + section_start.size())) {
			raw_sections.push_back(std::string(no_comments.data() + start + section_start.size(), no_comments.data() + end));
		}
		raw_sections.push_back(no_comments.substr(start + section_start.size()));

		std::vector<docopt_test_data> command_lines;
		const std::string section_end = "\"\"\"";
		const std::string invocation_start = "$ prog";
		const std::regex  newline("\n");
		std::size_t test_number = 0;
		for(std::string const& section : raw_sections) {
			const std::string usage_section = section.substr(0, section.find(section_end));
			const std::string all_invocations = section.substr(section.find(section_end) + section_end.size());

			std::vector<std::string> raw_invocations;
			auto s = all_invocations.find(invocation_start);
			auto e = all_invocations.find(invocation_start, s + invocation_start.size());
			for(; e != std::string::npos; s = e, e = all_invocations.find(invocation_start, s + invocation_start.size())) {
				raw_invocations.push_back(std::string(all_invocations.data() + s + invocation_start.size(), all_invocations.data() + e));
			}
			raw_invocations.push_back(all_invocations.substr(s + invocation_start.size()));
			std::size_t subtest_number = 0;
			for(std::string const& inv : raw_invocations) {
				const std::string args = inv.substr(0, inv.find("\n"));
				std::string expected = std::regex_replace(inv.substr(inv.find("\n") + 1), newline, "");

				if(expected[0] == '"') {
					expected = expected.substr(1);
				}
				if(expected[expected.size() - 1] == '"') {
					expected = expected.substr(0, expected.size() - 1);
				}

				docopt_test_data data = {
					usage_section, docopt::crack_argv(args), expected, test_number, subtest_number
				};
				command_lines.push_back(data);
				++subtest_number;
			}
			++test_number;
		}
		return command_lines;
	}
}

const std::vector<docopt_test_data> command_lines = load_data();

struct DocoptTest : ::testing::TestWithParam<docopt_test_data>
{
};

TEST_P(DocoptTest, ParserTest) {
	docopt_test_data data = GetParam();
	try {
		auto result = docopt::docopt_parse(data.usage, data.argv, false, false);
		const std::string json = results_to_json(result);
		EXPECT_EQ(data.expected_result, json);
	} catch(std::exception& e) {
		EXPECT_EQ(data.expected_result, e.what());
	}
}

std::string param_printer(testing::TestParamInfo<docopt_test_data> data) {
	std::string index_padding = data.index < 10  ? "00"
	                          : data.index < 100 ? "0"
	                          :                    "";

	std::string test_padding = data.param.test_number < 10  ? "00"
	                         : data.param.test_number < 100 ? "0"
	                         :                                "";

	std::string subtest_padding = data.param.subtest_number < 10  ? "00"
	                            : data.param.subtest_number < 100 ? "0"
	                            :                                   "";

	return index_padding + std::to_string(data.index)
	     + "Section" + test_padding + std::to_string(data.param.test_number)
	     + "Params" + subtest_padding + std::to_string(data.param.subtest_number);
}

INSTANTIATE_TEST_CASE_P(DocoptFullTests, DocoptTest, ::testing::ValuesIn(command_lines), param_printer);
