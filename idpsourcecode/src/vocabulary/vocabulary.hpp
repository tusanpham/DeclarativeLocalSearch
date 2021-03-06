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

#pragma once

#include <vector>
#include <set>
#include <map>
#include <ostream>
#include "parseinfo.hpp"
#include "commontypes.hpp"
#include "common.hpp"


/*********
 * Sorts
 ********/

class Predicate;
class Vocabulary;
class SortTable;
class Function;
class Structure;

class Sort {
private:
	std::string _name; //!< Name of the sort
	std::set<const Vocabulary*> _vocabularies; //!< All vocabularies the sort belongs to
	std::set<Sort*> _parents; //!< The parent sorts of the sort in the sort hierarchy
	std::set<Sort*> _children; //!< The children of the sort in the sort hierarchy
	Predicate* _pred; //!< The predicate that corresponds to the sort
	ParseInfo _pi; //!< The place where the sort was declared
	SortTable* _interpretation; //!< The interpretation of the sort if it has a fixed interpretation.
								//!< A null-pointer otherwise.
	bool sortConstructed;
	std::vector<Function*> _constructors; //!< The functions constructing this sort

	void removeParent(Sort* p); //!< Removes parent p
	void removeChild(Sort* c); //!< Removes child c

	void generatePred(); //!< Generate the predicate that corresponds to the sort

protected:
	void setPred(Predicate* p) {
		_pred = p;
	}

public:
	Sort(SortTable* inter = NULL); //!< Create an internal sort
	Sort(const std::string& name, SortTable* inter = NULL); //!< Create an internal sort
	Sort(const std::string& name, const ParseInfo& pi, SortTable* inter = NULL); //!< Create a user-declared sort

	// NOTE: only public for testing purposes, a sort deletes itself when it has no more vocabularies.
	virtual ~Sort();

	void addParent(Sort* p); //!< Adds p as a parent. Also adds this as a child of p.
	void addChild(Sort* c); //!< Adds c as a child. Also add this as a parent of c.

	// Inspectors
	const std::string& name() const{
		return _name;
	}
	const ParseInfo& pi() const{
		return _pi;
	}
	Predicate* pred() const{
		return _pred;
	}
	const std::set<Sort*>& parents() const{
		return _parents;
	}
	const std::set<Sort*>& children() const{
		return _children;
	}
	std::set<Sort*> ancestors(const Vocabulary* v = NULL) const; //!< Returns the ancestors of the sort
	std::set<Sort*> descendents(const Vocabulary* v = NULL) const; //!< Returns the descendents of the sort


	void setConstructed(bool constructed);
	bool isConstructed() const; //!< True iff the sort is generated by a set of contructor functions
	void addConstructor(Function* func);
	const std::vector<Function*>& getConstructors() const {
		return _constructors;
	}

	bool hasFixedInterpretation() const; //!< True iff the sort has a fixed interpretation independent of a given structure
	bool builtin() const; //!< True iff the sort has a fixed interpretation independent of a given structure TODO:DEPRECATED
	SortTable* interpretation() const; //!< Returns the interpretation for built-in sorts
	const std::set<const Vocabulary*>& getVocabularies() const;


	void removeVocabulary(const Vocabulary*); //!< Removes a vocabulary from the list of vocabularies
	void addVocabulary(const Vocabulary*); //!< Add a vocabulary to the list of vocabularies

	const Vocabulary* firstVocabulary(){
		return *(_vocabularies.cbegin());
	}

	virtual std::vector<const Sort*> getSortsForTable() const {
		return std::vector<const Sort*> { this };
	}

	std::ostream& put(std::ostream&) const;
};

bool operator<(const Sort&, const Sort&);

SortTable* getConstructedInterpretation(Sort* sort, const Structure* struc);

class UnionSort: public Sort {
private:
	std::vector<Sort*> sorts;
	~UnionSort() {
	}

public:
	UnionSort(const std::vector<Sort*>& sorts);

