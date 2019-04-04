// from https://github.com/docopt/docopt.cpp, dual licensed under MIT and Boost license

//
//  docopt_private.h
//  docopt
//
//  Created by Jared Grubb on 2013-11-04.
//  Copyright (c) 2013 Jared Grubb. All rights reserved.
//

#ifndef docopt_docopt_private_h
#define docopt_docopt_private_h

#include <vector>
#include <memory>
#include <unordered_set>
#include <cassert>
#include <string>
#include <functional>
#include <iosfwd>
#include <variant>
#include <charconv>

namespace docopt {
	inline bool is_empty(const value& v) noexcept {
		return std::holds_alternative<std::monostate>(v);
	}

	struct Pattern;
	struct LeafPattern;

	using PatternList = std::vector<std::shared_ptr<Pattern>>;

	struct Pattern : std::enable_shared_from_this<Pattern>
	{
		// Get all instances of 'T' from the pattern
		template <typename T>
		std::vector<std::shared_ptr<T>> flat() {
			std::vector<std::shared_ptr<docopt::Pattern>> flattened = this->do_flat([](std::shared_ptr<const docopt::Pattern> p) noexcept -> bool {
				return !!std::dynamic_pointer_cast<const T>(p);
			});

			// now, we're guaranteed to have T*'s, so just use static_cast
			std::vector<std::shared_ptr<T>> ret;
			std::transform(flattened.begin(), flattened.end(), std::back_inserter(ret), [](std::shared_ptr<docopt::Pattern> p) noexcept {
				return std::static_pointer_cast<T>(p);
			});
			return ret;
		}

		virtual std::string to_string() const = 0;

		inline static std::size_t depth = 0;

		static std::string get_spaces() {
			return std::string(depth, ' ');
		}

		// flatten out children, stopping descent when the given filter returns 'true'
		virtual std::vector<std::shared_ptr<Pattern>> do_flat(bool(* filter)(std::shared_ptr<const Pattern>)) = 0;

		// Attempt to find something in 'left' that matches this pattern's spec, and if so, move it to 'collected'
		virtual bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const = 0;

		virtual bool hasValue() const noexcept {
			return false;
		}

		Pattern() = default;
		Pattern(const Pattern&) = default;
		Pattern(Pattern&&) = default;
		Pattern& operator=(const Pattern&) = default;
		Pattern& operator=(Pattern&&) = default;
		virtual ~Pattern() = default;
	};

	struct LeafPattern : Pattern
	{
		LeafPattern(std::string name, value v = {}) : fName(std::move(name)),
		                                              fValue(std::move(v)) {
		}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 26429)
#endif

		std::vector<std::shared_ptr<Pattern>> do_flat(bool(* filter)(std::shared_ptr<const Pattern>)) override {
			auto shared_this = this->shared_from_this();
			if((*filter)(shared_this)) {
				return { shared_this };
			}
			return {};
		}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

		std::string to_string() const override {
			const std::string val = std::visit(docopt::value_printer, fValue);
			
			const auto to_hex = [] (const void* x) {
				std::array<char, 2 + (2 * sizeof(void*))> bytes = { '0', 'x' };
				const auto result = std::to_chars(bytes.data() + 2, bytes.data() + bytes.size(), reinterpret_cast<std::size_t>(x), 16);
				return std::string(bytes.data(), result.ptr);
			};

			return get_spaces() + std::string(typeid(*this).name()) + " (" + name() + ": " + val + ") " + to_hex(this);
		}

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;

		bool hasValue() const noexcept override {
			return !std::holds_alternative<std::monostate>(fValue);
		}

		value const& getValue() const noexcept {
			return fValue;
		}

		void setValue(value&& v) {
			fValue = std::move(v);
		}

