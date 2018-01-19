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

// Workaround GCC 4.8 not having std::regex
#if DOCTOPT_USE_BOOST_REGEX
#include <boost/regex.hpp>
namespace docopt {
	using boost::regex;
	using boost::sregex_iterator;
	using boost::smatch;
	using boost::regex_search;
	namespace regex_constants {
		using boost::regex_constants::match_not_null;
	}
}
#else
#include <regex>
namespace docopt {
	using std::regex;
	using std::sregex_iterator;
	using std::smatch;
	using std::regex_search;
	namespace regex_constants {
		using std::regex_constants::match_not_null;
	}
}

#endif

#pragma region declarations

namespace std {
	template<> struct hash<std::vector<std::string> > {
		typedef std::vector<std::string> argument_type;
		typedef std::size_t result_type;

		result_type operator()(const argument_type& v) const noexcept {
			size_t seed = std::hash<size_t>{}(v.size());

			// stolen from boost::hash_combine
			std::hash<std::string> hasher = {};
			for(auto const& str : v) {
				seed ^= hasher(str) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}

			return seed;
		}
	};
}

namespace docopt {
	inline bool is_empty(const value& v) noexcept {
		return std::holds_alternative<std::monostate>(v);
	}

	struct Pattern;
	struct LeafPattern;

	using PatternList = std::vector<std::shared_ptr<Pattern>>;

	// Utility to use Pattern types in std hash-containers
	struct PatternHasher {
		template <typename P>
		size_t operator()(std::shared_ptr<P> const& pattern) const {
			return pattern->hash();
		}
		template <typename P>
		size_t operator()(P const* pattern) const {
			return pattern->hash();
		}
		template <typename P>
		size_t operator()(P const& pattern) const {
			return pattern.hash();
		}
	};

	// Utility to use 'hash' as the equality operator as well in std containers
	struct PatternPointerEquality {
		template <typename P1, typename P2>
		bool operator()(std::shared_ptr<P1> const& p1, std::shared_ptr<P2> const& p2) const {
			return p1->hash() == p2->hash();
		}
		template <typename P1, typename P2>
		bool operator()(P1 const* p1, P2 const* p2) const {
			return p1->hash() == p2->hash();
		}
	};

	// A hash-set that uniques by hash value
	using UniquePatternSet = std::unordered_set<std::shared_ptr<Pattern>, PatternHasher, PatternPointerEquality>;

	struct Pattern {
		// flatten out children, stopping descent when the given filter returns 'true'
		virtual std::vector<Pattern*> flat(bool (*filter)(Pattern const*)) = 0;

		// flatten out all children into a list of LeafPattern objects
		virtual void collect_leaves(std::vector<LeafPattern*>&) = 0;

		// flatten out all children into a list of LeafPattern objects
		std::vector<LeafPattern*> leaves() {
			std::vector<LeafPattern*> ret;
			collect_leaves(ret);
			return ret;
		}

		// Attempt to find something in 'left' that matches this pattern's spec, and if so, move it to 'collected'
		virtual bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const = 0;

		virtual std::string const& name() const = 0;

		virtual bool hasValue() const noexcept { return false; }

		virtual size_t hash() const = 0;

		virtual ~Pattern() = default;
	};

	struct LeafPattern : Pattern {
		LeafPattern(std::string name, value v = {})
			: fName(std::move(name)),
			fValue(std::move(v))
		{}

		virtual std::vector<Pattern*> flat(bool(*filter)(Pattern const*)) override {
			if(filter(this)) {
				return { this };
			}
			return {};
		}

		virtual void collect_leaves(std::vector<LeafPattern*>& lst) override final {
			lst.push_back(this);
		}

		virtual bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;

		virtual bool hasValue() const noexcept override { return !std::holds_alternative<std::monostate>(fValue); }

		value const& getValue() const noexcept { return fValue; }
		void setValue(value&& v) { fValue = std::move(v); }

		virtual std::string const& name() const noexcept override { return fName; }

		virtual size_t hash() const override {
			size_t seed = typeid(*this).hash_code();
			hash_combine(seed, fName);
			hash_combine(seed, fValue);
			return seed;
		}

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const&) const = 0;

