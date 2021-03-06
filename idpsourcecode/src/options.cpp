/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include <sstream>
#include "options.hpp"
#include "errorhandling/error.hpp"
#include <algorithm>
#include "utils/NumericLimits.hpp"
using namespace std;

std::string str(Language choice) {
	switch (choice) {
	case Language::IDP:
		return "idp";
	case Language::IDP2:
			return "idp2";
	case Language::ECNF:
		return "ecnf";
	case Language::TPTP:
		return "tptp";
	case Language::FLATZINC:
		return "flatzinc";
	case Language::ASP:
		return "asp";
	default:
		throw IdpException("Invalid code path.");
		//case Language::CNF:
		//	return "cnf";
		//case Language::LATEX:
		//	return "latex";
	}
}

std::string str(SolverHeuristic choice) {
	switch (choice) {
	case SolverHeuristic::CLASSIC:
		return "classic";
	case SolverHeuristic::VMTF:
		return "vmtf";
	default:
		throw IdpException("Invalid code path.");
	}
}

std::string str(LocalSearchType choice) {
	switch (choice) {
		case LocalSearchType::FIRSTIMPROVE:
			return "firstimprove";
		case LocalSearchType::BESTIMPROVE:
			return "bestimprove";
		case LocalSearchType ::LNS:
			return "LNS";
		case LocalSearchType ::ILS:
			return "ILS";
		case LocalSearchType::TABU:
			return "tabu";
		default:
			throw IdpException("Invalid code path.");
	}
}

std::string str(FullProp choice) {
	switch (choice) {
      case FullProp::ASSUMPTIONS:
        return "assumptions";
      case FullProp::INTERSECTION:
        return "intersection";
      case FullProp::ENUMERATION:
        return "enumeration";
	default:
		throw IdpException("Invalid code path.");
	}
}

std::string str(SymmetryBreaking choice) {
	switch (choice) {
	case SymmetryBreaking::NONE:
		return "none";
	case SymmetryBreaking::STATIC:
		return "static";
	default:
		throw IdpException("Invalid code path.");
	}
}

std::string str(ApproxDef choice) {
	switch (choice) {
	case ApproxDef::NONE:
		return "none";
	case ApproxDef::COMPLETE:
		return "complete";
	case ApproxDef::CHEAP_RULES_ONLY:
		return "cheap";
	case ApproxDef::STRATIFIED:
		return "stratified";
	case ApproxDef::ALL_POSSIBLE_RULES:
		return "all";
	default:
		throw IdpException("Invalid code path.");
	}
}

inline Language operator++(Language& x) {
	return x = (Language) (((int) (x) + 1));
}
inline SolverHeuristic operator++(SolverHeuristic& x) {
	return x = (SolverHeuristic) (((int) (x) + 1));
}
inline FullProp operator++(FullProp& x) {
	return x = (FullProp) (((int) (x) + 1));
}
inline SymmetryBreaking operator++(SymmetryBreaking& x) {
	return x = (SymmetryBreaking) (((int) (x) + 1));
}
inline LocalSearchType operator++(LocalSearchType& x) {
	return x = (LocalSearchType) (((int) (x) + 1));
}
inline ApproxDef operator++(ApproxDef& x) {
	return x = (ApproxDef) (((int) (x) + 1));
}
inline Language operator*(Language& x) {
	return x;
}
inline SolverHeuristic operator*(SolverHeuristic& x){
	return x;
}
inline FullProp operator*(FullProp& x) {
	return x;
}
inline SymmetryBreaking operator*(SymmetryBreaking& x) {
	return x;
}
inline LocalSearchType operator*(LocalSearchType& x) {
	return x;
}
inline ApproxDef operator*(ApproxDef& x) {
	return x;
}
inline bool operator<(Language x, Language y) {
	return (int)x < (int)y;
}
inline bool operator<(FullProp x, FullProp y) {
	return (int)x < (int)y;
}
inline bool operator<(SymmetryBreaking x, SymmetryBreaking y) {
	return (int)x < (int)y;
}
inline bool operator<(ApproxDef x, ApproxDef y) {
	return (int)x < (int)y;
}