	virtual std::vector<Sort*> getSortsForTable() {
		return sorts;
	}
};

std::ostream& operator<<(std::ostream&, const Sort&);

namespace SortUtils {
/** Return the unique nearest common ancestor of two sorts */
Sort* resolve(Sort* a, Sort* b, const Vocabulary* voc = NULL);

/** Return true iff sort a is a subsort of sort b */
bool isSubsort(Sort* a, Sort* b, const Vocabulary* voc = NULL);
}

/**************
 * Variables
 *************/

/**
 *	\brief	Class to represent variables.
 */
class Variable {
private:
	std::string _name; //!< Name of the variable
	Sort* _sort; //!< Sort of the variable (0 if the sort is not derived)
	ParseInfo _pi; //!< The place where the variable was quantified

public:
	Variable(const std::string& name, Sort* sort, const ParseInfo& pi);
	//!< Constructor for a named variable
	Variable(Sort* s);
	//!< Constructor for an internal variable

	~Variable(); //!< Destructor

	void sort(Sort* s); //!< Change the sort of the variable

	const std::string& name() const; //!< Returns the name of the variable
	Sort* sort() const; //!< Returns the sort of the variable (null-pointer if the variable has no sort)
	const ParseInfo& pi() const; //!< Returns the parse info of the variable

	std::ostream& put(std::ostream&) const;
};

std::ostream& operator<<(std::ostream&, const Variable&);

namespace VarUtils {
/**
 * Make a vector of fresh variables of given sorts.
 */
std::vector<Variable*> makeNewVariables(const std::vector<Sort*>&);
}

/**********************************
 * Predicate and function symbols
 *********************************/

class Structure;

enum SymbolType {
	ST_NONE,
	ST_CT,
	ST_CF,
	ST_PT,
	ST_PF
};

/** 
 *	\brief	Abstract base class to represent predicate and function symbols
 */
class PFSymbol {
private:
	std::string _name; //!< Name of the symbol (ending with the /arity)
	ParseInfo _pi; //!< The place where the symbol was declared
	std::set<const Vocabulary*> _vocabularies; //!< All vocabularies the symbol belongs to
	std::vector<Sort*> _sorts; //!< Sorts of the arguments.
	// IMPORTANT: for overloaded symbols, sorts can be NULL
	//!< For a function symbol, the last sort is the output sort.
	bool _infix; //!< True iff the symbol is infix

	std::map<SymbolType, Predicate*> _derivedsymbols; //!< The symbols this<ct>, this<cf>, this<pt>, and this<pf>

protected:
	void setName(const std::string& name) {
		_name = name;
	}
	void addSort(Sort* s) {
		_sorts.push_back(s);
	}
	virtual ~PFSymbol();

public:
	PFSymbol(const std::string& name, std::size_t nrsorts, bool infix = false);
	PFSymbol(const std::string& name, const std::vector<Sort*>& sorts, bool infix = false);
	PFSymbol(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi, bool infix = false);

	virtual bool removeVocabulary(const Vocabulary*); //!< Removes a vocabulary from the list of vocabularies
	virtual void addVocabulary(const Vocabulary*); //!< Add a vocabulary to the list of vocabularies

	const std::string& name() const; //!< Returns the name of the symbol (ends on /arity)
	std::string fqn_name() const; //!< Returns the full name for this symbol (namespace+symbolname+sort)
	std::string nameNoArity() const; //!< Returns the name of the symbol (without /arity)
	const ParseInfo& pi() const; //!< Returns the parse info of the symbol

	virtual unsigned int arity() const = 0; // Returns the arity

	// IMPORTANT: for overloaded symbols, sorts can be NULL
	std::size_t nrSorts() const; // Returns the number of sorts of the symbol (arity for predicates, arity+1 for functions)
	Sort* sort(std::size_t n) const; //!< Returns the n'th sort of the symbol
	const std::vector<Sort*>& sorts() const {
		return _sorts;
	}
	bool infix() const; //!< True iff the symbol is infix

