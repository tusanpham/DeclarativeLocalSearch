//
// Created by ck on 21.11.17.
//

#include "Localsearch.hpp"
#include "BestImprovementSearch.hpp"
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
#include <time.h>
#include "utils/ListUtils.hpp"


//BestImprovementSearch::BestImprovementSearch(AbstractTheory* T, Query*  getObj, AbstractTheory* Tnext,
//                                               Query*  queryValidMoves, Query*  queryDeltaObj, Structure* str,Vocabulary* Vmove):
//        LocalSearch(T, getObj, Tnext, queryValidMoves, queryDeltaObj,str, Vmove){
//}

BestImprovementSearch::BestImprovementSearch(AbstractTheory* T, Query*  queryGetObj,Term *termObj, Structure* str , Structure *iniSol):
        LocalSearch(T, queryGetObj, termObj, str, iniSol){}
BestImprovementSearch::BestImprovementSearch(LocalSearch* ls, AbstractMoveModelling* move):LocalSearch(ls, move){}


void BestImprovementSearch::doSearch() {
    if(_verbosity >= 1)
        std::clog  << "hello, this is BEST IMPROVEMENT local search with move " <<*(this->_listmovemodelling[0]->_name)<<"\n";

    clock_t DEBUGstart = clock();
    long timegetmove = 0;
    long timedecodemove = 0;
    long timegetdelta = 0;
    long timegetneighbor = 0;

    LocalSearch::doSearch();

    std::shared_ptr<Structure> currentSol = _initialSol;
    int currentobj = _iniObjVal;

    while(!timeout()) {
        int bestimprove = 100000;
        std::shared_ptr<Structure> bestSolWithMove = nullptr;
        std::vector<const DomainElement*> bestMove;

        clock_t beforegetmoves = clock();
        NeighborhoodMove* moves = getValidMoves(currentSol.get());
        clock_t aftergetmoves = clock();
        timegetmove += aftergetmoves- beforegetmoves;

        if (_verbosity >= 1) {
            std::clog<<"no valid moves: "<<moves->_size<<std::endl;
        }

        TableIterator move = moves->begin();
        while(!move.isAtEnd()){
            clock_t beforedecodemove = clock();

//            for(int t = 0;t<bestMove.size();t++) {
//                (*move)[t]->put(std::clog);
//                std::clog<<" ";
//            }
//            std::clog<<std::endl;

            auto solutionWithMove = decodeMove(currentSol.get(), *move);
            clock_t afterdecodemove = clock();
            timedecodemove += afterdecodemove - beforedecodemove;

//            solutionWithMove->clean();
            clock_t beforegetdelta = clock();
            auto deltaObj = getDeltaObj(solutionWithMove.get());// LocalSearchUtility::getIntegerFromQuery(queryDeltaObj, solutionWithMove);
            clock_t aftergetdelta = clock();
            timegetdelta += aftergetdelta - beforegetdelta;
            if (deltaObj < bestimprove) {              //if improves
                bestimprove = deltaObj;
                bestSolWithMove = solutionWithMove;
                bestMove = *move;
            }
            ++move;
        }
        delete moves;

        if (bestimprove < 0) {
            clock_t beforegetneighbor = clock();
            auto newsol = getNeighborSolution(bestSolWithMove.get());
            clock_t aftergetneighbor = clock();
            timegetneighbor += aftergetneighbor - beforegetneighbor;
            if(newsol == NULL)
                break;
            currentSol = newsol;
            int obj = currentobj + bestimprove;
            currentobj = obj;
            if (_verbosity >= 1) {
                std::clog << "\t move to new solution, delobj = " <<  bestimprove<< " obj " << obj<<"   ";
//                for(int t = 0;t<bestMove.size();t++) {
//                    bestMove[t]->put(std::clog);
//                    std::clog<<" ";
//                }
                std::clog<<std::endl;
            }
            if (obj < _bestObjVal) {
                _bestObjVal = obj;
                //TODO check if _bestSol is not iniSol then delete it
                _bestSol = currentSol;
            }
        } else {
            break;
        }
    }
    if(_bestSol == NULL)
    {
        Structure* copyinitial = _initialSol.get()->clone();
        _bestSol = std::shared_ptr<Structure>(copyinitial);
    }
    int bestobj = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, _bestSol.get());
    _bestObjVal = bestobj;
}
