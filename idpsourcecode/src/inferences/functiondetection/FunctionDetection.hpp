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
#include <string>
class Vocabulary;
class Theory;
class Predicate;
class Variable;
class Structure;
#include "vocabulary/VarCompare.hpp"

class FunctionDetection {
private:
	Vocabulary* origVoc;
	Theory *theory;

	// Stats
	int inoutputvarcount, totalfunc, partfunc, provercalls;

public:
	static void doDetectAndRewriteIntoFunctions(Theory* theory);

private:
	FunctionDetection(Theory* theory);
	~FunctionDetection();
	void detectAndRewriteIntoFunctions();
	bool tryToTransform(Theory* newTheory, Predicate* pred, const std::vector<Variable*>& predvars, const varset& domainset, bool partial = false);
};
