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

#include "PrologProgram.hpp"
#include "FormulaClause.hpp"
#include "FormulaClauseBuilder.hpp"
#include "XSBToIDPTranslator.hpp"
#include "FormulaClauseToPrologClauseConverter.hpp"
#include "vocabulary/vocabulary.hpp"
#include "theory/theory.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/ListUtils.hpp"
#include "structure/Structure.hpp"
#include "structure/MainStructureComponents.hpp"
#include "Assert.hpp"

using namespace std;

PrologProgram::~PrologProgram() {
	_definition->recursiveDelete();
}

void PrologProgram::setDefinition(Definition* d) {
	_definition = d;

//	Defined symbols should be tabled
	for (auto symbol : d->defsymbols()) {
		table(symbol);
	}

//  Opens that are threevalued should be tabled
	for (auto symbol : DefinitionUtils::opens(d)) {
		if (not _structure->inter(symbol)->approxTwoValued()) {
			table(symbol);
		}
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 5) {
		clog << "Definition before XSB transformation: " << toString(d) << std::endl;
	}
	FormulaClauseBuilder builder(this, d, _translator);
	FormulaClauseToPrologClauseConverter converter(this, _translator);
	for (auto clause = builder.clauses().begin(); clause != builder.clauses().end(); ++clause) {
		converter.visit(*clause);
	}
	for (auto clause : builder.clauses()) {
		clause->recursiveDelete();
	}
	doReordering();
}

string PrologProgram::getCode() {
	for (auto clause : _clauses) {
		auto plterm = clause->head();
		auto name = plterm->name();
		auto predname = name.append("/").append(toString(plterm->arguments().size()));
		_all_predicates.insert(predname);
	}
	stringstream s;
	s <<":- set_prolog_flag(unknown, fail)." << endl;
// disable compiler specification (because otherwise stuff is printed to the log stream
	s << ":- compiler_options([spec_off])." << endl;
	s << *this << endl;
	return s.str();
}


string PrologProgram::getRanges() {
	stringstream output;
	for (auto it = _sorts.begin(); it != _sorts.end(); ++it) {
		if (_translator->isXSBCompilerSupported(*it)) { _loaded.insert((*it)->name()); }
		if (_loaded.find((*it)->name()) == _loaded.end()) {
			SortTable* st = _structure->inter((*it));
			if (st->isRange() && not st->size().isInfinite()) {
				_loaded.insert((*it)->name());
				_all_predicates.insert(_translator->to_prolog_pred_and_arity(*it));
				auto sortname = _translator->to_prolog_sortname((*it));
				output << sortname << "(X) :- var(X), between(" << _translator->to_prolog_term(st->first()) << ","
						<< _translator->to_prolog_term(st->last()) << ",X).\n";
				output << sortname << "(X) :- nonvar(X), X >= " << _translator->to_prolog_term(st->first()) << ", X =< "
						<< _translator->to_prolog_term(st->last()) << ".\n";
			}
		}
	}
	return output.str();
}

string PrologProgram::getFacts() {
	stringstream output;

	// Always consider built-in sorts
	for (auto name2sort : Vocabulary::std()->getSorts()) {
		_sorts.insert(name2sort.second);
	}

	auto openSymbols = DefinitionUtils::opens(_definition);

	for (auto symbol : openSymbols) {
		printOpenSymbol(symbol,output);
	}
	
	auto sortsize = 0;
	while (sortsize < _sorts.size()) {
		sortsize = _sorts.size(); // flow control: during execution, more sorts can be added. Thus, if the size increases again, all sorts must be iterated again as well
		for (auto sort : _sorts) {
			if (_loaded.find(sort->name()) == _loaded.end()) { // advance until first one that hasn't been loaded
				printSort(sort,output);
			}
		}
	}
	return output.str();
}
void PrologProgram::printSort(const Sort* sort, std::ostream& output) {
	if (sort->isConstructed()) {
		printConstructedTypesRules(sort,output);
		_loaded.insert(sort->name());
		_all_predicates.insert(_translator->to_prolog_pred_and_arity(sort));
	} else {
		SortTable* st = _structure->inter(sort);
		if (not st->isRange() && st->finite()) {
			_loaded.insert(sort->name());
			_all_predicates.insert(_translator->to_prolog_pred_and_arity(sort));
			auto factname = _translator->to_prolog_sortname(sort);
			for (auto tuple = st->begin(); !tuple.isAtEnd(); ++tuple) {
				output << factname << "(";
				printDomainElement((*tuple).front(),output);
				output << ").\n";
			}
		}
	}
}

