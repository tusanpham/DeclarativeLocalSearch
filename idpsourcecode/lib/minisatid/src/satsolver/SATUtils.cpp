/*
 * Copyright 2007-2011 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat and Maarten Mariën, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */
#include "satsolver/SATUtils.hpp"

using namespace MinisatID;

rClause MinisatID::nullPtrClause = Minisat::CRef_Undef;

pClause MinisatID::getClauseRef(rClause rc){
	return rc;
}

double MinisatID::getDefaultDecay(){
	return 0.95;
}
double MinisatID::getDefaultRandfreq(){
	return 0;
}
Polarity MinisatID::getDefaultPolarity(){
	return Polarity::STORED;
}
InitActivity MinisatID::getDefaultInitActivity(){
	return InitActivity::DEFAULT;
}
