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

#ifndef CONTAINSFUNCTERMSNOSET_HPP_
#define CONTAINSFUNCTERMSNOSET_HPP_

#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

class CheckContainsFuncTermsOutsideOfSets: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	bool _result;

public:
	bool execute(const Formula* f) {
		_result = false;
		f->accept(this);
		return _result;
	}

protected:
	void visit(const FuncTerm*) {
		_result = true;
	}

	virtual void visit(const EnumSetExpr*){
		return;
	}
	virtual void visit(const QuantSetExpr*){
		return;
	}
};

#endif /* CONTAINSFUNCTERMS_HPP_ */
