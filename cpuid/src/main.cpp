#include "stdafx.h"

#include "cpuid/cpuid.hpp"

#include "docopt.h"

static const char usage_message[] =
R"(cpuid.

Usage:
	cpuid [--read-dump <filename>] [--read-format <format>] [--all-cpus | --cpu <id>] [--ignore-vendor] [--ignore-feature-bits] [--brute-force] [--raw] [--write-dump <filename>] [--write-format <format>] [--single-value <spec>] [--no-topology | --only-topology]
	cpuid --list-ids [--read-dump <filename>] [--read-format <format>]
	cpuid --help
	cpuid --version

Input options:
	--read-dump=<filename>     Read from <filename> rather than the current processors
	--read-format=<format>     Dump format to read: native, etallen, libcpuid, instlat. [default: native]
	--all-cpus                 Show output from every CPU
	--cpu <id>                 Show output from CPU with APIC ID <id>
	--single-value <spec>      Print specific flag value, using Intel syntax (e.g. CPUID.01H.EDX.SSE[bit 25]).
	                           Handles most of the wild inconsistencies found in Intel's documentation.
	--ignore-vendor            Ignore vendor constraints
	--ignore-feature-bits      Ignore feature bit constraints
	--brute-force              Ignore constraints, and enumerate even reserved leaves

Output options:
	--raw                      Write unparsed output to screen
	--write-dump=<filename>    Write unparsed output to <filename>
	--write-format=<format>    Dump format to write: native, etallen, libcpuid, instlat. [default: native]
	--no-topology              Don't print the processor and cache topology
	--only-topology            Only print the processor and cache topology
	--list-ids                 List all core IDs

Other options:
	--help                     Show this text
	--version                  Show the version

)";

static const char version[] = "cpuid 0.1";

int main(int argc, char* argv[]) try {
	HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	::GetConsoleMode(output, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	::SetConsoleMode(output, mode);
	::SetConsoleCP(CP_UTF8);
	::SetConsoleOutputCP(CP_UTF8);

	std::cout.rdbuf()->pubsetbuf(nullptr, 1024);

	const std::map<std::string, docopt::value> args = docopt::docopt_parse(usage_message, { argv + 1, argv + argc }, true, true);
	const bool skip_vendor_check  = std::get<bool>(args.at("--ignore-vendor"));
	const bool skip_feature_check = std::get<bool>(args.at("--ignore-feature-bits"));
	const bool raw_dump           = std::get<bool>(args.at("--raw"));
	const bool all_cpus           = std::get<bool>(args.at("--all-cpus"));
	const bool list_ids           = std::get<bool>(args.at("--list-ids"));
	const bool brute_force        = std::get<bool>(args.at("--brute-force"));
	const bool no_topology        = std::get<bool>(args.at("--no-topology"));
	const bool only_topology      = std::get<bool>(args.at("--only-topology"));

	std::map<std::uint32_t, cpu_t> logical_cpus;
	if(std::holds_alternative<std::string>(args.at("--read-dump"))) {
		file_format format = file_format::native;
		const std::string format_name = boost::to_lower_copy(std::get<std::string>(args.at("--read-format")));
		if("etallen" == format_name) {
			format = file_format::etallen;
		} else if("libcpuid" == format_name) {
			format = file_format::libcpuid;
		} else if("instlat" == format_name) {
			format = file_format::instlat;
		}
		const std::string filename = std::get<std::string>(args.at("--read-dump"));
		std::ifstream fin;
		if(filename != "-") {
			fin.open(filename);
			if(!fin) {
				throw std::runtime_error(fmt::format("Could not open {:s} for input", filename));
			}
		}
		logical_cpus = enumerate_file(filename != "-" ? fin : std::cin, format);
	} else {
		logical_cpus = enumerate_processors(brute_force, skip_vendor_check, skip_feature_check);
	}

	if(logical_cpus.size() == 0) {
		throw std::runtime_error("No processors found, which is implausible.");
	}

	if(list_ids) {
		fmt::MemoryWriter w;
		for(const auto& p : logical_cpus) {
			w.write("{:#04x}\n", p.first);
		}
		std::cout << w.str() << std::flush;
		return 0;
	}

	if(raw_dump || std::holds_alternative<std::string>(args.at("--write-dump"))) {
		file_format format = file_format::native;
		const std::string format_name = boost::to_lower_copy(std::get<std::string>(args.at("--write-format")));
		if("etallen" == format_name) {
			format = file_format::etallen;
		} else if("libcpuid" == format_name) {
			format = file_format::libcpuid;
		} else if("instlat" == format_name) {
			format = file_format::instlat;
		}
		fmt::MemoryWriter w;
		print_dump(w, logical_cpus, format);
		std::string filename = "-";
		if(std::holds_alternative<std::string>(args.at("--write-dump"))) {
			filename = std::get<std::string>(args.at("--write-dump"));
		}
		std::ofstream fout;
		if(filename != "-") {
			fout.open(filename);
			if(!fout) {
				throw std::runtime_error(fmt::format("Could not open {:s} for output", filename));
			}
		}
		(filename != "-" ? fout : std::cout) << w.str() << std::flush;
		return 0;
	}

	std::vector<std::uint32_t> cpu_ids;

	if(all_cpus) {
		for(const auto& p : logical_cpus) {
			cpu_ids.push_back(p.second.apic_id);
		}
	} else {
		const std::uint32_t chosen_id = std::holds_alternative<std::string>(args.at("--cpu")) ? std::stoul(std::get<std::string>(args.at("--cpu")), nullptr, 16)
		                                                                                      : logical_cpus.begin()->first;
		if(logical_cpus.find(chosen_id) == logical_cpus.end()) {
			throw std::runtime_error(fmt::format("No such CPU ID: {:#04x}\n", chosen_id));
		}
		cpu_ids.push_back(chosen_id);
	}

	if(!only_topology) {
		for(const std::uint32_t chosen_id : cpu_ids) {
			const cpu_t& cpu = logical_cpus.at(chosen_id);
			fmt::MemoryWriter w;
			if(std::holds_alternative<std::string>(args.at("--single-value"))) {
				const std::string flag_spec = std::get<std::string>(args.at("--single-value"));
				print_single_flag(w, cpu, flag_spec);
			} else {
				print_leaves(w, cpu, skip_vendor_check, skip_feature_check);
			}
			std::cout << w.str() << std::flush;
		}
	}

	if(!std::holds_alternative<std::string>(args.at("--single-value"))
	&& !no_topology) {
		fmt::MemoryWriter w;
		system_t machine = build_topology(logical_cpus);
		print_topology(w, machine);
		std::cout << w.str() << std::flush;
	}

	return EXIT_SUCCESS;
} catch(const docopt::exit_help&) {
	std::cout << usage_message << std::endl;
	return EXIT_SUCCESS;
} catch(const docopt::exit_version&) {
	std::cout << version << std::endl;
	return EXIT_SUCCESS;
} catch(const docopt::language_error& e) {
	std::cerr << "Docopt usage string could not be parsed" << std::endl;
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
} catch(const docopt::argument_error& e) {
	std::cerr << e.what() << std::endl;
	std::cerr << usage_message << std::endl;
	return EXIT_FAILURE;
} catch(std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}