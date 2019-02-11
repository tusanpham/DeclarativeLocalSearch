//
// Created by san on 07.08.18.
//

#ifndef IDP_VND_H
#define IDP_VND_H

//#include "LargeNeighbourhoodSearch.h"

class LocalSearch;
class Structure;
class AbstractTheory;
class Vocabulary;
class Theory;
class Query;
class Sort;
class NeighborhoodMove;
class MoveModelling;
class PFSymbol;
class PredInter;
class Term;
class PredTable;
class LargeNeighbourhoodSearch;

class ILS: public LocalSearch{
protected:
    std::shared_ptr<Structure> _currentSol;
    int _currentObjVal;
    int _mMinDisturbSize = 10;
    int _mMaxDisturbSize = 50;
    int _mCurrentDisturbSize = -1;

public:
    ILS(AbstractTheory* T, Query*  queryGetObj,Term* termObj, Structure* str, Structure *iniSol);
    void doSearch();

protected:
    int getNextMove(int currentmoveindex);
    //std::shared_ptr<Structure>
    int doSingleNeighbourhoodSearch(std::shared_ptr<Structure> currentSol, LocalSearchType lstype, AbstractMoveModelling* move);
    int disturbSolution(Structure* iniSol);
};


//class ILS {
//
//};


#endif //IDP_VND_H