void PrologProgram::printAsFacts(string symbol_name, PFSymbol* symbol, std::ostream& ss) {
	if (_structure->inter(symbol)->approxTwoValued()) {
		print2valFacts(symbol_name,symbol,ss);
	} else {
		print3valFacts(symbol_name,symbol,ss);
	}
}

void PrologProgram::printOpenSymbol(PFSymbol* symbol, std::ostream& output) {
	if (VocabularyUtils::isConstructorFunction(symbol)) {
		Assert(isa<Function>(*symbol));
		return;
	}
	if (_translator->isXSBBuiltIn(symbol->nameNoArity()) or
			_translator->isXSBCompilerSupported(symbol)) {
		return;
	}
	if (not hasElem(_sorts, [&](const Sort* sort){return sort->pred() == symbol;}) ) {
		_all_predicates.insert(_translator->to_prolog_pred_and_arity(symbol));
		printAsFacts(_translator->to_prolog_term(symbol), symbol, output);
	}
}

void PrologProgram::print2valFacts(string symbol_name, PFSymbol* symbol, std::ostream& ss) {
	auto certainly_true = _structure->inter(symbol)->ct();
	for (auto it = certainly_true->begin(); !it.isAtEnd(); ++it) {
		ss << symbol_name;
		ElementTuple tuple = *it;
		printTuple(tuple,ss);
		ss << ".\n";
	}
}

void PrologProgram::print3valFacts(string symbol_name, PFSymbol* symbol, std::ostream& ss) {
	auto certainly_true = _structure->inter(symbol)->ct(); // TODO: This can probably be done more efficiently by a creeping iterator method
	auto possibly_true = _structure->inter(symbol)->pt();
	for (auto it = possibly_true->begin(); !it.isAtEnd(); ++it) {
		ss << symbol_name;
		ElementTuple tuple = *it;
		printTuple(tuple,ss);
		if (not certainly_true->contains(tuple)) {
			// If the tuple is not in the certainly true table, it is unknown
			// Unknown symbols needs to be printed as follows:
			//   symbol(arg1,..,argn) :- undef.
			ss << " :- undef";
		}
		ss << ".\n";
	}
}

void PrologProgram::printTuple(const ElementTuple& tuple, std::ostream& ss) {
	if (tuple.size()>0){
		ss << "(";
		printList(ss, tuple, ",", [&](std::ostream& output, const DomainElement* domelem){
			printDomainElement(domelem,output);
		}, true);
		ss <<")";
	}
}

void PrologProgram::printDomainElement(const DomainElement* domelem, std::ostream& ss) {
	ss << _translator->to_prolog_term(domelem); 
}
void PrologProgram::printConstructedTypesRules(const Sort* sort, std::ostream& ss) {
	for (auto constructor : sort->getConstructors()) {
		printConstructorRules(sort,constructor,ss);
	}
}

