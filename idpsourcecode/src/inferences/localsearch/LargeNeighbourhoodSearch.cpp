//
// Created by san on 21.07.18.
//
#include "Localsearch.hpp"
#include "FirstImprovementSearch.hpp"


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
#include "LargeNeighbourhoodSearch.h"

LargeNeighbourhoodSearch::LargeNeighbourhoodSearch(AbstractTheory* T, Query*  queryGetObj,Term* termObj,
                                                   Structure* str, Structure *iniSol):
        LocalSearch(T, queryGetObj,termObj, str, iniSol){
    this->_averagesize = 10000;
    _lConcreteMovemodelling = std::vector<LNSMove*>();
    this->_mTimeLimitMinimizeStep = getOption(IntType::MXTIMEOUT);
}
int LargeNeighbourhoodSearch::getRandomMove(int iterationsWithoutImprovement){
    int result = -1;

    if(this->_lEnabledMoves.size() == 0)
        return -1;
    int moveindex = LocalSearchUtility::getRandom(0, this->_lEnabledMoves.size());
    if(moveindex == this->_lEnabledMoves.size())
        result = 1000;
    else
        result = _lEnabledMoves[moveindex];
//    if(iterationsWithoutImprovement >= this->_mMaxItersNoImprove)           //always use random move if stuck
//        result = 1000;
    return result;
}
int LargeNeighbourhoodSearch::getNextMove(int currentmoveindex){        //without random, I mean
    int i = currentmoveindex + 1;
    if(i == _lConcreteMovemodelling.size())
        i = 0;
    for(;i<_lConcreteMovemodelling.size();i++){
        if(_lConcreteMovemodelling[i]->_enable == true)
            return i;
        if (i == _lConcreteMovemodelling.size() - 1)
            i = 0;
        if (i == currentmoveindex)
            break;
    }
    return -1;
}
void LargeNeighbourhoodSearch::enableMove(int index){
    if(index <0 || index >= _lConcreteMovemodelling.size())
        return;
    LNSMove* move = _lConcreteMovemodelling[index];
    if(move->_enable == true)
        return;
    move->_enable = true;
    _lEnabledMoves.push_back(index);
}
void LargeNeighbourhoodSearch::disableMove(int index){
    if(index <0 || index >= _lConcreteMovemodelling.size())
        return;
    LNSMove* move = _lConcreteMovemodelling[index];
    if(move->_enable == false)
        return;
    move->_enable = false;
    for(int i = 0;i<_lEnabledMoves.size();i++){
        if(_lEnabledMoves[i] == index){
            _lEnabledMoves.erase(_lEnabledMoves.begin() + i);
            return;
        }
    }
}
void LargeNeighbourhoodSearch::doSearch(){
    if(_verbosity >= 1)
        std::clog  << "hello, this is large neighbourhood search" <<"\n";
    LocalSearch::doSearch();
    int increasedTimeLimit = _mTimeLimitMinimizeStep;

    setOption(IntType::MXTIMEOUT, _mTimeLimitMinimizeStep);

    std::shared_ptr<Structure> currentSol = _initialSol;
    signalChangeCurrentSolution(currentSol.get());
    int currentobj = _iniObjVal;
    _bestObjVal = currentobj;
    int strength = this->_mMinDisturbSize;
    int iterationsWithoutImprovement = 0;

    int iter = 0;
    while(!timeout()) {
        iter++;
        if(iterationsWithoutImprovement >= this->_mMaxItersNoImprove) {
            strength++;
            increasedTimeLimit += 10;
        }
        if (strength >= _mMaxDisturbSize)
            break;

        int moveindex = getRandomMove(iterationsWithoutImprovement);
        if(moveindex == -1){
            std::clog<<"no valid moves anymore \n";
            break;
        }

        if(this->_verbosity >= 1)
            std::clog<<iter<<" - disturb solution with move " <<moveindex<<"\t";//<<": "<<this->_listmovemodelling[moveindex]->_name<<std::endl;

        std::shared_ptr<Structure> newsol;
        if(moveindex == 1000) {             //RANDOM move
            setOption(IntType::MXTIMEOUT, increasedTimeLimit);
            newsol = disturbSolutionRandomly(currentSol.get(), strength);
        }
        else {
            setOption(IntType::MXTIMEOUT, _mTimeLimitMinimizeStep);
            newsol = disturbFromMove(currentSol.get(), moveindex);
        }
        if(newsol == NULL)
            continue;
        auto newobj = LocalSearchUtility::getIntegerFromQuery(this->_queryGetObj, newsol.get());
        if(_verbosity >= 1)
            std::clog<<"newobj = " <<newobj <<std::endl;

        if(newobj < currentobj) {           //<=
            currentSol = newsol;
            currentobj = newobj;
            std::clog<<"\t update solution \n";
            signalChangeCurrentSolution(currentSol.get());
        }
        if (newobj < _bestObjVal) {
            _bestObjVal = newobj;
            _bestSol = currentSol;
            iterationsWithoutImprovement = 0;
        }
        else
            iterationsWithoutImprovement++;
    }
    if(_bestSol == NULL)
    {
        Structure* copyinitial = _initialSol.get()->clone();
        _bestSol = std::shared_ptr<Structure>(copyinitial);
    }
}

