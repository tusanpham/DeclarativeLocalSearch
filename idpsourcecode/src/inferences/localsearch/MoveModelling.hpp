//
// Created by ck on 24.07.18.
//

#ifndef IDP_MOVEMODELLING_HPP
#define IDP_MOVEMODELLING_HPP

#include <time.h>
#include <vector>
#include <iostream>
#include <memory>

class Structure;
class AbstractTheory;
class Vocabulary;
class Theory;
class Term;
class Query;
class Sort;
class NeighborhoodMove;
class DomainElement;
class PredTable;
class Namespace;
class SortTable;

#include "theory/theory.hpp"

class AbstractMoveModelling{
public:
    std::string* _name;
    AbstractMoveModelling(){
        _name = NULL;
    }

    ~AbstractMoveModelling(){
        if(_name != NULL)
            delete _name;
        _name = NULL;
    }
};
class MoveModelling: public AbstractMoveModelling{
public:
    AbstractTheory* _theoNext;
    Vocabulary* _vocNext;
    Vocabulary* _vocMove;

    Vocabulary* _vocValidMoves;
    AbstractTheory* _theoGetValidMoves;

    Query* _queryValidMoves;
    Query* _queryDeltaObj;
    Query* _queryGetNextCost;

    MoveModelling( std::string* name, AbstractTheory* Tnext,
    Query*  queryValidMoves, Query* queryDeltaObj,Query* queryNextCost, Vocabulary* Vmove):
    _theoNext(Tnext),
    _queryValidMoves(queryValidMoves),
    _queryDeltaObj(queryDeltaObj),
    _queryGetNextCost(queryNextCost),
    _vocMove(Vmove)
    {
        this->_name = new std::string(*name);
        _vocNext = _theoNext->vocabulary();
        _vocValidMoves = NULL;
        _theoGetValidMoves = NULL;

    }
    MoveModelling(std::string* name, AbstractTheory* Tnext, Query*  queryDeltaObj,Query* queryNextCost, Vocabulary* Vmove,
            AbstractTheory* TgetValidMoves):
    _theoNext(Tnext),
    _queryDeltaObj(queryDeltaObj),
    _queryGetNextCost(queryNextCost),
    _vocMove(Vmove),
    _theoGetValidMoves(TgetValidMoves)
    {
//        this->_name = name;
        this->_name = new std::string(*name);
        _vocNext = _theoNext->vocabulary();
        _vocValidMoves = _theoGetValidMoves->vocabulary();
    }
};

class LNSMoveModelling: public AbstractMoveModelling{
public:
    Vocabulary* _vocMove;
    Query* _queryGetMove;
    Query* _queryGetRandVar_Domain;

    LNSMoveModelling(std::string* name, Vocabulary* Vmove, Query* queryGetMove, Query* queryGetRandomVarDomain):
            _vocMove(Vmove),
            _queryGetMove(queryGetMove){
        this->_queryGetRandVar_Domain = queryGetRandomVarDomain;
        //PTSAN what's the difference between those two constructors, can cause error or nottttt?
        this->_name = name;

    }

};


#endif //IDP_MOVEMODELLING_HPP