void PrologProgram::printConstructorRules(const Sort* sort, Function* constructor, std::ostream& ss) {
	int nr = 0;
	ss << _translator->to_prolog_sortname(sort) << "(";
	ss << _translator->to_prolog_term(constructor);
	if (constructor->insorts().size() > 0) {
		ss << "(";
		printList(ss, constructor->insorts(), ",", [&](std::ostream& output, const Sort*){
			output << "V" << nr++;
		}, true);
		ss << ")";
	}
	ss << ")";
	if (constructor->insorts().size() > 0) {
		ss << " :- ";
		nr = 0;
		printList(ss, constructor->insorts(), ", ", [&](std::ostream& output, const Sort* sort){
			output << _translator->to_prolog_sortname(sort) << "(V" << nr++ << ")";
		}, true);
	}
	ss << "." << endl;
	for (auto sort : constructor->insorts()) {
		addDomainDeclarationFor(sort);
	}
}



void PrologProgram::table(PFSymbol* pt) {
	_tabled.insert(pt);
}

void PrologProgram::addDomainDeclarationFor(Sort* s) {
	_sorts.insert(s);
}

void PrologProgram::doReordering() {
	std::set<std::string> symbolNamesMarkedFinite;
	std::set<std::string> definedSymbols;

	for (auto r : _clauses) {
		auto s = r->head()->namep();
		definedSymbols.insert(std::string(*s));
	}

	auto opens = DefinitionUtils::approxTwoValuedOpens(_definition,_structure);
	for (auto s : opens) {
		if(not s->builtin() && _structure->inter(s)->pt()->approxFinite()){
			symbolNamesMarkedFinite.insert(_translator->to_prolog_term(s));
		}
	}

	bool changed = true;
	while (changed) {
		// Go over all prolog clauses and reorder body: finitely marked symbols move forward, but never before an earlier marked finite symbol

		for(auto r: _clauses){
			r->reorder_MoveFront(symbolNamesMarkedFinite);
		}


		changed = false;
		std::set<std::string> unsafe;

		for(auto r: _clauses){
			if(r->isUnsafe(symbolNamesMarkedFinite)){
				unsafe.insert(std::string(*r->head()->namep()));
			}
		}

		for (auto p : definedSymbols) {
			if (not contains(unsafe,p) && not contains(symbolNamesMarkedFinite,p)){
				symbolNamesMarkedFinite.insert(p);
				changed=true;
			}
		}
	}

	for (auto r : _clauses) {
		r->moveIODependenciesForward();
	}
}



std::ostream& operator<<(std::ostream& output, const PrologProgram& p) {
//	Generating table statements
	for (auto it = p._tabled.begin(); it != p._tabled.end(); ++it) {
		output << ":- table " << p._translator->to_prolog_pred_and_arity(*it) << ".\n";
	}
//	Generating clauses
	for (auto it = p._clauses.begin(); it != p._clauses.end(); ++it) {
		output << **it << "\n";
	}
	return output;
}

void PrologClause::reorder() {
	_body.sort(PrologTerm::term_ordering);
}

void PrologClause::reorder_MoveFront(std::set<std::string>& frontSymbols){
	_body.sort([&frontSymbols](PrologTerm* first, PrologTerm* second)->bool{return contains(frontSymbols,std::string(*first->namep())) && not contains(frontSymbols,std::string(*second->namep()));} );
}

bool PrologClause::isUnsafe(std::set<std::string>& safeSymbols){
	std::set<PrologVariable*> allvars{};
	std::set<PrologVariable*> safeVars{};

	allvars.insert(_head->variables().begin(),_head->variables().end());

	for(auto t: _body){
		if(contains(safeSymbols,std::string(*t->namep()))){
			safeVars.insert(t->variables().begin(),t->variables().end());
		}
		allvars.insert(t->variables().begin(),t->variables().end());
	}

	return(allvars.size()>safeVars.size());
}

