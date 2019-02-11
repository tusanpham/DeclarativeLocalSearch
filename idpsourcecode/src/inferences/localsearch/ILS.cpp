//
// Created by san on 07.08.18.
//

#include "Localsearch.hpp"
#include "ILS.h"
#include "LargeNeighbourhoodSearch.h"                 //remember, include it before the others causes weird errors :(


#include <inferences/querying/Query.hpp>
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "structure/MainStructureComponents.hpp"
#include "structure/StructureComponents.hpp"
#include "inferences/modelexpansion/LuaTraceMonitor.hpp"
#include "lua/luaconnection.hpp"

#include <IncludeComponents.hpp>
#include "structure/Structure.hpp"
#include "theory/theory.hpp"
#include "theory/Query.hpp"
#include "theory/term.hpp"
#include "vocabulary/vocabulary.hpp"
#include "NeighborhoodMove.hpp"
#include "TabuSearch.hpp"
#include <time.h>
#include "utils/ListUtils.hpp"
//#include "LargeNeighbourhoodSearch.h"
#include "FirstImprovementSearch.hpp"
#include "BestImprovementSearch.hpp"

ILS::ILS(AbstractTheory* T, Query*  queryGetObj, Term* termObj, Structure* str, Structure *iniSol):
LocalSearch(T, queryGetObj, termObj, str, iniSol)
{
    this->_mCurrentDisturbSize = this->_mMinDisturbSize;
}

void ILS::doSearch(){
    if(_verbosity >= 1)
        std::clog  << "hello, this is ILS" <<"\n";
    LocalSearch::doSearch();

    this->_currentSol = _initialSol;
    this->_currentObjVal = _iniObjVal;
    int iterationsWithoutImprovement = 0;
    int iter = 0;
    int moveindex = -1;

    while(!timeout()) {
        iter++;
        moveindex = getNextMove(moveindex); //getRandomMove(0);

        if(_verbosity>= 1)
            std::clog<<"perfoming ILS with move "<<moveindex<<std::endl;

        int deltaobj = doSingleNeighbourhoodSearch(this->_currentSol, this->_listLSType[moveindex], this->_listmovemodelling[moveindex]);

        if(deltaobj < 0) {                //improve
            iterationsWithoutImprovement = 0;
            if(this->_currentObjVal < this->_bestObjVal){           //then update best solution
                this->_bestObjVal = this->_currentObjVal;
                this->_bestSol = this->_currentSol;
            }
        }
        else {
            iterationsWithoutImprovement++;
            if(iterationsWithoutImprovement > this->_listmovemodelling.size()) {
                if(_verbosity>= 1)
                    std::clog<<"no improvement found by all neighbourhood, stop ILS"<<std::endl;
                break;
            }
        }
    }
}

int ILS::getNextMove(int currentmoveindex){        //without random, I mean
    int i = currentmoveindex + 1;
    if(i == this->_listmovemodelling.size())
        i = 0;
    return i;
}

int ILS::doSingleNeighbourhoodSearch(std::shared_ptr<Structure> currentSol,  LocalSearchType lstype, AbstractMoveModelling* move){
    LocalSearch* ls;
    switch(lstype){
        case LocalSearchType::BESTIMPROVE:
            ls = new BestImprovementSearch(this, move);
            break;
        case LocalSearchType::TABU:
            ls = new TabuSearch(this, move);
            break;
        case LocalSearchType::FIRSTIMPROVE:
        default:
            ls = new FirstImprovementSearch(this, move);
            break;
    }

    ls->setInitialSolution(currentSol);

    int currentobj = LocalSearchUtility::getIntegerFromQuery(this->_queryGetObj, currentSol.get());
    if(_verbosity>= 1)
        std::clog<<"start ILS with obj = "<<currentobj<<std::endl;

    ls->doSearch();
    this->_currentSol = ls->_bestSol;
    this->_currentObjVal = ls->_bestObjVal;
    return (ls->_bestObjVal - ls->_iniObjVal);
}

int ILS::disturbSolution(Structure* iniSol) {
    LargeNeighbourhoodSearch* lns = new LargeNeighbourhoodSearch(this->_theo, this->_queryGetObj, this->_termObj, this->_strucInput, iniSol);
    std::shared_ptr<Structure> results = lns->disturbSolutionRandomly(iniSol, this->_mCurrentDisturbSize);

    //TODO: update the results
    if(this->_mCurrentDisturbSize < this->_mMaxDisturbSize)
        this->_mCurrentDisturbSize++;
    return 0;
}