	bool hasVocabularies() const; //!< Returns true iff the symbol occurs in a
								  //!< vocabulary
	const std::set<const Vocabulary*>& getVocabularies() const {
		return _vocabularies;
	}

	Predicate* derivedSymbol(SymbolType); //!< Return the derived symbol of the given type
	std::vector<unsigned int> argumentNrs(const Sort*) const; //!< Returns the numbers of the arguments where this
															  //!< PFSymbol ranges over the given sort

	virtual bool isFunction() const = 0;
	virtual bool isPredicate() const = 0;

	virtual bool builtin() const = 0; //!< Returns true iff the symbol is built-in
	virtual bool isNonConstructorBuiltin() const = 0; //!< Returns true iff the symbol is built-in
	virtual bool overloaded() const = 0; //!< Returns true iff the symbol is in fact a set of overloaded
										 //!< symbols
	virtual std::set<Sort*> allsorts() const = 0; //!< Return all sorts that occur in the (overloaded) symbol(s)

// Disambiguate overloaded symbols
	virtual PFSymbol* resolve(const std::vector<Sort*>&) = 0;
	virtual PFSymbol* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0) = 0;

	virtual std::ostream& put(std::ostream&) const = 0;

	friend class Vocabulary;
};

std::ostream& operator<<(std::ostream&, const PFSymbol&);

class PredGenerator;
class PredInter;
class PredInterGenerator;
class Structure;

/**
 * \brief	Class to represent predicate symbols
 */
class Predicate: public PFSymbol {
private:
	SymbolType _type; //!< The type of the symbol
	PFSymbol* _parent; //!< The symbol this predicate is derived from.
					   //!< Nullpointer if _type == ST_NONE.
	PredInterGenerator* _interpretation; //!< The interpretation if the predicate is built-in, a null-pointer
										 //!< otherwise.
	PredGenerator* _overpredgenerator; //!< Generates new built-in, overloaded predicates.
									   //!< Null-pointer if the predicate is not overloaded.

public:
	Predicate(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi = ParseInfo{}, bool infix = false);
	Predicate(const std::vector<Sort*>& sorts); //!< constructor for tseitin predicates
	Predicate(const std::string& name, const std::vector<Sort*>& sorts, PredInterGenerator* inter, bool infix);
	Predicate(PredGenerator* generator);

	~Predicate();

	virtual bool isPredicate() const {
		return true;
	}
	virtual bool isFunction() const {
		return false;
	}

	bool removeVocabulary(const Vocabulary*); //!< Removes a vocabulary from the list of vocabularies
	void addVocabulary(const Vocabulary*); //!< Add a vocabulary to the list of vocabularies
	void type(SymbolType, PFSymbol* parent); //!< Set the type and parent of the predicate

	SymbolType type() const {
		return _type;
	}
	PFSymbol* parent() const {
		return _parent;
	}
	unsigned int arity() const {
		return sorts().size();
	}
	bool builtin() const;
	bool isNonConstructorBuiltin() const;
	
	bool overloaded() const;
	std::set<Sort*> allsorts() const;

	// Built-in symbols
	PredInter* interpretation(const Structure*) const;


	// Overloaded symbols
	bool contains(const Predicate* p) const;
	Predicate* resolve(const std::vector<Sort*>&);
	Predicate* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0);
	std::set<Predicate*> nonbuiltins(); //!< Returns the set of predicates that are not builtin
										//!< and that are overloaded by 'this'.

	std::ostream& put(std::ostream&) const;

	friend class Sort;
	friend class Vocabulary;
	friend class PredGenerator;
};

std::ostream& operator<<(std::ostream&, const Predicate&);

namespace PredUtils {
/**
 * Return a new overloaded predicate containing the two given predicates
 */
Predicate* overload(Predicate* p1, Predicate* p2);

/**
 * Return a new overloaded predicate containing the given predicates
 */
Predicate* overload(const std::set<Predicate*>&);


/**
 * Return true if and only if the predicate is a type predicate
 */
bool isTypePredicate(const Predicate*);

}