std::shared_ptr<Structure> LargeNeighbourhoodSearch::disturbSolutionRandomly(const Structure *currentsol, int strength){
    //change decision variables randomly with the given strength
    PredInter* inter_dec_vars = currentsol->inter(this->_dec_vars)->clone();
    changeInterRandomly(inter_dec_vars, strength);

    //now create new solution, decode this new decision variables into dec_fixed
    auto newstruc = std::shared_ptr<Structure>(this->_strucInput->clone());

    PredInter* inter_dec_fixed = newstruc->inter(this->_dec_vars);      //(this->_dec_fixed);
    inter_dec_fixed->ct(inter_dec_vars->ct());

    if(this->_averagesize == 10000)
        this->_averagesize = inter_dec_vars->ct()->size()._size/2;

    LuaTraceMonitor* tracer = NULL;
    if (getOption(BoolType::TRACE)) {
        tracer = LuaConnection::getLuaTraceMonitor();
    }
    auto models = ModelExpansion::doMinimization(this->_theo, newstruc.get(), this->_termObj, NULL, tracer)._models;

    if(models.size() == 0)
        std::clog<<"something wrong here, no result on minimization"<<std::endl;
    LocalSearchUtility::cleanUpAfterModelExpand(models,0);
    std::shared_ptr<Structure> result(models[0]);
    return result;
}

void LargeNeighbourhoodSearch::changeInterRandomly(PredInter *inter, int strength){
    PredTable* cttable = inter->ct();
    auto size = cttable->size()._size;

    if(strength >= size)
        strength = size -1;
    int startindex = LocalSearchUtility::getRandom(0, size-strength-1);
    int endindex = startindex + strength;

    //now, destroy that part
    int i = 0;
    for (auto ctit = cttable->begin(); not ctit.isAtEnd(); ++ctit) {
        if(i >= startindex && i <= endindex){
            auto tuple = *ctit;                         //auto ctvalue = tuple.back();
//            inter->makeFalseExactly(tuple);
            cttable->remove(tuple);
        }
        i++;
    }
    if(this->_verbosity >= 1)
        std::clog<<"size: "<<size << " - strength: " << strength <<" [" << startindex << "," << endindex<<"]\t";
}

LargeNeighbourhoodSearch::~LargeNeighbourhoodSearch(){
    for(int i = 0;i<_lConcreteMovemodelling.size();i++){
        if(_lConcreteMovemodelling[i] != nullptr){
            delete(_lConcreteMovemodelling[i]);
            _lConcreteMovemodelling[i] = nullptr;
        }
    }
}


