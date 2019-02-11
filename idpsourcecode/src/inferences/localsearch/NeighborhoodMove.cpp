//
// Created by ck on 10.11.17.
//

#include "NeighborhoodMove.hpp"
#include "structure/DomainElement.hpp"
#include "structure/MainStructureComponents.hpp"
#include <Assert.hpp>


NeighborhoodMove::NeighborhoodMove(PredTable *table){
    _tablevalue = table;
    _size = table->size()._size;
}

TableIterator NeighborhoodMove::begin(){
    return _tablevalue->begin();
}

//bool NeighborhoodMove::operator==(const NeighborhoodMove& other) const{
//    Assert(_size == other._size);
//    for(int i = 0;i<_size;i++){
//        if(_value[i] != other._value[i])
//            return false;
//    }
//    return true;
//}

bool NeighborhoodMove::movesAreEqual(const std::vector<const DomainElement*>& move1,const std::vector<const DomainElement*>& move2){
//    std::clog<<"moves compare "<<move1<<" and "<<move2<<std::endl;
    //TODO create operator == in domainelement
    for(int i = 0;i<move1.size();i++){
        switch(move1[i]->type()){
            case DomainElementType::DET_INT:
                if(move1[i]->value()._int != move2[i]->value()._int)
                    return false;
                break;
            case DomainElementType::DET_DOUBLE:
                if(move1[i]->value()._double != move2[i]->value()._double)
                    return false;
                break;
            case DomainElementType::DET_STRING:
                if(move1[i]->value()._string->compare(*(move2[i]->value()._string)) != 0)
                    return false;
                break;
            default:                //DET_COMPOUND
                break;
                //TODO throw exception
        }
    }
    return true;
}
NeighborhoodMove::~NeighborhoodMove(){
    if(_tablevalue != NULL)
        delete _tablevalue;
}