	private:
		std::string fName;
		value fValue;
	};

	namespace {
		std::vector<PatternList> transform(PatternList pattern);
	}

	struct BranchPattern : Pattern {
		BranchPattern(PatternList children = {}) noexcept
			: fChildren(std::move(children))
		{}

		Pattern& fix() {
			UniquePatternSet patterns;
			fix_identities(patterns);
			fix_repeating_arguments();
			return *this;
		}

		virtual std::string const& name() const override {
			throw std::runtime_error("Logic error: name() shouldnt be called on a BranchPattern");
		}

		virtual value const& getValue() const {
			throw std::runtime_error("Logic error: name() shouldnt be called on a BranchPattern");
		}

		virtual std::vector<Pattern*> flat(bool(*filter)(Pattern const*)) override {
			if(filter(this)) {
				return { this };
			}

			std::vector<Pattern*> ret;
			for(auto& child : fChildren) {
				auto sublist = child->flat(filter);
				ret.insert(ret.end(), sublist.begin(), sublist.end());
			}
			return ret;
		}

		virtual void collect_leaves(std::vector<LeafPattern*>& lst) override final {
			for(auto& child : fChildren) {
				child->collect_leaves(lst);
			}
		}

		void setChildren(PatternList children) {
			fChildren = std::move(children);
		}

		PatternList const& children() const noexcept { return fChildren; }

		virtual void fix_identities(UniquePatternSet& patterns) {
			for(auto& child : fChildren) {
				// this will fix up all its children, if needed
				if(auto bp = dynamic_cast<BranchPattern*>(child.get())) {
					bp->fix_identities(patterns);
				}

				// then we try to add it to the list
				const auto inserted = patterns.insert(child);
				if(!inserted.second) {
					// already there? then reuse the existing shared_ptr for that thing
					child = *inserted.first;
				}
			}
		}

		virtual size_t hash() const override {
			size_t seed = typeid(*this).hash_code();
			hash_combine(seed, fChildren.size());
			for(auto const& child : fChildren) {
				hash_combine(seed, child->hash());
			}
			return seed;
		}
	private:
		void fix_repeating_arguments();

	protected:
		PatternList fChildren;
	};

	struct Argument : LeafPattern {
		using LeafPattern::LeafPattern;

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;
	};

	struct Command : Argument {
		Command(std::string name, value v = value{ false })
			: Argument(std::move(name), std::move(v))
		{}

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;
	};

	struct Option final : LeafPattern
	{
		static Option parse(std::string const& option_description);

		Option(std::string shortOption,
		       std::string longOption,
		       int argcount = 0,
		       value v = value{false})
		: LeafPattern(longOption.empty() ? shortOption : longOption,
			      std::move(v)),
		  fShortOption(std::move(shortOption)),
		  fLongOption(std::move(longOption)),
		  fArgcount(argcount)
		{
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

		using LeafPattern::setValue;

		const std::string & longOption() const noexcept { return fLongOption; }
		const std::string & shortOption() const noexcept { return fShortOption; }
		int argCount() const noexcept { return fArgcount; }

		virtual size_t hash() const override {
			size_t seed = LeafPattern::hash();
			hash_combine(seed, fShortOption);
			hash_combine(seed, fLongOption);
			hash_combine(seed, fArgcount);
			return seed;
		}

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;

	private:
		std::string fShortOption;
		std::string fLongOption;
		int fArgcount;
	};

	struct Required : BranchPattern {
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

	struct Optional : BranchPattern {
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override {
			for(auto const& pattern : fChildren) {
				pattern->match(left, collected);
			}
			return true;
		}
	};

	struct OptionsShortcut : Optional {
		using Optional::Optional;
	};

	struct OneOrMore : BranchPattern {
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

	struct Either : BranchPattern {
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

#pragma endregion
#pragma region inline implementations

	namespace {
		inline std::vector<PatternList> transform(PatternList pattern)
		{
			std::vector<PatternList> result;

			std::vector<PatternList> groups;
			groups.emplace_back(std::move(pattern));

			while(!groups.empty()) {
				// pop off the first element
				auto children = std::move(groups[0]);
				groups.erase(groups.begin());

				// find the first branch node in the list
				auto child_iter = std::find_if(children.begin(), children.end(), [](std::shared_ptr<Pattern> const& p) noexcept {
					return dynamic_cast<BranchPattern const*>(p.get());
				});

				// no branch nodes left : expansion is complete for this grouping
				if(child_iter == children.end()) {
					result.emplace_back(std::move(children));
					continue;
				}

				// pop the child from the list
				auto child = std::move(*child_iter);
				children.erase(child_iter);

				// expand the branch in the appropriate way
				if(const Either* const either = dynamic_cast<Either*>(child.get())) {
					// "[e] + children" for each child 'e' in Either
					for(const auto& eitherChild : either->children()) {
						PatternList group = { eitherChild };
						group.insert(group.end(), children.begin(), children.end());

						groups.emplace_back(std::move(group));
					}
				} else if(const OneOrMore* const oneOrMore = dynamic_cast<OneOrMore*>(child.get())) {
					// child.children * 2 + children
					const auto& subchildren = oneOrMore->children();
					PatternList group = subchildren;
					group.insert(group.end(), subchildren.begin(), subchildren.end());
					group.insert(group.end(), children.begin(), children.end());

					groups.emplace_back(std::move(group));
				} else { // Required, Optional, OptionsShortcut
					const BranchPattern* const branch = dynamic_cast<BranchPattern*>(child.get());

					// child.children + children
					PatternList group = branch->children();
					group.insert(group.end(), children.begin(), children.end());

					groups.emplace_back(std::move(group));
				}
			}

			return result;
		}
	}

	inline void BranchPattern::fix_repeating_arguments()
	{
		std::vector<PatternList> either = transform(children());
		for(auto const& group : either) {
			// use multiset to help identify duplicate entries
			std::unordered_multiset<std::shared_ptr<Pattern>, PatternHasher> group_set{ group.begin(), group.end() };
			for(auto const& e : group_set) {
				if(group_set.count(e) == 1)
					continue;

				LeafPattern* leaf = dynamic_cast<LeafPattern*>(e.get());
				if(!leaf) continue;

				bool ensureList = false;
				bool ensureInt = false;

				if(dynamic_cast<Command*>(leaf)) {
					ensureInt = true;
				} else if(dynamic_cast<Argument*>(leaf)) {
					ensureList = true;
				} else if(const Option* const o = dynamic_cast<Option*>(leaf)) {
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
					if(!std::holds_alternative<std::vector<std::string> >(leaf->getValue())) {
						leaf->setValue(value{ newValue });
					}
				} else if(ensureInt) {
					leaf->setValue(value{ 0u });
				}
			}
		}
	}

	inline bool LeafPattern::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const
	{
		auto match = single_match(left);
		if(!match.second) {
			return false;
		}

		left.erase(left.begin() + static_cast<std::ptrdiff_t>(match.first));

		auto same_name = std::find_if(collected.begin(), collected.end(), [&](std::shared_ptr<LeafPattern> const& p) noexcept {
			return p->name() == name();
		});
		if(std::holds_alternative<unsigned int>(getValue())) {
			unsigned int val = 1;
			if(same_name == collected.end()) {
				collected.push_back(match.second);
				match.second->setValue(value{val});
			} else if(std::holds_alternative<unsigned int>((**same_name).getValue())) {
				val += std::get<unsigned int>((**same_name).getValue());
				(**same_name).setValue(value{val});
			} else {
				(**same_name).setValue(value{val});
			}
		} else if(std::holds_alternative<std::vector<std::string> >(getValue())) {
			std::vector<std::string> val;
			if (std::holds_alternative<std::string>(match.second->getValue())) {
				val.push_back(std::get<std::string>(match.second->getValue()));
			} else if (std::holds_alternative<std::vector<std::string> >(match.second->getValue())) {
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

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Argument::single_match(PatternList const& left) const
	{
		std::pair<size_t, std::shared_ptr<LeafPattern>> ret{};

		for(size_t i = 0, size = left.size(); i < size; ++i)
		{
			auto arg = dynamic_cast<Argument const*>(left[i].get());
			if(arg) {
				ret.first = i;
				ret.second = std::make_shared<Argument>(name(), arg->getValue());
				break;
			}
		}

		return ret;
	}

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Command::single_match(PatternList const& left) const
	{
		std::pair<size_t, std::shared_ptr<LeafPattern>> ret{};

		for(size_t i = 0, size = left.size(); i < size; ++i)
		{
			auto arg = dynamic_cast<Argument const*>(left[i].get());
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

	inline Option Option::parse(std::string const& option_description)
	{
		std::string shortOption, longOption;
		int argcount = 0;
		value val{ false };

		const auto double_space = option_description.find("  ");
		auto options_end = option_description.end();
		if(double_space != std::string::npos) {
			options_end = option_description.begin() + static_cast<std::ptrdiff_t>(double_space);
		}

		static const docopt::regex pattern{ "(-{1,2})?(.*?)([,= ]|$)" };
		for(docopt::sregex_iterator i{ option_description.begin(), options_end, pattern, docopt::regex_constants::match_not_null },
			e{};
			i != e;
			++i)
		{
			docopt::smatch const& match = *i;
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
			docopt::smatch match;
			if(docopt::regex_search(options_end, option_description.end(),
				match,
				docopt::regex{ "\\[default: (.*)\\]", docopt::regex::icase }))
			{
				val = match[1].str();
			}
		}

		return { std::move(shortOption),
			std::move(longOption),
			argcount,
			std::move(val) };
	}

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Option::single_match(PatternList const& left) const
	{
		auto thematch = find_if(left.begin(), left.end(), [this](std::shared_ptr<Pattern> const& a) noexcept {
			auto leaf = std::dynamic_pointer_cast<LeafPattern>(a);
			return leaf && this->name() == leaf->name();
		});
		if(thematch == left.end()) {
			return {};
		}
		return { std::distance(left.begin(), thematch), std::dynamic_pointer_cast<LeafPattern>(*thematch) };
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

	inline bool OneOrMore::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const
	{
		assert(fChildren.size() == 1);

		auto l = left;
		auto c = collected;

		bool matched = true;
		size_t times = 0;

		decltype(l) l_;
		bool firstLoop = true;

		while(matched) {
			// could it be that something didn't match but changed l or c?
			matched = fChildren[0]->match(l, c);

			if(matched)
				++times;

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

	inline bool Either::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const
	{
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

#pragma endregion

#endif