class FuncGenerator;
class FuncInter;
class FuncInterGenerator;
class FuncTable;

class Function: public PFSymbol {
private:
	bool _partial; //!< true iff the function is declared as partial function
	std::vector<Sort*> _insorts; //!< the input sorts of the function symbol
	Sort* _outsort; //!< the output sort of the function symbol
	FuncInterGenerator* _interpretation; //!< the interpretation if the function is built-in, a null-pointer
										 //!< otherwise
	FuncGenerator* _overfuncgenerator; //!< generates new built-in, overloaded functions.
									   //!< Null-pointer if the function is not overloaded.
	unsigned int _binding; //!< Binding strength of infix functions.

	bool _isConstructor;

public:
	Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, const ParseInfo& pi = ParseInfo(), bool isConstructor = false);
	Function(const std::string& name, const std::vector<Sort*>& sorts, FuncInterGenerator*, unsigned int binding);
	Function(const std::vector<Sort*>& is, Sort* os, const ParseInfo& pi, unsigned int binding = 0);
	Function(FuncGenerator*);

	~Function();

	virtual bool isPredicate() const {
		return false;
	}
	virtual bool isFunction() const {
		return true;
	}
	bool isConstructorFunction() const {
		return _isConstructor;
	}

	virtual unsigned int arity() const {
		return _insorts.size();
	}

	void partial(bool b); //!< Make the function total/partial if b is false/true
	bool removeVocabulary(const Vocabulary*); //!< Removes a vocabulary from the list of vocabularies
	void addVocabulary(const Vocabulary*); //!< Add a vocabulary to the list of vocabularies

	const std::vector<Sort*>& insorts() const; //!< Return the input sorts of the function
	Sort* insort(unsigned int n) const; //!< Returns the n'th input sort of the function
	Sort* outsort() const; //!< Returns the output sort of the function
	bool partial() const; //!< Returns true iff the function is partial
	bool builtin() const;
	bool isNonConstructorBuiltin() const;
	bool overloaded() const;
	bool hasDerivableSorts() const;
	FuncGenerator* overfuncgenerator() const;
	unsigned int binding() const; //!< Returns binding strength
	std::set<Sort*> allsorts() const;

	FuncInter* interpretation(const Structure*) const;

	// Overloaded symbols
	bool contains(const Function* f) const;
	Function* resolve(const std::vector<Sort*>&);
	Function* disambiguate(const std::vector<Sort*>&, const Vocabulary*);
	std::set<Function*> nonbuiltins(); //!< Returns the set of predicates that are not builtin
									   //!< and that are overloaded by 'this'.

									   // Output
	std::ostream& put(std::ostream&) const;

	friend class Vocabulary;
};

std::ostream& operator<<(std::ostream&, const Function&);

namespace FuncUtils {
/** Return an new overloaded function containing the two given functions **/
Function* overload(Function* p1, Function* p2);

/** Return a new overloaded function containing the given functions **/
Function* overload(const std::set<Function*>&);

/** Check whether the output sort of a function is integer **/
bool isIntFunc(const Function*, const Vocabulary*);

/** Check whether the function is a sum over integers **/
bool isIntSum(const Function*, const Vocabulary*);

/** Check whether the function is a product over integers **/
bool isIntProduct(const Function*, const Vocabulary*);
}

/**************
 * Vocabulary
 *************/

class Namespace;

enum class STDSORT {
	NATSORT,
	INTSORT,
	FLOATSORT,
	CHARSORT,
	STRINGSORT
};
enum class STDPRED {
	EQ,
	GT,
	LT
};
enum class STDFUNC {
	UNARYMINUS,
	ADDITION,
	SUBSTRACTION,
	PRODUCT,
	DIVISION,
	ABS,
	MODULO,
	EXPONENTIAL,
	MINELEM,
	MAXELEM,
	SUCCESSOR,
	PREDECESSOR,
	FIRST = UNARYMINUS,
	LAST = PREDECESSOR
};
template<class S>
std::string getSymbolName(S s);
template<>
std::string getSymbolName(STDFUNC s);
template<>
std::string getSymbolName(STDSORT s);
template<>
std::string getSymbolName(STDPRED s);

