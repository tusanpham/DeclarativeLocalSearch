//
// Created by ck on 16.11.17.
//


#include <inferences/querying/Query.hpp>
#include "Localsearch.hpp"
#include "TabuSearch.hpp"
#include "MoveModelling.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "structure/MainStructureComponents.hpp"
#include "structure/StructureComponents.hpp"

#include <IncludeComponents.hpp>
#include "structure/Structure.hpp"
#include "theory/theory.hpp"
#include "theory/Query.hpp"
#include "vocabulary/vocabulary.hpp"
#include "NeighborhoodMove.hpp"
#include <time.h>


TabuSearch::TabuSearch(AbstractTheory* T, Query*  queryGetObj, Term *termObj,Structure* str, Structure *iniSol):
        LocalSearch(T, queryGetObj, termObj,str, iniSol){
    _tabuTenure = 15;           //TODO depends on instance size
}
TabuSearch::TabuSearch(LocalSearch* ls, AbstractMoveModelling* move):LocalSearch(ls, move){}

void TabuSearch::doSearch(){
    if(_verbosity >= 1)
        std::clog  << "hello, this is TABU SEARCH with move " <<*(this->_listmovemodelling[0]->_name)<<"\n";
    LocalSearch::doSearch();

    std::shared_ptr<Structure> currentSol = _initialSol;
    int currentobj = _iniObjVal;
    while(!timeout()){
        NeighborhoodMove* moves = getValidMoves( currentSol.get());        //std::vector<NeighborhoodMove*>
        if (_verbosity >= 1) {
            std::clog << "\t nb of valid moves = " << moves->_size << "\n";
        }
        if(moves->_size == 0)
            break;
        auto bestSolWithMove = this->getBestMove(currentSol.get(), moves);

        auto newsol = getNeighborSolution(bestSolWithMove.get());
        if(newsol == NULL)
            break;
        currentSol = newsol;
        int obj = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, currentSol.get());            //TODO overhead
        if (_verbosity >= 1) {
            std::clog << "\t move to new solution, obj = " << obj << "\n";
        }
        if(obj < _bestObjVal){
            _bestObjVal = obj;
            _bestSol = currentSol;
        }
    }
    if(_bestSol == NULL)
    {
        Structure* copyinitial = _initialSol.get()->clone();
        _bestSol = std::shared_ptr<Structure>(copyinitial);
    }
}
void TabuSearch::removeFromTabuList(int index){
    Assert(index > -1 && index < this->_tabuList.size());
    this->_tabuList.erase(_tabuList.begin() + index);
//    if(_verbosity>=1)
//        std::clog<<"remove from tabu list "<<index<<" size after "<<_tabuList.size()<<std::endl;
}
std::shared_ptr<Structure> TabuSearch::getBestMove(const Structure* currentSol,NeighborhoodMove* listmoves){
    //get best move that is not in tabulist
    ElementTuple bestMove;
    int bestImprovement = 100000;
    std::shared_ptr<Structure> bestSolutionWithMove= nullptr;

    int currentObj = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, currentSol);       //TODO overhead
    bool flag_toogoodmove = false;
    TableIterator move = listmoves->begin();

    while(!move.isAtEnd()){
        auto solutionWithMove = decodeMove(currentSol, *move);
        auto deltaObj = LocalSearchUtility::getIntegerFromQuery(((MoveModelling*)_listmovemodelling[0])->_queryDeltaObj,solutionWithMove.get());

        int indexInTabulist = this->getMoveFromTabuList(*move);
        if(indexInTabulist > -1){           //if move is in tabu list but too good -> accept any way
            if(currentObj + deltaObj < this->_bestObjVal){
                if (_verbosity >= 1) {
                    std::clog << "move is tabu - but good, takeee it anw " << deltaObj << "\n";
                }
                bestImprovement = deltaObj;
                bestMove = *move;   // listmoves[j];
                flag_toogoodmove = true;
            }
        }else {                                 //move is not in tabu list
            if (deltaObj < bestImprovement) {
                bestImprovement = deltaObj;
                bestMove = *move;       //listmoves[j];
                bestSolutionWithMove = solutionWithMove;
            }
        }
        ++move;
    }
    //    bestSolutionWithMove->clean();

    //if everything is in tabu list already, what do you do?
    //actually it shouldn't happen, the tabu tenure should be proportional with instance size
    if(bestImprovement == 100000){
        bestMove = _tabuList[0];
        bestSolutionWithMove = std::shared_ptr<Structure>(decodeMove(currentSol, bestMove));
        removeFromTabuList(0);
    }
    else{
        if(flag_toogoodmove == false) {
            updateTabuList(bestMove);
//            if (_verbosity >= 1) {
//                std::clog << "update tabu list with best move size: "<<this->_tabuList.size()<<" " <<bestMove<<std::endl;
//            }
        }
    }
    return bestSolutionWithMove;
}


void TabuSearch::updateTabuList(const std::vector<const DomainElement*>& move)
{
    if(_tabuList.size() >= _tabuTenure){
        _tabuList.erase(_tabuList.begin());
        if(_verbosity >= 1)
            std::clog<<"remove from tabu list, size now " << _tabuList.size()<<std::endl;
    }
    _tabuList.push_back(move);

//    if #tabulist >= tabu_tenure then
//    //local oldest = itercount - tabu_tenure
//    table.remove(tabulist)
//    end
//    table.insert(tabulist, 1, move)
}

int TabuSearch::getMoveFromTabuList(const std::vector<const DomainElement*>& move){
    for(int i = 0;i<this->_tabuList.size();i++) {
//            std::clog<<"comparing move "<<move<<" with i = "<<i<< " move "<< _tabuList[i]<<
//                                                               " and size = "<<_tabuList.size()<<std::endl;
        if(NeighborhoodMove::movesAreEqual(_tabuList[i], move))
            return i;
    }
    return -1;
}


TabuSearch::~TabuSearch() {

}