template<class T>
std::set<T> possibleValues() {
	std::set<T> s;
	for (auto i = T::FIRST; ; ++i) {
		s.insert(*i);
		if(i==T::LAST) {
			break;
		}
	}
	return s;
}
template<class T>
std::set<std::string> possibleStringValues() {
	std::set<std::string> s;
	auto values = possibleValues<T>();
	for (auto i = values.cbegin(); i != values.cend(); ++i) {
		s.insert(str(*i));
	}
	return s;
}

std::string str(Format choice) {
	switch (choice) {
	case Format::TWOVALUED:
		return "twovalued";
	case Format::THREEVALUED:
		return "threevalued";
	case Format::ALL:
		return "all";
	default:
		return "unknown";
	}
}

Options::Options():_isVerbosity(false) {
	Options(false);
}
// TODO add descriptions to options
Options::Options(bool verboseOptions): _isVerbosity(verboseOptions) {
	std::set<bool> boolvalues { true, false };
	if (verboseOptions) {
		IntPol::createOption(IntType::VERBOSE_CREATE_GROUNDERS, "creategrounders", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_GEN_AND_CHECK, "generatorsandcheckers", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_GROUNDING, "grounding", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_GROUNDING_STATISTICS, "groundingstats", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_SOLVING_STATISTICS, "solvingstats", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_TRANSFORMATIONS, "transformations", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_SOLVING, "solving", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_ENTAILMENT, "entails", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_PROPAGATING, "propagation", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_CREATE_PROPAGATORS, "createpropagators", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_QUERY, "query", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_FUNCDETECT, "functiondetection", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_DEFINITIONS, "calculatedefinitions", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_APPROXDEF, "approxdef", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_LOCALSEARCH, "localsearch", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::VERBOSE_SYMMETRY, "symmetrybreaking", 0, getMaxElem<int>(), 0, PrintBehaviour::PRINT);
	} else {
		auto opt = new Options(true);
		OptionPol::createOption(OptionType::VERBOSITY, "verbosity", { opt }, opt, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::SHOWWARNINGS, "showwarnings", boolvalues, true, PrintBehaviour::DONOTPRINT);
		BoolPol::createOption(BoolType::CPSUPPORT, "cpsupport", boolvalues, true, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::FUNCDETECT, "functiondetection", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::SKOLEMIZE, "skolemize", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::CPGROUNDATOMS, "cpgroundatoms", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::ADDEQUALITYTHEORY, "equalitytheory", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::LAZYHEURISTIC, "lazyheur", boolvalues, false, PrintBehaviour::DONOTPRINT);
		BoolPol::createOption(BoolType::WATCHEDRELEVANCE, "relevancewatched", boolvalues, false, PrintBehaviour::DONOTPRINT);
		BoolPol::createOption(BoolType::SHAREDTSEITIN, "sharedtseitins", { false }, false, PrintBehaviour::DONOTPRINT);
		BoolPol::createOption(BoolType::TRACE, "trace", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::STABLESEMANTICS, "stablesemantics", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::REDUCEDGROUNDING, "reducedgrounding", boolvalues, true, PrintBehaviour::DONOTPRINT);
		BoolPol::createOption(BoolType::AUTOCOMPLETE, "autocomplete", boolvalues, true, PrintBehaviour::DONOTPRINT); // TODO is only used before any lua is executed (during parsing) so not useful for user atm!
		BoolPol::createOption(BoolType::LONGNAMES, "longnames", boolvalues, false, PrintBehaviour::DONOTPRINT);
		BoolPol::createOption(BoolType::CREATETRANSLATION, "createtranslation", { false }, false, PrintBehaviour::DONOTPRINT); // TODO bugged: when grounding: write out the information about which string belongs to which cnf number
		BoolPol::createOption(BoolType::MXRANDOMPOLARITYCHOICE, "randomvaluechoice", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::MXRANDOMINITACTIVITY, "randominitactivity", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::XSB_SHORT_NAMES, "xsbshortnames", boolvalues, true, PrintBehaviour::DONOTPRINT); // Translation to XSB code does not maintain predicate and atom names, but introduces identifiers to minimize communication overhead with XSB
		BoolPol::createOption(BoolType::XSB_SUBSUMPTIVE_TABLING, "xsbsubsumptivetabling", boolvalues, false, PrintBehaviour::DONOTPRINT); // Use the XSB setting for "Subsumptive Tabling". Setting this option TRUE is not reccomended, as it might interfere with some of the built-ins used in our xsb_compiler (see XSB's manual for explanation).
		BoolPol::createOption(BoolType::SHOW_XSB_WARNINGS, "showxsbwarnings", boolvalues, false, PrintBehaviour::DONOTPRINT); // Show XSB warnings
#ifdef WITHXSB
		// only set XSB default to true when the options is available.
		// This makes the getOption(XSB) check consistent, whether it occurs within a #ifdef WITHXSB context or not.
		BoolPol::createOption(BoolType::XSB, "xsb", boolvalues, true, PrintBehaviour::PRINT); // Request to compute definitions as much as possible with xsb
#else
		// When the XSB option is not available, the option still exists, but is always false
		// This is because it is false by default, and it cannot be set to true (see setValue/2 implementation)
		BoolPol::createOption(BoolType::XSB, "xsb", boolvalues, false, PrintBehaviour::DONOTPRINT); // Request to compute definitions as much as possible with xsb
#endif
		BoolPol::createOption(BoolType::REFINE_DEFS_WITH_XSB, "refinedefinitionswithxsb", boolvalues, false, PrintBehaviour::PRINT); // Request to compute definitions as much as possible with xsb
		BoolPol::createOption(BoolType::XSB_COMPILES_PROGRAMS, "xsbusescompiler", boolvalues, false, PrintBehaviour::PRINT); // compile instead of load programs. For efficiency reasons this should be TRUE for large definitions and FALSE for small ones
		BoolPol::createOption(BoolType::EXPANDIMMEDIATELY, "expandimm", boolvalues, false, PrintBehaviour::DONOTPRINT);
		BoolPol::createOption(BoolType::TSEITINDELAY, "tseitindelay", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::SATISFIABILITYDELAY, "satdelay", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::EXISTS_ONLYONELEFT_APPROX, "existsonlyoneleftapprox", boolvalues, false, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::POSTPROCESS_DEFS, "postprocessdefs", boolvalues, true, PrintBehaviour::DONOTPRINT);//Only for internal use for the moment: serves for avoiding loops in bootstrapping
		BoolPol::createOption(BoolType::SPLIT_DEFS, "splitdefs", boolvalues, true, PrintBehaviour::DONOTPRINT);//Only for internal use for the moment: serves for avoiding loops in bootstrapping
		BoolPol::createOption(BoolType::JOIN_DEFS_FOR_XSB, "joindefsforxsb", boolvalues, false, PrintBehaviour::DONOTPRINT);//Only for internal use for the moment: serves for avoiding loops in bootstrapping
		BoolPol::createOption(BoolType::GUARANTEE_NO_REC_NEG, "guaranteenorecursionnegation", boolvalues, false, PrintBehaviour::DONOTPRINT);//Only for internal use for the moment: serves for avoiding loops in bootstrapping
		BoolPol::createOption(BoolType::GUARANTEE_NO_REC_AGG, "guaranteenorecursiveaggs", boolvalues, false, PrintBehaviour::DONOTPRINT);//Only for internal use for the moment

		BoolPol::createOption(BoolType::RELATIVEPROPAGATIONSTEPS, "relativepropsteps", boolvalues, true, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::GROUNDWITHBOUNDS, "groundwithbounds", boolvalues, true, PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::LIFTEDUNITPROPAGATION, "liftedunitpropagation", boolvalues, true, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::NRPROPSTEPS, "nrpropsteps", 0, getMaxElem<int>(), 6, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::LONGESTBRANCH, "longestbranch", 0, getMaxElem<int>(), 13, PrintBehaviour::PRINT);

		BoolPol::createOption(BoolType::ASSUMECONSISTENTINPUT, "assumeconsistentinput", boolvalues, false, PrintBehaviour::PRINT);

		IntPol::createOption(IntType::RANDOMSEED, "seed", 1, getMaxElem<int>(), 91648253, PrintBehaviour::PRINT); // This is the default minisat random seed to (for consistency)
		IntPol::createOption(IntType::NBMODELS, "nbmodels", 0, getMaxElem<int>(), 1, PrintBehaviour::PRINT);

		IntPol::createOption(IntType::LAZYSIZETHRESHOLD, "lazysizelimit", 1, getMaxElem<int>(), 12, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::EXISTSEXPANSIONSTEPS, "existsexpansion", 1, getMaxElem<int>(), 10, PrintBehaviour::PRINT);

		// NOTE: set this to infinity, so he always starts timing, even when the options have not been read in yet.
		IntPol::createOption(IntType::TIMEOUT, "timeout", 0, getMaxElem<int>(), getMaxElem<int>(), PrintBehaviour::PRINT);
		IntPol::createOption(IntType::MXTIMEOUT, "mxtimeout", 0, getMaxElem<int>(), getMaxElem<int>(), PrintBehaviour::PRINT);
		IntPol::createOption(IntType::MEMORYOUT, "memoryout", 0, getMaxElem<int>(), getMaxElem<int>(), PrintBehaviour::PRINT);
		IntPol::createOption(IntType::MXMEMORYOUT, "mxmemoryout", 0, getMaxElem<int>(), getMaxElem<int>(), PrintBehaviour::PRINT);
		IntPol::createOption(IntType::TIMEOUT_ENTAILMENT, "timeout_entailment", 0, getMaxElem<int>(), 2, PrintBehaviour::PRINT);
		IntPol::createOption(IntType::TIMEOUT_LOCALSEARCH, "timeout_localsearch", 0, getMaxElem<int>(), 2, PrintBehaviour::PRINT);

		StringPol::createOption(StringType::PROVERCOMMAND, "provercommand", "", PrintBehaviour::PRINT);
		BoolPol::createOption(BoolType::PROVER_SUPPORTS_TFA, "proversupportsTFA", boolvalues, false, PrintBehaviour::PRINT); // TFA = Typed FO + arithmetic
		StringPol::createOption(StringType::LANGUAGE, "language", possibleStringValues<Language>(), str(Language::IDP), PrintBehaviour::PRINT);
        StringPol::createOption(StringType::OPTIMALPROPAGATION, "optimalpropagation", possibleStringValues<FullProp>(), str(FullProp::DEFAULT),
                PrintBehaviour::PRINT);
		StringPol::createOption(StringType::SYMMETRYBREAKING, "symmetrybreaking", possibleStringValues<SymmetryBreaking>(), str(SymmetryBreaking::NONE),
				PrintBehaviour::PRINT);
		StringPol::createOption(StringType::APPROXDEF, "approxdef", possibleStringValues<ApproxDef>(), str(ApproxDef::NONE),
				PrintBehaviour::PRINT);
		StringPol::createOption(StringType::SOLVERHEURISTIC, "solverheuristic", possibleStringValues<SolverHeuristic>(), str(SolverHeuristic::CLASSIC),
				PrintBehaviour::PRINT);
		//possibleStringValues<LocalSearchType >()
		StringPol::createOption(StringType::LOCALSEARCHTYPE, "localsearchtype", str(LocalSearchType::FIRSTIMPROVE),PrintBehaviour::PRINT);
	}
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::createOption(EnumType type, const std::string& name, const ValueType& lowerbound, const ValueType& upperbound,
		const ValueType& defaultValue, PrintBehaviour visible) {
	_name2type[name] = type;
	auto newoption = new RangeOption<EnumType, ValueType>(type, name, lowerbound, upperbound, visible);
	newoption->setValue(defaultValue);
	if (_options.size() <= (unsigned int) type) {
		_options.resize(type + 1, NULL);
	}
	_options[type] = newoption;
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::createOption(EnumType type, const std::string& name, const std::set<ValueType>& values, const ValueType& defaultValue,
		PrintBehaviour visible) {
	_name2type[name] = type;
	auto newoption = new EnumeratedOption<EnumType, ValueType>(type, name, values, visible);
	newoption->setValue(defaultValue);
	if (_options.size() <= (unsigned int) type) {
		_options.resize(type + 1, NULL);
	}
	_options[type] = newoption;
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::createOption(EnumType type, const std::string& name, const ValueType& defaultValue,
		PrintBehaviour visible) {
	_name2type[name] = type;
	auto newoption = new AnyOption<EnumType, ValueType>(type, name, visible);
	newoption->setValue(defaultValue);
	if (_options.size() <= (unsigned int) type) {
		_options.resize(type + 1, NULL);
	}
	_options[type] = newoption;
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::copyValues(const OptionPolicy<EnumType, ValueType>& opts) {
	for (auto option: _options) {
		if(option!=NULL){
			option->setValue(opts.getValue(option->getName()));
		}
	}
}

template<>
void OptionPolicy<OptionType, Options*>::copyValues(const OptionPolicy<OptionType, Options*>& opts) {
	for (auto option: _options) {
		if(option==NULL){
			continue;
		}
		auto value = option->getValue();
		if(value->isVerbosityBlock()){
			value->copyValues(*opts.getValue(VERBOSITY));
		}
	}
}

void Options::copyValues(const Options& opts) {
	StringPol::copyValues(opts);
	BoolPol::copyValues(opts);
	IntPol::copyValues(opts);
	DoublePol::copyValues(opts);
	OptionPol::copyValues(opts);
}

template<class EnumType, class ConcreteType>
std::string RangeOption<EnumType, ConcreteType>::printOption() const {
	if (TypedOption<EnumType, ConcreteType>::shouldPrint()) {
		std::stringstream ss;
		ss << "\t" << TypedOption<EnumType, ConcreteType>::getName() << " = " << TypedOption<EnumType, ConcreteType>::getValue();
		ss << "\n\t\t => between " << lower() << " and " << upper() << ".\n";
		return ss.str();
	} else {
		return "";
	}
}

template<class EnumType, class ConcreteType>
std::string AnyOption<EnumType, ConcreteType>::printOption() const {
	if (TypedOption<EnumType, ConcreteType>::shouldPrint()) {
		std::stringstream ss;
		ss << "\t" << TypedOption<EnumType, ConcreteType>::getName() << " = " << TypedOption<EnumType, ConcreteType>::getValue() <<"\n";
		return ss.str();
	} else {
		return "";
	}
}

template<>
std::string EnumeratedOption<BoolType, bool>::printOption() const {
	if (TypedOption<BoolType, bool>::shouldPrint()) {
		std::stringstream ss;
		ss << "\t" << TypedOption<BoolType, bool>::getName() << " = " << (TypedOption<BoolType, bool>::getValue() ? "true" : "false");
		ss << "\n\t\t => one of [";
		bool begin = true;
		for (auto i = getAllowedValues().cbegin(); i != getAllowedValues().cend(); ++i) {
			if (not begin) {
				ss << ", ";
			}
			begin = false;
			ss << ((*i) ? "true" : "false");
		}
		ss << "]\n";
		return ss.str();
	} else {
		return "";
	}
}

template<>
std::string EnumeratedOption<StringType, string>::printOption() const {
	if (TypedOption<StringType, string>::shouldPrint()) {
		std::stringstream ss;
		ss << "\t" << TypedOption<StringType, string>::getName() << " = " << TypedOption<StringType, string>::getValue();
		ss << "\n\t\t => one of [";
		bool begin = true;
		for (auto i = getAllowedValues().cbegin(); i != getAllowedValues().cend(); ++i) {
			if (not begin) {
				ss << ", ";
			}
			begin = false;
			ss << "\"" <<*i <<"\"";
		}
		ss << "]\n";
		return ss.str();
	} else {
		return "";
	}
}

template<>
std::string EnumeratedOption<OptionType, Options*>::printOption() const {
	if (TypedOption<OptionType, Options*>::shouldPrint()) {
		std::stringstream ss;
		ss << "\t" << TypedOption<OptionType, Options*>::getName() << " (print for details)\n";
		return ss.str();
	} else {
		return "";
	}
}

template<class EnumType, class ConcreteType>
std::string EnumeratedOption<EnumType, ConcreteType>::printOption() const {
	if (TypedOption<EnumType, ConcreteType>::shouldPrint()) {
		std::stringstream ss;
		ss << "\t" << TypedOption<EnumType, ConcreteType>::getName() << " = " << TypedOption<EnumType, ConcreteType>::getValue();
		ss << "\n\t\t => one of [";
		bool begin = true;
		for (auto i = getAllowedValues().cbegin(); i != getAllowedValues().cend(); ++i) {
			if (not begin) {
				ss << ", ";
			}
			begin = false;
			ss << *i;
		}
		ss << "]\n";
		return ss.str();
	} else {
		return "";
	}
}

Language Options::language() const {
	auto values = possibleValues<Language>();
	auto value = StringPol::getValue(StringType::LANGUAGE);
	for (auto i = values.cbegin(); i != values.cend(); ++i) {
		if (value.compare(str(*i)) == 0) {
			return *i;
		}
	}
	Warning::warning("Encountered unsupported language option, assuming ECNF.\n");
	return Language::ECNF;
}

FullProp Options::fullPropagation() const {
	auto values = possibleValues<FullProp>();
	const std::string& value = StringPol::getValue(StringType::OPTIMALPROPAGATION);
	for (auto i = values.cbegin(); i != values.cend(); ++i) {
		if (value.compare(str(*i)) == 0) {
			return *i;
		}
	}
	Warning::warning("Encountered unsupported language option, assuming default.\n");
	return FullProp::DEFAULT;
}

SymmetryBreaking Options::symmetryBreaking() const {
	auto values = possibleValues<SymmetryBreaking>();
	const std::string& value = StringPol::getValue(StringType::SYMMETRYBREAKING);
	for (auto i = values.cbegin(); i != values.cend(); ++i) {
		if (value.compare(str(*i)) == 0) {
			return *i;
		}
	}
	Warning::warning("Encountered unsupported language option, assuming NONE.\n");
	return SymmetryBreaking::NONE;
}

ApproxDef Options::approxDef() const {
	auto values = possibleValues<ApproxDef>();
	const std::string& value = StringPol::getValue(StringType::APPROXDEF);
	for (auto i = values.cbegin(); i != values.cend(); ++i) {
		if (value.compare(str(*i)) == 0) {
			return *i;
		}
	}
	Warning::warning("Encountered unsupported language option, assuming NONE.\n");
	return ApproxDef::NONE;
}

SolverHeuristic Options::solverHeuristic() const {
	auto values = possibleValues<SolverHeuristic>();
	const std::string& value = StringPol::getValue(StringType::SOLVERHEURISTIC);
	for (auto i = values.cbegin(); i != values.cend(); ++i) {
		if (value.compare(str(*i)) == 0) {
			return *i;
		}
	}
	Warning::warning("Encountered unsupported language option, assuming NONE.\n");
	return SolverHeuristic::CLASSIC;
}

//LocalSearchType Options::localsearchType() const {
//	auto values = possibleValues<LocalSearchType>();
//	const std::string& value = StringPol::getValue(StringType::LOCALSEARCHTYPE);
//    std::clog<<value<<" hi \n";
//	for (auto i = values.cbegin(); i != values.cend(); ++i) {
//		if (value.compare(str(*i)) == 0) {
//			return *i;
//		}
//	}
//	Warning::warning("Encountered unsupported language option, assuming NONE.\n");
//	return LocalSearchType::FIRSTIMPROVE;
//}

std::string Options::localsearchType() const {
	const std::string& value = StringPol::getValue(StringType::LOCALSEARCHTYPE);
    std::string s(value);
    return s;
}


std::string Options::printAllowedValues(const std::string& name) const {
	if (isOptionOfType<int>(name)) {
		return IntPol::printOption(name);
	} else if (isOptionOfType<std::string>(name)) {
		return StringPol::printOption(name);
	} else if (isOptionOfType<double>(name)) {
		return DoublePol::printOption(name);
	} else if (isOptionOfType<bool>(name)) {
		return BoolPol::printOption(name);
	} else {
		return "";
	}
}

bool Options::isOption(const string& optname) const {
	return isOptionOfType<int>(optname) || isOptionOfType<bool>(optname) || isOptionOfType<double>(optname) || isOptionOfType<std::string>(optname)
			|| isOptionOfType<Options*>(optname);
}

template<class OptionList, class StringList>
void getStringFromOption(const OptionList& list, StringList& newlist) {
	for (auto it = list.cbegin(); it != list.cend(); ++it) {
		stringstream ss;
		ss << (*it)->getName() << " = " << (*it)->getValue();
		newlist.push_back(ss.str());
	}
}

ostream& Options::put(ostream& output) const {
	vector<string> optionslines;
	StringPol::addOptionStrings(optionslines);
	IntPol::addOptionStrings(optionslines);
	BoolPol::addOptionStrings(optionslines);
	DoublePol::addOptionStrings(optionslines);
	OptionPol::addOptionStrings(optionslines);

	sort(optionslines.begin(), optionslines.end());
	for (auto i = optionslines.cbegin(); i < optionslines.cend(); ++i) {
		output << *i;
	}

	return output;
}

template<>
bool isVerbosityOption<IntType>(IntType t) {
	return t>=IntType::FIRST_VERBOSE && t<=IntType::LAST_VERBOSE;
}
