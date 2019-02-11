//
// Created by ck on 21.11.17.
//

#pragma once

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

class FirstImprovementSearch:public LocalSearch {
private:

public:
    FirstImprovementSearch(AbstractTheory* T, Query*  queryGetObj,Term *termObj, Structure* str, Structure *iniSol);
    FirstImprovementSearch(LocalSearch* ls, AbstractMoveModelling* move);
    void doSearch();
};