void PrologClause::moveIODependenciesForward(){
	list<PrologTerm*> ioterms;

	//collect all ioterms and remove them from list
	for(auto it = _body.begin(); it != _body.end(); ){
		auto t=*it;
		if(not t->outputvarsToCheck().empty()){
			ioterms.push_back(t);
			it++;
			_body.remove(t);
		}
		else{
			it++;
		}
	}

	set<PrologVariable*> boundVars;
	for(auto it = _body.begin(); it != _body.end(); it++ ){
		bool foundOne = false;
		//Search an ioterm to insert on this place
		for(auto ioT: ioterms){
			bool decided = true;
			for(auto v: ioT->inputvarsToCheck()){
				if(not contains(boundVars,v)){
					decided = false;
					break;
				}
			}
			if(decided){
				auto outvars = ioT->outputvarsToCheck();
				boundVars.insert(outvars.begin(), outvars.end());
				it = _body.insert(it,ioT);
				ioterms.remove(ioT);
				foundOne = true;
				break;
			}
		}
		if(foundOne){
			continue;
		}

		//If none is found: continue
		auto t = *it;
		auto tVars = t->variables();
		boundVars.insert(tVars.begin(), tVars.end());

	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (auto ioT : ioterms) {
			bool decided = true;
			for (auto v : ioT->inputvarsToCheck()) {
				if (not contains(boundVars, v)) {
					decided = false;
					break;
				}
			}
			if (decided) {
				auto outvars = ioT->outputvarsToCheck();
				boundVars.insert(outvars.begin(), outvars.end());
				_body.push_back(ioT);
				ioterms.remove(ioT);
				changed = true;
				break;
			}
		}
		if (changed) {
			continue;
		}
	}

	for (auto t : ioterms) {
		_body.push_back(t);
	}


}




std::ostream& operator<<(std::ostream& output, const PrologClause& pc) {
	output << *(pc._head);
	if (!pc._body.empty()) {
		output << " :- ";
		auto instantiatedAndCheckedVars = new std::set<PrologVariable*>();
		for (std::list<PrologTerm*>::const_iterator it = (pc._body).begin(); it != (pc._body).end(); ++it) {
			if (it != (pc._body).begin()) {
				output << ", ";
			}
			// Add type generators for the necessary variables
			for (auto var = (*it)->inputvarsToCheck().begin(); var != (*it)->inputvarsToCheck().end(); ++var) {
				if(instantiatedAndCheckedVars->find(*var) == instantiatedAndCheckedVars->end()) {
					output << *(*var)->instantiation() << ", ";
					instantiatedAndCheckedVars->insert(*var);
				}
			}
			output << (**it);
			// Add type check to the necessary variables
			for (auto var = (*it)->outputvarsToCheck().begin(); var != (*it)->outputvarsToCheck().end(); ++var) {
				if(instantiatedAndCheckedVars->find(*var) == instantiatedAndCheckedVars->end()) {
					output << ", " << *(*var)->instantiation();
					instantiatedAndCheckedVars->insert(*var);
				}
			}
			// All variables of the printed call have to be checked - they could be over a stricter type than the call
			for (auto var = (*it)->variables().begin(); var != (*it)->variables().end(); ++var) {
				if(instantiatedAndCheckedVars->find(*var) == instantiatedAndCheckedVars->end()) {
					output << ", " << *(*var)->instantiation();
					instantiatedAndCheckedVars->insert(*var);
				}
			}
		}
		for (auto it = (pc._head)->variables().begin(); it != (pc._head)->variables().end(); ++it) {
			if(instantiatedAndCheckedVars->find(*it) == instantiatedAndCheckedVars->end()) {
				output << ", " << *(*it)->instantiation();
			}
		}
		delete(instantiatedAndCheckedVars);
	}
	output << ".";
	return output;
}