template<typename SymbolType>
bool is(const PFSymbol* symbol, SymbolType type) {
	return symbol->name() == getSymbolName(type);
}

Sort* get(STDSORT type);
Function* get(STDFUNC type); // NOTE might not have an interpretation yet, as it might be overloaded
Function* getStdFunc(STDFUNC type, const std::vector<Sort*>& sorts, Vocabulary* voc);
Function* get(STDFUNC type, const std::vector<Sort*>& sorts, Vocabulary* voc);
Predicate* get(STDPRED type); // NOTE might not have an interpretation yet, as it might be overloaded
Predicate* get(STDPRED type, Sort* sort);

class Structure;

class Vocabulary {
private:
	std::string _name; //!< Name of the vocabulary. Default name is the empty string.
	ParseInfo _pi; //!< Place where the vocabulary was parsed
	Namespace* _namespace; //!< The namespace the vocabulary belongs to.

	std::map<std::string, Sort*> _name2sort; //!< Map a name to the sort having that name in the vocabulary
	std::map<std::string, Predicate*> _name2pred; //!< Map a name to the (overloaded) predicate having
												  //!< that name in the vocabulary. Name should end on /arity.
	std::map<std::string, Function*> _name2func; //!< Map a name to the (overloaded) function having
												 //!< that name in the vocabulary. Name should end on /arity.

	static Vocabulary* _std; //!< The standard vocabulary

	std::set<Structure*> structures;



public:
	Vocabulary(const std::string& name);
	Vocabulary(const std::string& name, const ParseInfo& pi);

	void addStructure(Structure* s) {
		structures.insert(s);
	}
	void removeStructure(Structure* s) {
		structures.erase(s);
	}

	~Vocabulary();

	void setNamespace(Namespace* n) {
		_namespace = n;
	}

	void add(Sort*); //!< Add the given sort (and its ancestors) to the vocabulary
	void add(PFSymbol*); //!< Add the given predicate (and its sorts) to the vocabulary
	void add(Predicate*); //!< Add the given predicate (and its sorts) to the vocabulary
	void add(Function*); //!< Add the given function (and its sorts) to the vocabulary
	void add(Vocabulary*); //!< Add all symbols of a given vocabulary to the vocabulary

	static Vocabulary* std(); //!< Returns the standard vocabulary
	const std::string& name() const; //!< Returns the name of the vocabulary
	const ParseInfo& pi() const; //!< Returns the parse info of the vocabularyPred

	bool hasSortWithName(const std::string& name) const;
	bool hasPredWithName(const std::string& name) const;
	bool hasFuncWithName(const std::string& name) const;

	bool contains(const Sort* s) const; //!< True iff the vocabulary contains the sort
	bool contains(const Predicate* p) const; //!< True iff the vocabulary contains the predicate
	bool contains(const Function* f) const; //!< True iff the vocabulary contains the function
	bool contains(const PFSymbol* s) const; //!< True iff the vocabulary contains the symbol

	const std::map<std::string, Predicate*>& getPreds() const {
		return _name2pred;
	}
	const std::map<std::string, Function*>& getFuncs() const {
		return _name2func;
	}
	const std::map<std::string, Sort*>& getSorts() const{
		return _name2sort;
	}

	std::vector<PFSymbol*> getNonBuiltinNonOverloadedNonTypeSymbols() const;

	std::vector<PFSymbol*> getNonBuiltinNonOverloadedSymbols() const {
		std::vector<PFSymbol*> symbols;
		for (auto pn : getPreds()) {
			for (auto p : pn.second->nonbuiltins()) {
				symbols.push_back(p);
			}
		}
		for (auto pn : getFuncs()) {
			for (auto p : pn.second->nonbuiltins()) {
				symbols.push_back(p);
			}
		}
		return symbols;
	}