void LargeNeighbourhoodSearch::processInputData(){
    auto listfunc = this->_voc->getFuncs();        //std::map<std::string, Function*>&
    auto listpred = this->_voc->getPreds();         //std::map<std::string, Predicate*>&

    auto prediter = listpred.begin();
    while(prediter != listpred.end()){
        auto tuple = *prediter;
        if(tuple.first.find("dec_") == 0 ) //&& != std::string::npos)// == "dec_Path/2")
        {
            this->_dec_vars = tuple.second;
        }
        ++prediter;
    }

    auto funcinter = listfunc.begin();              ////std::map<std::string, Function*>&
    while(funcinter != listfunc.end()){      // while(!funcinter._M_is_end()){
        auto tuple = *funcinter;
        if(tuple.first.find("dec_") == 0 ) { //&& != std::string::npos)// == "dec_Path/2")
            this->_dec_vars = tuple.second;
        }
        ++funcinter;
    }

    for(int i = 0;i<this->_listmovemodelling.size();i++) {
        this->_lConcreteMovemodelling.push_back(new LNSMove((LNSMoveModelling*)this->_listmovemodelling[i], this->_strucInput));
        this->enableMove(i);
        if(_verbosity >= 1)
            std::clog<<"enable move "<<i<<std::endl;
    }

}
void LargeNeighbourhoodSearch::signalChangeCurrentSolution(const Structure *currentsol){
    for(int i = 0;i<_lConcreteMovemodelling.size();i++){
        if(_lConcreteMovemodelling[i]->_move->_queryGetRandVar_Domain!= nullptr)
            if(_lConcreteMovemodelling[i]->resetRandomVariablesDomain(currentsol) == false) {
                disableMove(i);
                std::clog<<"disable move "<<i<<std::endl;
            }
    }
}
bool LNSMove::resetRandomVariablesDomain(const Structure* currentsol){
    if(_randomVariablesDomain != nullptr){
        delete(_randomVariablesDomain);
    }
    _randomVariablesDomain = Querying::doSolveQuery(_move->_queryGetRandVar_Domain, currentsol);
    if(_randomVariablesDomain->empty()) {
        return false;
    }
    _tabulist.empty();
    return true;
}
bool LNSMove::isTabu(int index){
    for(int i = 0;i<_tabulist.size();i++)
        if(index == _tabulist[i])
            return true;
    return false;
}

ElementTuple LNSMove::getVarAtIndex(int index){
    if(this->_verbosity >= 2)
        std::clog << index << "\t";
    if(this->_verbosity >= 3) {
        std::clog<<"\trandom variable's domain: ";
        _randomVariablesDomain->put(std::clog);
        std::clog << std::endl;
    }

    //TODO can have a list once then don't have to iterate everytime like that anymoreeeeeeeeee
    int i = 0;
    ElementTuple de;          //typedef std::vector<const DomainElement*> ElementTuple;
    for (auto it = _randomVariablesDomain->begin(); not it.isAtEnd(); ++it) {
        if(i == index) {
            de = (*it);
            break;
        }
        i++;
    }
    return de;
}


