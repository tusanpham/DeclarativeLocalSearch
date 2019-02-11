//
// Created by ck on 10.11.17.
//

#pragma once

#include <debug/vector>
#include <ostream>
class PredTable;
class DomainElement;
class TableIterator;

class NeighborhoodMove{
    //contains a set of neighborhood move
//    typedef std::vector<const DomainElement*> ElementTuple;
public:
    PredTable* _tablevalue;
    long _size;
    //when iterate through a preTable, *iter return a ElementTuple or std::vector<const DomainElement*>,
    //which corresponds to a singe move

//    std::vector<const DomainElement*>  _value;

    NeighborhoodMove(PredTable* table);
    TableIterator begin();          //iterator
    static bool movesAreEqual(const std::vector<const DomainElement*>& move1,const std::vector<const DomainElement*>& move2);
//    bool operator==(const NeighborhoodMove& other) const;
    ~NeighborhoodMove();

};