	std::vector<PFSymbol*> getNonOverloadedSymbols() const{
		std::vector<PFSymbol*> symbols;
		for(auto pn: getPreds()){
			for(auto p:pn.second->nonbuiltins()){
				symbols.push_back(p);
			}
			if(pn.second->builtin() && not pn.second->overloaded()){
				symbols.push_back(pn.second);
			}
		}
		for(auto pn: getFuncs()){
			for(auto p:pn.second->nonbuiltins()){
				symbols.push_back(p);
			}
			if(pn.second->builtin() && not pn.second->overloaded()){
				symbols.push_back(pn.second);
			}
		}
		return symbols;
	}

	std::map<std::string, Sort*>::iterator firstSort() {
		return _name2sort.begin();
	}
	std::map<std::string, Predicate*>::iterator firstPred() {
		return _name2pred.begin();
	}
	std::map<std::string, Function*>::iterator firstFunc() {
		return _name2func.begin();
	}
	std::map<std::string, Sort*>::iterator lastSort() {
		return _name2sort.end();
	}
	std::map<std::string, Predicate*>::iterator lastPred() {
		return _name2pred.end();
	}
	std::map<std::string, Function*>::iterator lastFunc() {
		return _name2func.end();
	}

	std::map<std::string, Sort*>::const_iterator firstSort() const {
		return _name2sort.cbegin();
	}
	std::map<std::string, Predicate*>::const_iterator firstPred() const {
		return _name2pred.cbegin();
	}
	std::map<std::string, Function*>::const_iterator firstFunc() const {
		return _name2func.cbegin();
	}
	std::map<std::string, Sort*>::const_iterator lastSort() const {
		return _name2sort.cend();
	}
	std::map<std::string, Predicate*>::const_iterator lastPred() const {
		return _name2pred.cend();
	}
	std::map<std::string, Function*>::const_iterator lastFunc() const {
		return _name2func.cend();
	}

	Sort* sort(const std::string&) const;
	//!< return the sorts with the given name
	Predicate* pred(const std::string&) const;
	//!< return the predicate with the given name (ending on /arity)
	Function* func(const std::string&) const;
	//!< return the function with the given name (ending on /arity)

	std::set<Predicate*> pred_no_arity(const std::string&) const;
	//!< return all predicates with the given name (not including the arity)
	std::set<Function*> func_no_arity(const std::string&) const;
	//!< return all functions with the given name (not including the arity)

	// Output
	std::ostream& putName(std::ostream&) const;
	std::ostream& put(std::ostream&) const;

	enum class Symbol { PREDICATE, FUNCTION};

	friend class Namespace;
};

std::ostream& operator<<(std::ostream&, const Vocabulary&);

namespace VocabularyUtils {
bool isComparisonPredicate(const PFSymbol*); //!< returns true iff the given symbol is =/2, </2, or >/2
bool isPredicate(const PFSymbol*, STDPRED predtype); //!< returns true iff the given symbol is an instance of the given predicate type
bool isFunction(const PFSymbol*, STDFUNC functype); //!< returns true iff the given symbol is an instance of the given function type
bool isTypePredicate(const PFSymbol*);
CompType getComparisonType(const PFSymbol* symbol);
bool isIntPredicate(const PFSymbol*, const Vocabulary*);
bool isIntComparisonPredicate(const PFSymbol*, const Vocabulary*);
bool isNumeric(Sort*); //!< returns true iff the given sort is a subsort of float
bool isConstructorFunction(const PFSymbol*);

bool isSubVocabulary(Vocabulary* child, Vocabulary* parent);
bool containsSymbol(std::string name,int arity,Vocabulary::Symbol sym, const Vocabulary* voc);
PFSymbol* getSymbol(const Vocabulary* voc, Vocabulary::Symbol sym,std::string name);
}
