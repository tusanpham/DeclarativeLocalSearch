//
// Created by ck on 21.11.17.
//

#include "Localsearch.hpp"
#include "FirstImprovementSearch.hpp"
#include "MoveModelling.hpp"

#include <inferences/querying/Query.hpp>
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "structure/MainStructureComponents.hpp"
#include "structure/StructureComponents.hpp"

#include <IncludeComponents.hpp>
#include "structure/Structure.hpp"
#include "theory/theory.hpp"
#include "theory/Query.hpp"
#include "vocabulary/vocabulary.hpp"
#include "NeighborhoodMove.hpp"
#include "TabuSearch.hpp"
#include <time.h>
#include "utils/ListUtils.hpp"

FirstImprovementSearch::FirstImprovementSearch(AbstractTheory* T, Query*  queryGetObj, Term *termObj, Structure* str, Structure *iniSol):
        LocalSearch(T, queryGetObj,termObj, str, iniSol){}
FirstImprovementSearch::FirstImprovementSearch(LocalSearch* ls, AbstractMoveModelling* move):LocalSearch(ls, move){}

void FirstImprovementSearch::doSearch() {
    if(_verbosity >= 1)
        std::clog  << "hello, this is FIRST IMPROVEMENT local search with move " <<*(this->_listmovemodelling[0]->_name)<<"\n";

    LocalSearch::doSearch();

    std::shared_ptr<Structure> currentSol = _initialSol;
    int currentobj = _iniObjVal;

    while(!timeout()){
        bool improved = false;
        NeighborhoodMove* moves = getValidMoves(currentSol.get());

        if (_verbosity >= 1) {
            std::clog << "\t number of valid moves = " << moves->_size << "\n";
        }
        TableIterator itermove = moves->begin();

        while(!itermove.isAtEnd()){
            auto move =*itermove;                   //*iterator is ElementTuple&, or actually, std::vector<const DomainElement*>

            auto solutionWithMove = decodeMove(currentSol.get(), move);

            if(((MoveModelling*)_listmovemodelling[0])->_queryDeltaObj != NULL){                    //check if delta objective is available
                auto deltaObj = getDeltaObj(solutionWithMove.get());

                if (_verbosity >= 1) {
                    std::clog << "process move " << move << " delta obj: " << deltaObj << " current obj; " << currentobj << std::endl;
                    if (_verbosity >= 3) {
                        std::clog<<"solution with move: ";
                        solutionWithMove.get()->put(std::clog);
                    }
                }

                auto newsol = getNeighborSolution(solutionWithMove.get());

                if(newsol == NULL)
                    break;

                int newobj = getObjValue(newsol.get());

                if (_verbosity >= 2)
                    std::clog<<"process move "<<move<<" delta obj: "<< deltaObj<< " current obj; " << currentobj << " new obj: " << newobj<<std::endl;
                if(deltaObj < 0) {
                    if(newobj < currentobj) {
                        improved = true;
                        if (_verbosity >= 1) {
                            std::clog << "\t move to new solution, obj = " << newobj << "\n";
                        }
                        currentSol = newsol;
                        currentobj = newobj;
                        if (newobj < _bestObjVal) {
                            _bestObjVal = newobj;
                            _bestSol = currentSol;
                        }
                        break;
                    }
                }
            }
            else{           //if not, then do model expansion to get new solution and new objval
                time_t start = clock();
                auto nextsol = getNextSolution(solutionWithMove.get());
                if(nextsol == NULL)  break;
                auto newobj = LocalSearchUtility::getIntegerFromQuery(((MoveModelling*)_listmovemodelling[0])->_queryGetNextCost, nextsol.get());
                auto deltaObj = newobj - currentobj;

                _timeinfor._timeGetDelObj += clock() - start;
                _timeinfor._countGetDelObj++;

                std::clog<<"newobj = " <<newobj << " currentobj ="<<currentobj<<std::endl;
//                std::clog<<"\n finish getting next cost new = " <<newobj <<" old = "<<currentobj <<" delta = "<<deltaObj<<std::endl;
                if(deltaObj < 0){
                    improved = true;
                    auto newsol = getNeighborSolutionWithoutMX(nextsol.get());
                    auto realobj = LocalSearchUtility::getIntegerFromQuery(this->_queryGetObj, newsol.get());
                    std::clog<<"newobj = " <<newobj << " realobj ="<<realobj<<std::endl;
                    if(newsol == NULL) break;
                    if (_verbosity >= 1) {
                        std::clog << "\t move to new solution, obj = " << newobj << "\n";
                    }
                    currentSol = newsol;
                    currentobj = newobj;
                    if (newobj < _bestObjVal) {
                        _bestObjVal = newobj;
                        _bestSol = currentSol;
                    }
                    break;
                }
            }
            ++itermove;

            if(timeout())
                break;
        }
        delete moves;
        if(!improved) {
            std::clog <<"no improvement found, stop!" <<std::endl;
            break;
        }

    }
    if(_bestSol == NULL)
    {
        Structure* copyinitial = _initialSol.get()->clone();
        _bestSol = std::shared_ptr<Structure>(copyinitial);
    }
}

//before change
//void FirstImprovementSearch::doSearch() {
//    if(_verbosity >= 1)
//        std::clog  << "hello, this is first improvement local search" <<"\n";
//    auto inisol = getInitialSolution();              //return std::vector<Structure*>
//
////    std::clog<<"finish getting inital solution "<<std::endl;
////    inisol.get()->put(std::clog);
//
//    _bestObjVal = _iniObjVal;
//    _bestSol = inisol;                              //_initialSol;
//
//    std::shared_ptr<Structure> currentSol = _initialSol;
//    int currentobj = _iniObjVal;
//    while(!timeout()){
//        bool improved = false;
//
//        NeighborhoodMove moves = getValidMoves(currentSol.get());
//        TableIterator itermove = moves.begin();
//        while(!itermove.isAtEnd()){
//            //*iterator is ElementTuple&, or actually, std::vector<const DomainElement*>
//            auto move =*itermove;
//            auto solutionWithMove = decodeMove(currentSol.get(), move);
////            solutionWithMove->clean();
//
//            auto deltaObj = getDeltaObj(solutionWithMove.get());
////            std::clog<<"process move "<<move<<" with deltaobj "<< deltaObj<<std::endl;
//            if(deltaObj < 0) {              //if improves
//                improved = true;
//
//                auto newsol = getNeighborSolution(solutionWithMove.get());
////                std::clog<<"done getting neighbor solution"<<std::endl;
////                newsol.get()->put(std::clog);
//
//                if(newsol == NULL)
//                    break;
//                int newobj = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, newsol.get());
//                if(newobj < currentobj) {
//                    if (_verbosity >= 1) {
//                        std::clog << "\t move to new solution, obj = " << newobj << "\n";
//                    }
//                    currentSol = newsol;
//                    currentobj = newobj;
//                    if (newobj < _bestObjVal) {
//                        _bestObjVal = newobj;
//                        //TODO check if _bestSol is not iniSol then delete it
//                        _bestSol = currentSol;
//                    }
//                    break;
//                }
//            }
//            ++itermove;
//        }
//        if(!improved)
//            break;
//    }
//}