		std::string const& name() const noexcept {
			return fName;
		}

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const&) const = 0;

	private:
		std::string fName;
		value fValue;
	};

	struct match_by_name
	{
		bool operator()(const std::shared_ptr<docopt::LeafPattern const> lhs,
			const std::shared_ptr<docopt::LeafPattern const> rhs) const noexcept {
			if (lhs == rhs) {
				return true;
			}
			return lhs->name() == rhs->name();
		}
	};

	struct sort_by_name
	{
		bool operator()(const std::shared_ptr<docopt::LeafPattern const> lhs,
			const std::shared_ptr<docopt::LeafPattern const> rhs) const noexcept {
			return lhs->name() < rhs->name();
		}
	};

	struct BranchPattern : Pattern
	{
		BranchPattern(PatternList children = {}) noexcept : fChildren(std::move(children)) {
		}

		std::string to_string() const override {
			std::string rv = get_spaces() + typeid(*this).name();
			rv += " {\n";
			++Pattern::depth;
			for(const auto& c : children()) {
				rv += c->to_string();
				rv += ",\n";
			}
			--Pattern::depth;
			rv += get_spaces() + "}";
			return rv;
		}

		std::shared_ptr<Pattern> fix() {
			auto uniq = flat<docopt::LeafPattern>();
			std::sort(std::begin(uniq), std::end(uniq), sort_by_name{});
			uniq.erase(std::unique(std::begin(uniq), std::end(uniq), match_by_name{}), std::end(uniq));

			fix_identities(uniq);
			fix_repeating_arguments(this);
			return this->shared_from_this();
		}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 26429)
