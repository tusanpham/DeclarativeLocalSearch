//
// Created by ck on 21.11.17.
//

#pragma once

#include <memory>

class LocalSearch;
class Structure;
class AbstractTheory;
class Vocabulary;
class Theory;
class Query;
class Sort;
class NeighborhoodMove;
//class MoveModelling;
class AbstractMoveModelling;

class BestImprovementSearch: public LocalSearch{
public:
    BestImprovementSearch(AbstractTheory* T, Query*  queryGetObj,Term *termObj,  Structure* str, Structure *iniSol);
    BestImprovementSearch(LocalSearch* ls, AbstractMoveModelling* move);
    void doSearch();
};


