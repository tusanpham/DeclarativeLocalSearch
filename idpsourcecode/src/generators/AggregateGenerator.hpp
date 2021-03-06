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

#ifndef AGGGENERATOR_HPP_
#define AGGGENERATOR_HPP_


#include "commontypes.hpp"
#include "InstGenerator.hpp"
class DomElemContainer;
class InstGenerator;
class  DomainElement;


/**
 * A generator for aggregate expressions.  Generator for formulas of the form
 * v = card {...}
 * v will always be output. All other free variables are assumed to be input.
 * A comparisongenerator needs to be used after this one in order to get general aggforms
 * Also, no table for v is given to this generator.  It is the responsibility of the (later-called)
 * comparisongenerator to check for consistency of the sorts.
 */
class AggGenerator: public InstGenerator {
private:
	const DomElemContainer* _left;
	AggFunction _func;
	std::vector<InstGenerator*> _formulagenerators;
	std::vector<InstGenerator*> _termgenerators;
	std::vector<const DomElemContainer*> _terms; //Generated by the _termgenerators

	double _result;
	bool _reset;

	//Executes the next on the ith formulagenerator,...
	//Returns false if no value is possible (if this one determines the generator to be at end).
	bool next(unsigned int i);
	double getEmptySetValue();

	bool setValue(const DomainElement* d);

	bool doOperation(double d);

public:
	AggGenerator(const DomElemContainer* left, AggFunction func, std::vector<InstGenerator*> formulagenerators, std::vector<InstGenerator*> termgenerators,
			std::vector<const DomElemContainer*> terms);

	AggGenerator* clone() const;

	~AggGenerator();

	void reset() {
		_reset = true;
	}

	void next();

	virtual void put(std::ostream& stream) const;
};

#endif /* AGGGENERATOR_HPP_ */