#endif

		std::vector<std::shared_ptr<Pattern>> do_flat(bool(* filter)(std::shared_ptr<const Pattern>)) override {
			auto shared_this = this->shared_from_this();
			if((*filter)(shared_this)) {
				return { shared_this };
			}

			std::vector<std::shared_ptr<Pattern>> ret;
			for(auto& child : fChildren) {
				auto sublist = child->do_flat(filter);
				std::move(sublist.begin(), sublist.end(), std::back_inserter(ret));
			}
			return ret;
		}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

		void setChildren(PatternList children) {
			fChildren = std::move(children);
		}

		PatternList const& children() const noexcept {
			return fChildren;
		}

		virtual void fix_identities(std::vector<std::shared_ptr<docopt::LeafPattern>>& patterns) {
			for(auto& child : fChildren) {
				if(auto bp = dynamic_cast<BranchPattern*>(child.get())) {
					bp->fix_identities(patterns);
				} else {
					child = *std::find_if(std::begin(patterns), std::end(patterns), [&] (const auto& p) {
						return match_by_name{}(p, std::dynamic_pointer_cast<LeafPattern const>(child));
					});
				}
			}
		}

	private:
		void fix_repeating_arguments(Pattern* current);

	protected:
		PatternList fChildren;
	};

	struct Argument : LeafPattern
	{
		using LeafPattern::LeafPattern;

	protected:
		std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;
	};

	struct Command : Argument
	{
		Command(std::string name, value v = value{ false }) : Argument(std::move(name), std::move(v)) {
		}

	protected:
		std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;
	};

	struct Option final : LeafPattern
	{
		static std::shared_ptr<Option> parse(std::string const& option_description);

		Option(std::string shortOption,
		       std::string longOption,
		       int argcount = 0,
		       value v = value{false}) : LeafPattern(longOption.empty() ? shortOption : longOption, std::move(v)),
		                                 fShortOption(std::move(shortOption)),
		                                 fLongOption(std::move(longOption)),
		                                 fArgcount(argcount) {
			// From Python:
			//   self.value = None if value is False and argcount else value
			if(argcount && std::holds_alternative<bool>(v) && !std::get<bool>(v)) {
				setValue(value{});
			}
		}

		Option(const Option&) = default;
		Option(Option&&) = default;
		Option& operator=(const Option&) = default;
		Option& operator=(Option&&) = default;
		~Option() = default;

		using LeafPattern::setValue;

		const std::string& longOption() const noexcept {
			return fLongOption;
		}

		const std::string& shortOption() const noexcept {
			return fShortOption;
		}

		int argCount() const noexcept {
			return fArgcount;
		}

	protected:
		std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;

	private:
		std::string fShortOption;
		std::string fLongOption;
		int fArgcount;
	};

	struct Required : BranchPattern
	{
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

	struct Optional : BranchPattern
	{
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override {
			for(auto const& pattern : fChildren) {
				pattern->match(left, collected);
			}
			return true;
		}
	};

	struct OptionsShortcut : Optional
	{
		using Optional::Optional;
	};

	struct OneOrMore : BranchPattern
	{
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

	struct Either : BranchPattern
	{
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

	inline void make_leaf_repeatable(std::shared_ptr<LeafPattern> leaf) {
		if(std::holds_alternative<unsigned long long>(leaf->getValue())
		|| std::holds_alternative<std::vector<std::string>>(leaf->getValue())) {
			return;
		}

		bool ensureList = false;
		bool ensureInt = false;

		if(std::dynamic_pointer_cast<Command>(leaf)) {
			ensureInt = true;
		} else if(std::dynamic_pointer_cast<Argument>(leaf)) {
			ensureList = true;
		} else if(std::shared_ptr<Option const> const o = std::dynamic_pointer_cast<Option const>(leaf)) {
			if(o->argCount()) {
				ensureList = true;
			} else {
				ensureInt = true;
			}
		}

		if(ensureList) {
			std::vector<std::string> newValue;
			if(std::holds_alternative<std::string>(leaf->getValue())) {
				newValue = split(std::get<std::string>(leaf->getValue()));
			}
			leaf->setValue(value{ newValue });
		} else if(ensureInt) {
			leaf->setValue(value{ 0ull });
		}
	}

	inline void BranchPattern::fix_repeating_arguments(Pattern* current) {
		if(BranchPattern* bp = dynamic_cast<BranchPattern*>(current)) {
			const bool all_children_repeat = dynamic_cast<OneOrMore*>(bp) != nullptr;
			const bool no_children_repeat  = dynamic_cast<Either*>(bp) != nullptr;
			const std::size_t child_count = bp->fChildren.size();
			for(std::size_t i = 0; i < child_count; ++i) {
				auto& child = bp->fChildren.at(i);
				if(all_children_repeat) {
					// all children of a OneOrMore can be repeated
					auto child_leaves = child->flat<LeafPattern>();
					for(auto& child_leaf : child_leaves) {
						make_leaf_repeatable(child_leaf);
					}
				} else if(!no_children_repeat) {
					// children of one branch of a Required or Optional can match children of any other branch
					for(std::size_t j = 0; j < child_count; ++j) {
						if(i == j) {
							continue;
						}
						auto& sibling = bp->fChildren.at(j);

						auto child_leaves = child->flat<LeafPattern>();
						auto sibling_leaves = sibling->flat<LeafPattern>();

						for(auto& child_leaf : child_leaves) {
							for(auto& sibling_leaf : sibling_leaves) {
								if(child_leaf == sibling_leaf) {
									make_leaf_repeatable(child_leaf);
								}
							}
						}
					}
				} else {
					// children of one branch of an Either cannot match children of any other branch
				}
				// and recurse
				fix_repeating_arguments(child.get());
			}
		}
	}

	inline bool LeafPattern::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const {
		auto match = single_match(left);
		if(!match.second) {
			return false;
		}

		left.erase(left.begin() + static_cast<std::ptrdiff_t>(match.first));

		auto same_name = std::find_if(collected.begin(), collected.end(), [&](std::shared_ptr<LeafPattern> const& p) noexcept {
			return p->name() == name();
		});
		if(std::holds_alternative<unsigned long long>(getValue())) {
			unsigned long long val = 1;
			if(same_name == collected.end()) {
				collected.push_back(match.second);
				match.second->setValue(value{val});
			} else if(std::holds_alternative<unsigned long long>((**same_name).getValue())) {
				val += std::get<unsigned long long>((**same_name).getValue());
				(**same_name).setValue(value{val});
			} else {
				(**same_name).setValue(value{val});
			}
		} else if(std::holds_alternative<std::vector<std::string> >(getValue())) {
			std::vector<std::string> val;
			if (std::holds_alternative<std::string>(match.second->getValue())) {
				val.push_back(std::get<std::string>(match.second->getValue()));
			} else if(std::holds_alternative<std::vector<std::string> >(match.second->getValue())) {
				val = std::get<std::vector<std::string> >(match.second->getValue());
			} else {
				/// cant be!?
			}

			if(same_name == collected.end()) {
				collected.push_back(match.second);
				match.second->setValue(value{ val });
			} else if (std::holds_alternative<std::vector<std::string> >((**same_name).getValue())) {
				std::vector<std::string> const& list = std::get<std::vector<std::string> >((**same_name).getValue());
				val.insert(val.begin(), list.begin(), list.end());
				(**same_name).setValue(value{ val });
			} else {
				(**same_name).setValue(value{ val });
			}
		} else {
			collected.push_back(match.second);
		}
		return true;
	}

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Argument::single_match(PatternList const& left) const {
		std::pair<size_t, std::shared_ptr<LeafPattern>> ret{};
		const size_t size = left.size();
		for(size_t i = 0; i < size; ++i) {
			auto arg = dynamic_cast<Argument const*>(left.at(i).get());
			if(arg) {
				ret.first = i;
				ret.second = std::make_shared<Argument>(name(), arg->getValue());
				break;
			}
		}

		return ret;
	}

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Command::single_match(PatternList const& left) const {
		std::pair<size_t, std::shared_ptr<LeafPattern>> ret{};
		const size_t size = left.size();
		for(size_t i = 0; i < size; ++i) {
			auto arg = dynamic_cast<Argument const*>(left.at(i).get());
			if(arg) {
				if (name() == std::get<std::string>(arg->getValue())) {
					ret.first = i;
					ret.second = std::make_shared<Command>(name(), value{ true });
				}
				break;
			}
		}

		return ret;
	}

	inline std::shared_ptr<Option> Option::parse(std::string const& option_description) {
		std::string shortOption, longOption;
		int argcount = 0;
		value val{ false };

		const auto double_space = option_description.find("  ");
		auto options_end = option_description.end();
		if(double_space != std::string::npos) {
			options_end = option_description.begin() + static_cast<std::ptrdiff_t>(double_space);
		}

		static const xp::sregex pattern(xp::sregex::compile("(-{1,2})?(.*?)([,= ]|$)"));
		for(xp::sregex_iterator i(option_description.begin(), options_end, pattern, xp::regex_constants::match_not_null), e{};
		    i != e;
		    ++i) {
			xp::smatch const& match = *i;
			if(match[1].matched) { // [1] is optional.
				if(match[1].length() == 1) {
					shortOption = "-" + match[2].str();
				} else {
					longOption = "--" + match[2].str();
				}
			} else if(match[2].length() > 0) { // [2] always matches.
				std::string m = match[2];
				argcount = 1;
			} else {
				// delimeter
			}

			if(match[3].length() == 0) { // [3] always matches.
				                         // Hit end of string. For some reason 'match_not_null' will let us match empty
				                         // at the end, and then we'll spin in an infinite loop. So, if we hit an empty
				                         // match, we know we must be at the end.
				break;
			}
		}

		if(argcount) {
			xp::smatch match;
			if(xp::regex_search(options_end, option_description.end(), match, xp::sregex(xp::sregex::compile("\\[default: (.*)\\]", xp::sregex::icase)))) {
				val = match[1].str();
			}
		}

		return std::make_shared<Option>(std::move(shortOption), std::move(longOption), argcount, std::move(val));
	}

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Option::single_match(PatternList const& left) const {
		auto thematch = std::find_if(left.begin(), left.end(), [this](std::shared_ptr<Pattern> const& a) noexcept {
			auto leaf = std::dynamic_pointer_cast<LeafPattern>(a);
			return leaf && this->name() == leaf->name();
		});
		if(thematch == left.end()) {
			return {};
		}
		return std::make_pair(std::distance(left.begin(), thematch), std::dynamic_pointer_cast<LeafPattern>(*thematch));
	}

	inline bool Required::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const {
		auto l = left;
		auto c = collected;
		for(auto const& pattern : fChildren) {
			const bool ret = pattern->match(l, c);
			if(!ret) {
				// leave (left, collected) untouched
				return false;
			}
		}

		left = std::move(l);
		collected = std::move(c);
		return true;
	}

	inline bool OneOrMore::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const {
		assert(fChildren.size() == 1);

		auto l = left;
		auto c = collected;

		bool matched = true;
		size_t times = 0;

		decltype(l) l_;
		bool firstLoop = true;

		while(matched) {
			// could it be that something didn't match but changed l or c?
			matched = fChildren.at(0)->match(l, c);

			if(matched) {
				++times;
			}

			if(firstLoop) {
				firstLoop = false;
			} else if(l == l_) {
				break;
			}

			l_ = l;
		}

		if(times == 0) {
			return false;
		}

		left = std::move(l);
		collected = std::move(c);
		return true;
	}

	inline bool Either::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const {
		using Outcome = std::pair<PatternList, std::vector<std::shared_ptr<LeafPattern>>>;

		std::vector<Outcome> outcomes;

		for(auto const& pattern : fChildren) {
			// need a copy so we apply the same one for every iteration
			auto l = left;
			auto c = collected;
			const bool matched = pattern->match(l, c);
			if(matched) {
				outcomes.emplace_back(std::move(l), std::move(c));
			}
		}

		auto min = std::min_element(outcomes.begin(), outcomes.end(), [](Outcome const& o1, Outcome const& o2) noexcept {
			return o1.first.size() < o2.first.size();
		});

		if(min == outcomes.end()) {
			// (left, collected) unchanged
			return false;
		}

		std::tie(left, collected) = std::move(*min);
		return true;
	}

}

#endif