ElementTuple LNSMove::getaRandomVar(){
    int varchosenvariableindex = LocalSearchUtility::getRandom(0, _randomVariablesDomain->size()._size-1);

    if(this->_verbosity >= 2)
        std::clog << varchosenvariableindex << "\t";
    if(this->_verbosity >= 3) {
        std::clog<<"\trandom variable's domain: ";
        _randomVariablesDomain->put(std::clog);
        std::clog << std::endl;
    }

    int index = 0;
    ElementTuple de;          //typedef std::vector<const DomainElement*> ElementTuple;
    for (auto it = _randomVariablesDomain->begin(); not it.isAtEnd(); ++it) {
        if(index == varchosenvariableindex) {
            de = (*it);
            break;
        }
        index++;
    }
    return de;
}
Structure* LargeNeighbourhoodSearch::chooseRandomVars(const Structure *currentsol, LNSMove* chosenmove){
    Structure* strucToGetMove = currentsol->clone();
    strucToGetMove->changeVocabulary(chosenmove->_move->_vocMove);

            if(this->_verbosity >= 2){
                std::clog<<"\t";
            }

    if(chosenmove->_move->_queryGetRandVar_Domain != nullptr){
        ElementTuple de = chosenmove->getaRandomVar();

        for(int i = 0;i<chosenmove->_randVars.size();i++){
            PFSymbol* randvar = chosenmove->_randVars[i];
            //TODO: error-proned, the order of vars in the query must be the same with the order in the vocabulary
            std::vector<const DomainElement*> tuple = std::vector<const DomainElement*>();
            tuple.push_back(de[i]);
            strucToGetMove->inter(randvar)->makeTrueExactly(tuple);
        }

    }
    else{
        int previousindex = -1;
        Sort* preSort = nullptr;

        for(int i = 0;i<chosenmove->_randVars.size();i++){
            PFSymbol* randvar = chosenmove->_randVars[i];
            int randvarsize = chosenmove->_randVars_size[i];
            int vardeletedindex = LocalSearchUtility::getRandomWithSeed(0, randvarsize-1, i*2);

            if(vardeletedindex == previousindex && randvarsize >1 && chosenmove->_randVars_Sort[i] == preSort){
                if(vardeletedindex<(randvarsize/2))
                    vardeletedindex = LocalSearchUtility::getRandomWithSeed(vardeletedindex + 1, randvarsize-1, i*2);
                else
                    vardeletedindex = LocalSearchUtility::getRandomWithSeed(0, vardeletedindex-1, i*2);
            }
            previousindex = vardeletedindex;
            preSort = chosenmove->_randVars_Sort[i];

            const DomainElement* deletedElement = chosenmove->_randVars_DomainList[i][vardeletedindex];
            if(this->_verbosity >= 2){
                deletedElement->put(std::clog);
                std::clog<<" ";
            }
            std::vector<const DomainElement*> tuple = std::vector<const DomainElement*>();
            tuple.push_back(deletedElement);
            strucToGetMove->inter(randvar)->makeTrueExactly(tuple);
        }
    }
    strucToGetMove->makeTwoValued();
    return strucToGetMove;
}

std::shared_ptr<Structure> LargeNeighbourhoodSearch::disturbFromMove(const Structure *currentsol, int moveindex){
    LNSMove* chosenmove = _lConcreteMovemodelling[moveindex];
    Structure* strucToGetMove = chooseRandomVars(currentsol, chosenmove);

    //finish getting random variables, now execute the query to get deleted values
    PredTable* deletedVariables = Querying::doSolveQuery(chosenmove->_move->_queryGetMove, strucToGetMove);

    PredInter* interDecVar = currentsol->inter(this->_dec_vars);

//    std::clog<<"\n~~~~before destroy ~~~~\n"<<std::endl;
//    std::clog<<"ct: ";
//    interDecVar->ct()->put(std::clog);
//    std::clog<<"\npt: ";
//    interDecVar->pt()->put(std::clog);
//    std::clog<<"\ncf: ";
//    interDecVar->cf()->put(std::clog);
//    std::clog<<"\npf: ";
//    interDecVar->pf()->put(std::clog);
//    std::clog<<std::endl;

    int oldsize = 0;
    if(_verbosity>=2)
        oldsize = interDecVar->ct()->size()._size + interDecVar->cf()->size()._size;

    PredTable* newcttable = new PredTable(interDecVar->ct()->internTable(), interDecVar->ct()->universe());
    PredTable* newcftable = new PredTable(interDecVar->cf()->internTable(), interDecVar->cf()->universe());

    UpdateInterWithDeletedTable(newcttable, newcftable, deletedVariables);

        if(_verbosity>=2) {
            std::clog<<"\tdestroy "<<deletedVariables->size()._size<<" of "<<oldsize<<" \t";
        }
                    if(_verbosity>=3){
                        std::clog<<"  ";
                        deletedVariables->put(std::clog);
                        std::clog<<std::endl;
                    }
//    std::clog<<"\n~~~~after destroy ~~~~\n"<<std::endl;
//    interDecVar->put(std::clog);
//    std::clog<<std::endl;

    Structure* nextSolution = this->_strucInput->clone();
    nextSolution->inter(this->_dec_vars)->ct(newcttable);
    nextSolution->inter(this->_dec_vars)->cf(newcftable);

    auto models = ModelExpansion::doMinimization(this->_theo, nextSolution, this->_termObj, NULL)._models;

    if(models.size() == 0)
        std::clog<<"something wrong here, no result on minimization"<<std::endl;
    LocalSearchUtility::cleanUpAfterModelExpand(models,0);
    std::shared_ptr<Structure> result(models[0]);

//    delete nextSolution;
    delete strucToGetMove;
    delete nextSolution;
    return result;
}