string PrologProgram::getXSBBuiltinCode(string& errorFileLocation) {
	stringstream ss;
	ss << R"SOMEDELIM(
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   Note: When adding new predicates to this file as a result of them being
%   'built-in' in IDP, make sure to also adapt the 
%   XSBToIDPTranslator::isXSBCompilerSupported procedure
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% ixexponential(Solution,Base,Power)
% Base case: right hand side is known
ixexponential(Solution,Base,Power) :-
    nonvar(Base),
    nonvar(Power),
    TMP is Base ** Power,
    ixsame_number(TMP,Solution).

% Solution equals the base and is 1 -> power could be anything.
ixexponential(1,1,Power) :-
    ixint(Power).

% Solution equals the base and is not 1 -> power has to be one.
ixexponential(Base,Base,1) :-
    nonvar(Base),
    ixdifferent_number(Base,1). % Rule out double answer generation for previous case

% ixdivision(Solution,Numerator,Denominator)
% Represents the built-in "Solution is Numerator/Denominator"
% Handle special cases of this expression first: X is Y/X, Y known
ixdivision(Denominator,Numerator,Denominator) :-
    nonvar(Numerator),
    var(Denominator),
    Denominator is sqrt(Numerator).
ixdivision(Denominator,Numerator,Denominator) :-
    var(Numerator),
    nonvar(Denominator),
    ixdifferent_number(Denominator,0),
    Numerator is Denominator*Denominator.

% Handle special cases of this expression first: X is X/Y, Y known 
% -> infinite generator
ixdivision(Numerator,Numerator,1) :-
    var(Numerator),
    throw_infinite_type_generation_error.

% Handle special cases of this expression first: X is X/Y, Y known, with Y ~= 1 
% -> this always fails
ixdivision(Numerator,Numerator,Denominator) :-
    nonvar(Denominator),
    ixdifferent_number(Denominator,1),
    fail.

% Handle special cases of this expression first: O is O/Y with Y known
% -> succeeds 
ixdivision(Numerator,Numerator,Denominator) :-
    nonvar(Numerator),
    ixsame_number(Numerator,0),
    nonvar(Denominator),
    ixdifferent_number(Denominator,0).

% Only normal cases left: X is Y/Z with Y and Z variables
ixdivision(Solution,Numerator,Denominator) :-
    nonvar(Numerator),
    nonvar(Denominator),
    TMP is Numerator / Denominator,
    ixsame_number(TMP,Solution).


ixabs(X,Y) :- 
    number(X),
    Y is abs(X).

ixabs(X,_) :-
    var(X),
    throw_infinite_type_generation_error.

ixsum(List,Sum) :- ixsum(List,Sum,0).
ixsum([],X,X).
ixsum([H|T],Sum,Agg) :- Agg2 is Agg + H, ixsum(T,Sum,Agg2).

ixprod(List,Prod) :- ixprod(List,Prod,1).
ixprod([],X,X).
ixprod([H|T],Prod,Agg) :- Agg2 is Agg * H, ixprod(T,Prod,Agg2).

ixcard(List,Card) :- length(List,Card).

ixmin([X|Rest],Min) :- ixmin(Rest,Min,X).

ixmin([],Min,Min).
ixmin([X|Rest],Min,TmpMin) :-
    X < TmpMin,
    ixmin(Rest,Min,X).
ixmin([X|Rest],Min,TmpMin) :-
    X >= TmpMin,
    ixmin(Rest,Min,TmpMin).

ixmax([X|Rest],Max) :- ixmax(Rest,Max,X).
ixmax([],Max,Max).
ixmax([X|Rest],Max,TmpMax) :-
    X > TmpMax,
    ixmax(Rest,Max,X).
ixmax([X|Rest],Max,TmpMax) :-
    X =< TmpMax,
    ixmax(Rest,Max,TmpMax).

% ixforall(Generator,Verifier)
% Only succeeds if for every succeeding call to Generator, the Verifier also succeeds
%
% Implementation-wise, tables:not_exists/1 is used for handling negation because it 
% supports the mixed usage of tabled and non-tabled predicates (as well as the
% conjunction/disjunction of these).
%
% Additionaly, this has to be surrounded by builtin call_tv([...], true), because
% otherwise, the answer may be tagged as "undefined", even it if does not show this
% when printing the answer. An example of this is the following program, in which
% ?- p. 
% is answered as "undefined", even though calling the body of the second rule for p
% is answered as "false".
%
%   :- set_prolog_flag(unknown, fail).
%   :- table p/0, d/1, c/0, or/1.
% 
%   p :- p.
%   p :- tables:not_exists((type(X), tables:not_exists(or(X)))).
% 
%   or(2).
%   or(_) :- c.
% 
%   c :- d(X), \+ 1 = X.
% 
%   d(1) :- p.
% 
%   type(1).
%   type(2).
%
ixforall(CallA, CallB) :-
    tables:not_exists((call(CallA), tables:not_exists(CallB))).

% ixthreeval_findall(Var,Query,Ret)
%   1: Gather all "true" answers
%   2: Gather all "undefined" answers of the Query
%   3: Append each possible subset of "undefined" answers list to the "true" 
%      answers list
%   4: Introduce loop to make return tuple undefined if some "undefined" answers
%      were added to the Ret list (generate_CT_or_U_answers/1 is used for this)
ixthreeval_findall(Var,Query,Ret) :-
    findall(Var,call_tv(Query,true),CTList),
    findall(Var,call_tv(Query,undefined),Ulist),
    ixsubset(Ulist,S),
    append(S,CTList,Ret),
    generate_CT_or_U_answer(Ulist,S).

generate_CT_or_U_answer([],_).
generate_CT_or_U_answer([_|_],_) :- undef.

:- table undef/0.
undef :- tnot(undef).

ixsubset([],[]).
ixsubset([E|Tail],[E|NTail]) :-
    ixsubset(Tail,NTail).
ixsubset([_|Tail],NTail) :-
    ixsubset(Tail,NTail).

ixint(X) :- 
    nonvar(X),
    ROUNDEDNUMBER is round(X),
    ZERO is ROUNDEDNUMBER - X,
    \+ 0 < ZERO,
    \+ 0 > ZERO.

ixint(X) :- 
    var(X),
    throw_infinite_type_generation_error.

ixfloat(X) :- 
    nonvar(X),
    number(X).

ixfloat(X) :- 
    var(X),
    throw_infinite_type_generation_error.

ixnat(X) :- 
    nonvar(X),
    X >= 0,
    ixint(X).

ixnat(X) :- 
    var(X),
    throw_infinite_type_generation_error.

% TODO: leaves through too much!
ixchar(X) :- 
    nonvar(X),
    atomic(X). % Possible todo - maintain strings during translation and check for is_charlist(X,1) (of size 1) here

ixchar(X) :- 
    var(X),
    throw_infinite_type_generation_error.

ixstring(X) :- 
    nonvar(X),
    atomic(X). % Possible todo - maintain strings during translation and check for is_charlist/1 here

ixstring(X) :-
    var(X),
    throw_infinite_type_generation_error.

% First argument has to be instantiated
% Second argument can be output variable or instantiated
ixsame_number(X,Y) :-
    ixconvert_to_int(X,X1),
    X1 = Y.

% First argument has to be instantiated
% Second argument has to be instantiated
ixdifferent_number(X,Y) :-
    ixconvert_to_int(X,X1),
    ixconvert_to_int(Y,Y1),
    X1 \== Y1.

ixconvert_to_int(Num,Int) :-
    nonvar(Num),
    ixint(Num), 
    Int is round(Num).

ixconvert_to_int(Num,Int) :-
    nonvar(Num),
    ixfloat(Num), 
    Int = Num.

throw_infinite_type_generation_error :-
    write_to_errorfile('Trying to generate an infinite type with XSB\ntry to rerun with stdoptions.xsb=false to see if that works.').

% Error throwing hack: write error to file at hard-coded location
% Make sure that this is the same location that is checked in XSBInterface::checkForXSBError() !
write_to_errorfile(Msg) :-
    error_file_location(ERRFILEURL),
    open(file(ERRFILEURL),write,S),
    write(S,Msg),
    close(S).
error_file_location(')SOMEDELIM";
	ss << errorFileLocation << "')." << endl;
	return ss.str();
}