void LargeNeighbourhoodSearch::UpdateInterWithDeletedTable(PredTable* cttable, PredTable* cftable, PredTable *deleteTable){
//    PredTable* originct = originIter->ct();
//    PredTable* orgincf = originIter->cf();

    for (auto ctit = cttable->begin(); not ctit.isAtEnd(); ++ctit) {
        if(deleteTable->contains(*ctit)){
            auto tuple = *ctit;
            cttable->remove(tuple);
        }
    }
    for (auto cfit = cftable->begin(); not cfit.isAtEnd(); ++cfit) {
        if(deleteTable->contains(*cfit)){
            auto tuple = *cfit;
            cftable->remove(tuple);
        }
    }
}

LNSMove::LNSMove(LNSMoveModelling* movemodelling, Structure* strucinput){
    _enable = false;
    _verbosity = getOption(IntType::VERBOSE_LOCALSEARCH);
    _move = movemodelling;
    _strucInput = strucinput;
    _randVars = std::vector<PFSymbol*>();
    _randVars_Sort = std::vector<Sort*>();
    _randVars_DomainList = std::vector<const DomainElement**> ();
    _randVars_size = std::vector<int>();

    _tabulist = std::vector<int>();
    _randomVariablesDomain = nullptr;

    auto listfunc = _move->_vocMove->getFuncs();        //std::map<std::string, Function*>&
    auto listpred = _move->_vocMove->getPreds();         //std::map<std::string, Predicate*>&

    auto prediter = listpred.begin();
    while(prediter != listpred.end()){
        auto tuple = *prediter;
        if(tuple.first.find("rand_") == 0 ) //&& != std::string::npos)// == "dec_Path/2")
        {
            _randVars.push_back(tuple.second);
        }
        ++prediter;
    }

    auto funcit = listfunc.begin();              ////std::map<std::string, Function*>&
    while(funcit != listfunc.end()){      // while(!funcinter._M_is_end()){
        auto tuple = *funcit;
        if(tuple.first.find("rand_") == 0 ) //&& != std::string::npos)// == "dec_Path/2")
        {
            _randVars.push_back(tuple.second);
        }
        ++funcit;
    }

    //TODO: redundant, should be processed once, since many rand vars can have the same sort
    for(int i = 0;i<_randVars.size();i++) {
        PFSymbol *var = _randVars[i];
        _randVars_Sort.push_back(var->sort(0));
        SortTable *_randVar_Domain = _strucInput->inter(_randVars_Sort[i]);
        this->_randVars_size.push_back(_randVar_Domain->size()._size);

        auto a = new const DomainElement *[this->_randVars_size[i]];
//        std::clog<<_randVars_DomainList.size();
        this->_randVars_DomainList.push_back(a);
        int j = 0;
        for (auto it = _randVar_Domain->sortBegin(); not it.isAtEnd(); ++it) {
            _randVars_DomainList[i][j++] = (*it);
        }
    }
}