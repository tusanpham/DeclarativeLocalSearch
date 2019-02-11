//
// Created by coolkat on 6-11-17.
//

#include <fstream>
#include <inferences/querying/Query.hpp>
#include "Localsearch.hpp"
#include "MoveModelling.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "structure/MainStructureComponents.hpp"
#include "structure/StructureComponents.hpp"

#include "options.hpp"

#include <IncludeComponents.hpp>
#include "structure/Structure.hpp"
#include "theory/theory.hpp"
#include "theory/Query.hpp"
#include "vocabulary/vocabulary.hpp"
#include "NeighborhoodMove.hpp"

#include <time.h>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include "utils/ListUtils.hpp"
#include "BestImprovementSearch.hpp"
#include "FirstImprovementSearch.hpp"
#include "TabuSearch.hpp"
#include "LargeNeighbourhoodSearch.h"
#include "ILS.h"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"

LocalSearch::LocalSearch(AbstractTheory* T, Query*  queryGetObj,Term *termObj,  Structure* str, Structure *iniSol):
        _theo(T), _queryGetObj(queryGetObj), _strucInput(str){
    {
        this->_termObj = termObj;
        if(iniSol == NULL) {
            this->_initialSol = NULL;
            this->_iniSolIsGiven = false;
        }
        else {
            this->_initialSol = std::make_shared<Structure>(*iniSol);
            this->_iniSolIsGiven = true;
        }

        this->_listmovemodelling = std::vector<AbstractMoveModelling*>();
        this->_listMoveName = std::vector<std::string>();
        this->_listLSType = std::vector<LocalSearchType>();
        _voc = _theo->vocabulary();
        _timeinfor._timeout_second = getOption(IntType::TIMEOUT_LOCALSEARCH);
        _timeinfor._timeout_clock = _timeinfor._timeout_second * (CLOCKS_PER_SEC);
        _verbosity = getOption(IntType::VERBOSE_LOCALSEARCH);
        _timeinfor._timeStart = clock();
    }
}


LocalSearch::LocalSearch(LocalSearch* ls, AbstractMoveModelling* move)
        :_theo(ls->_theo), _queryGetObj(ls->_queryGetObj),_strucInput(ls->_strucInput)
{
    this->_voc = ls->_voc;
    this->_termObj = ls->_termObj;
    this->_listmovemodelling = std::vector<AbstractMoveModelling*>();
    this->_listMoveName = std::vector<std::string>();
    this->_listLSType = std::vector<LocalSearchType>();
    if(move != NULL)
        this->_listmovemodelling.push_back(move);
//    for(std::vector<AbstractMoveModelling*>::iterator it = ls->_listmovemodelling.begin(); it != ls->_listmovemodelling.end(); ++it) {
//        this->_listmovemodelling.push_back(*it);
//    }

//    this->_dec_vars = ls->_dec_vars;
//    this->_dec_next_vars = ls->_dec_next_vars;

    this->processInputData();               //important, to read the dec_vars and dec_next_vars
                                            //but messy, in other function, it needs to be called seprately?

    if(ls->_initialSol == NULL) {
        this->_initialSol = NULL;
        this->_iniSolIsGiven = false;
    }
    else {
        this->_initialSol = ls->_initialSol;
        this->_iniSolIsGiven = true;
    }

    _timeinfor._timeout_second = ls->_timeinfor._timeout_second;
    _timeinfor._timeout_clock = ls->_timeinfor._timeout_clock;
    _verbosity = ls->_verbosity;
    _timeinfor._timeStart = clock();

    _mTimeLimitInitialSolution = ls->_mTimeLimitInitialSolution;
}

//copying all moves
LocalSearch::LocalSearch(LocalSearch* ls)
        :LocalSearch(ls, NULL)
{
//    this->_listmovemodelling = std::vector<AbstractMoveModelling*>();
    for(std::vector<AbstractMoveModelling*>::iterator it = ls->_listmovemodelling.begin(); it != ls->_listmovemodelling.end(); ++it) {
        this->_listmovemodelling.push_back(*it);
    }
    for(std::vector<std::string>::iterator it = ls->_listMoveName.begin(); it != ls->_listMoveName.end(); ++it) {
        this->_listMoveName.push_back(*it);
    }
    for(std::vector<LocalSearchType>::iterator it = ls->_listLSType.begin(); it != ls->_listLSType.end(); ++it) {
        this->_listLSType.push_back(*it);
    }
}

void LocalSearch::setInitialSolution(std::shared_ptr<Structure> iniSol){
    this->_initialSol = iniSol; //std::make_shared<Structure>(ls->_initialSol.get());
    this->_iniSolIsGiven = true;

}

void LocalSearch::testmem(const Structure *str){
    int i = 0;
//    auto inisol = getInitialSolution();
    while(true){
        std::cout<<"initial solution "<<i<<std::endl;
//        _iniObjVal = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, inisol);
        PredTable* result = Querying::doSolveQuery(_queryGetObj, str);
        delete result;
//        auto newstruc = _strucInput->clone();
//        auto models = ModelExpansion::doModelExpansion(_theo, newstruc, NULL)._models;        //_models is std::vector<Structure*>& v
//        delete newstruc;
//        for(int j = 0;j<models.size(); j++)
//            delete(models[j]);
        i++;
    }
}

std::vector<Structure*> LocalSearch::doLocalSearch(AbstractTheory *T, Structure *s){
    Structure *iniSol = NULL;
    return LocalSearch::doLocalSearch(T, s, iniSol);
}


std::vector<Structure*> LocalSearch::doLocalSearch(AbstractTheory *T, Structure *str, Structure *iniSol){
    Namespace* ns = getGlobal()->getGlobalNamespace();

    Query *getObj;
    if(ns->isQuery("objVal"))
        getObj = ns->query("objVal");
    else
        getObj = ns->query("ObjValue");

    Term *termObj = ns->term("Obj");

    auto lsstring = getGlobal()->getOptions()->localsearchType();

    auto result = std::vector<Structure*>();
    LocalSearch* ls;


    //TODO: check validity of the input string
    std::vector<std::string> listls = LocalSearchUtility::split(&lsstring, ';');
    //listls[i] is "firstimprove,2opt" right now, support firstimprove, bestimprove, tabu, lns; in case there are more than one, use ILS
    std::vector<std::string> lsoptions = LocalSearchUtility::split(&listls[0], ',');
    LocalSearchType lstype = LocalSearchUtility::getLSTypeFromString(lsoptions[0]);
    if(listls.size()>1){
        lstype = LocalSearchType::ILS;
    }

    switch(lstype){
        case LocalSearchType::BESTIMPROVE:
            ls = new BestImprovementSearch(T, getObj, termObj,str, iniSol);
            break;
        case LocalSearchType ::TABU:
            ls = new TabuSearch(T, getObj,termObj ,str, iniSol);
            break;
        case LocalSearchType ::LNS:
            ls = new LargeNeighbourhoodSearch(T, getObj,termObj, str, iniSol);
            break;
        case LocalSearchType ::ILS:
            ls = new ILS(T, getObj,termObj, str, iniSol);
            break;
        case LocalSearchType::FIRSTIMPROVE:
        default:                                                                         //first improve
            std::clog<<"this is first improve   \n";
            ls = new FirstImprovementSearch(T, getObj,termObj,str, iniSol);
            break;
    }

    for(int i = 0;i<listls.size();i++) {
        std::vector<std::string> lsoptions = LocalSearchUtility::split(&listls[i], ',');
        LocalSearchType sublstype = LocalSearchUtility::getLSTypeFromString(lsoptions[0]);
        ls->_listLSType.push_back(sublstype);
        ls->_listMoveName.push_back(lsoptions[1]);
    }

    ls->readMoveModellings();

    ls->processInputData();

    ls->doSearch();
    result.push_back(ls->_bestSol.get());

//    LocalSearchUtility::writeStructure(ls->_initialSol, "/home/ck/Desktop/idpexperiment/temp.txt");
    ls->_timeinfor._timeTotal =clock() - ls->_timeinfor._timeStart;
    ls->refineTimeInfor();

    ls->printResult();

    return result;
}
//
//void LocalSearch::processLocalSearchString(std::string* str){
//    //string has format firstimprove(move1); bestimprove(move2); tabu(move3)     (names of move)
//    //if more than 1 move, use ILS
//    std::clog<<"hi "<<str<<"\n";
//    Namespace* ns = getGlobal()->getGlobalNamespace();
//    //TODO: check validity of the input string
//    std::vector<std::string> listls = LocalSearchUtility::split(str, ';');
//    //listls[i] is "firstimprove,2opt"
//    //right now, support firstimprove, bestimprove, tabu, lns
//    //in case there are more than one, use ILS
//    std::vector<std::string> lsoptions = LocalSearchUtility::split(&listls[0], ',');
//    LocalSearchType lstype = LocalSearchUtility::getLSTypeFromString(lsoptions[0]);
//    if(listls.size()>1){
//        lstype = LocalSearchType::ILS;
//    }
//
//    LocalSearch* ls;
//    switch(lstype){
//    case LocalSearchType::BESTIMPROVE:
//        ls = new BestImprovementSearch(T, getObj, termObj,str, iniSol);
//        break;
//    case LocalSearchType ::TABU:
//        ls = new TabuSearch(T, getObj,termObj ,str, iniSol);
//        break;
//    case LocalSearchType ::LNS:
//        ls = new LargeNeighbourhoodSearch(T, getObj,termObj, str, iniSol);
//        break;
//    case LocalSearchType ::ILS:
//        ls = new ILS(T, getObj,termObj, str, iniSol);
//        break;
//    case LocalSearchType::FIRSTIMPROVE:
//    default:                                                                         //first improve
//        ls = new FirstImprovementSearch(T, getObj,termObj,str, iniSol);
//        break;
//}
//
//    for(int i = 0;i<listls.size();i++) {
//        std::vector<std::string> lsoptions = LocalSearchUtility::split(&listls[i], ',');
//        LocalSearchType lstype = LocalSearchUtility::getLSTypeFromString(lsoptions[0]);
//    }
//
//}
//void LocalSearch::readMoveModellings(std::string* string){
//    //string has format move1; move2; move3     (names of move)
//    Namespace* ns = getGlobal()->getGlobalNamespace();
//
//    if (_verbosity >= 1)
//        std::clog<<"~~~~~~~~~~~~~reading move modellings "<<*string<<std::endl;
//    std::vector<std::string> liststring = LocalSearchUtility::split(string, ';');
//
//    for(int i = 0;i<liststring.size();i++)
//    {
//        AbstractMoveModelling* move;
//        if(_verbosity >= 1) {
//            std::clog << "reading move " << i << ": " << liststring[i] << std::endl;    //tuple.first.find("dec_") == 0 )
//        }
//        //TODO: for now, just make it simply by indicate by lns prefix
//        if(liststring[i].find("lns") == 0){
//            move = LocalSearchUtility::getLNSMoveModelling(ns, &liststring[i]);
//        }
//        else{
//            move = LocalSearchUtility::getMoveModelling(ns, &liststring[i]);
//        }
//        this->_listmovemodelling.push_back(move);
//    }
//}

void LocalSearch::readMoveModellings(){
    //string has format move1; move2; move3     (names of move)
    Namespace* ns = getGlobal()->getGlobalNamespace();

    if (_verbosity >= 1)
        std::clog<<"~~~~~~~~~~~~~reading move modellings "<<std::endl;

    for(int i = 0;i<this->_listMoveName.size();i++)
    {
        AbstractMoveModelling* move;
        if(_verbosity >= 1) {
            std::clog << "reading move " << i << ": " << _listMoveName[i] << std::endl;    //tuple.first.find("dec_") == 0 )
        }
        //TODO: for now, just make it simply by indicating by lns prefix
        if(_listMoveName[i].find("lns") == 0){
            move = LocalSearchUtility::getLNSMoveModelling(ns, &_listMoveName[i]);
        }
        else{
            move = LocalSearchUtility::getMoveModelling(ns, &_listMoveName[i]);
        }
        this->_listmovemodelling.push_back(move);
    }
}


void LocalSearch::doSearch(){
    auto inisol = getInitialSolution();              //return std::vector<Structure*>
    _initialSol = inisol;

    _bestObjVal = _iniObjVal;
    _bestSol = NULL;
}

LNSMoveModelling* LocalSearchUtility::getLNSMoveModelling(Namespace* ns, std::string *movename){
    std::string vmovestr = "Vmove_";
    vmovestr.append(*movename);
    std::string querygetmovestr = "getMove_";
    querygetmovestr.append(*movename);
    std::string querygetranddomanin = "getRandomVars_";
    querygetranddomanin.append(*movename);

    Vocabulary *Vmove = ns->vocabulary(vmovestr);                   //("Vmove");
    Query* queryGetMove = ns->query(querygetmovestr);               //("queryGetMove");
    Query* queryGetRandomVarDomain = nullptr;
    if(ns->isQuery(querygetranddomanin))
        queryGetRandomVarDomain = ns->query(querygetranddomanin);

    LNSMoveModelling* move = new LNSMoveModelling(movename, Vmove, queryGetMove, queryGetRandomVarDomain);
    return move;
}
AbstractMoveModelling* LocalSearchUtility::getMoveModelling(Namespace* ns, std::string* _movename){
    std::string movename(*_movename);
    std::string theoryNextStr = "Tnext_";
    theoryNextStr.append(movename);
    AbstractTheory *Tnext = ns->theory(theoryNextStr);               //"Tnext"

    std::string vmovestr = "Vmove_";
    vmovestr.append(movename);
    Vocabulary *Vmove = ns->vocabulary(vmovestr);                   //"Vmove"

    //here, if queryGetDelObj is not provided, then, stuff
    std::string queryDeltaObjStr = "getDeltaObj_";
    queryDeltaObjStr.append(movename);
    Query *queryDeltaObj = NULL;
    if(ns->isQuery(queryDeltaObjStr))           // ns->queries().count("queryGetDelObj") > 0)              //exists
        queryDeltaObj = ns->query(queryDeltaObjStr);

    Query *queryNextObj = NULL;
    std::string queryNextCostStr = "getNextCost_";
    queryNextCostStr.append(movename);
    if(ns->isQuery(queryNextCostStr))                    //  ns->queries().count("queryNextCost") > 0)
        queryNextObj = ns->query(queryNextCostStr);


    // now check queryValidMove
    Query *queryGetValidMoves = NULL;
    std::string getpossibleMovesStr = "getValidMoves_";
    getpossibleMovesStr.append(movename);
    if(ns->isQuery(getpossibleMovesStr)){     //             ns->queries().count("getpossibleMoves") > 0) {
        queryGetValidMoves = ns->query(getpossibleMovesStr);
        //AbstractTheory* Tnext,    Query*  queryValidMoves, Query*  queryDeltaObj, Vocabulary* Vmove
        AbstractMoveModelling * mmodelling = new MoveModelling{&movename, Tnext, queryGetValidMoves, queryDeltaObj, queryNextObj, Vmove};
        return mmodelling;
    }


    AbstractTheory *Tmoves = NULL;
    std::string TmovesStr = "Tmoves_";
    TmovesStr.append(movename);
    if(ns->isTheory(TmovesStr)){          //  ns->theories().count("Tmoves") > 0) {
        Tmoves = ns->theory(TmovesStr);
        //moveModelling( AbstractTheory* Tnext, Query*  queryDeltaObj,Query* queryNextCost, Vocabulary* Vmove,AbstractTheory* TgetValidMoves):
        AbstractMoveModelling * mmodelling = new MoveModelling{&movename, Tnext, queryDeltaObj, queryNextObj, Vmove, Tmoves};
        return mmodelling;
    }
}

//#include <stdlib.h>

int  LocalSearchUtility::getRandom(int a, int b){
    srand (time(NULL));
    int i = rand() % (b - a + 1) + a;
    return i;
}
int  LocalSearchUtility::getRandomWithSeed(int a, int b, int seed){
    srand (clock() + seed + 2);         //avoid 1, since 1 is special
    //If seed is set to 1, the generator is reinitialized to its initial value
    // //and produces the same values as before any call to rand or srand. Geezz
    int i = rand() % (b - a + 1) + a;
    return i;
}
int LocalSearch::getDeltaObj(Structure const *const structureWithMove){
    time_t start = clock();
    auto deltaObj = LocalSearchUtility::getIntegerFromQuery(((MoveModelling*)_listmovemodelling[0])->_queryDeltaObj, structureWithMove);
    _timeinfor._timeGetDelObj += clock() - start;
    _timeinfor._countGetDelObj++;
    return deltaObj;
}
int LocalSearch::getObjValue(Structure const *const str)
{
    time_t start = clock();
    auto obj = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, str);
    _timeinfor._timeGetObjValue += clock() - start;
    _timeinfor._countGetObjValue++;
    return obj;
}
bool LocalSearch::timeout(){
    if((clock() - _timeinfor._timeStart) > _timeinfor._timeout_clock) {
        return true;
    }
    return false;
}
void LocalSearch::refineTimeInfor() {
    _timeinfor._timeTotal = _timeinfor._timeTotal/(CLOCKS_PER_SEC);
    _timeinfor._timeGetIniSol = _timeinfor._timeGetIniSol/(CLOCKS_PER_SEC);

    if(_timeinfor._countGetMove > 0) {
        _timeinfor._timeGetMove = _timeinfor._timeGetMove/(CLOCKS_PER_SEC);
        _timeinfor._avgtimeGetMove = _timeinfor._timeGetMove / _timeinfor._countGetMove;
    }
    if(_timeinfor._countExpansionTnext > 0) {
        _timeinfor._timeExpansionTnext = _timeinfor._timeExpansionTnext/(CLOCKS_PER_SEC);
        _timeinfor._avgtimeExpansionTnext = _timeinfor._timeExpansionTnext / _timeinfor._countExpansionTnext;

    }
    if(_timeinfor._countCreateNeighbor > 0) {
        _timeinfor._timeCreateNeighbor = _timeinfor._timeCreateNeighbor/(CLOCKS_PER_SEC);
        _timeinfor._avgtimeCreateNeighbor = _timeinfor._timeCreateNeighbor / _timeinfor._countCreateNeighbor;

    }
    if(_timeinfor._countGetDelObj > 0) {
        _timeinfor._timeGetDelObj = _timeinfor._timeGetDelObj/(CLOCKS_PER_SEC);
        _timeinfor._avgtimeGetDelObj = _timeinfor._timeGetDelObj / _timeinfor._countGetDelObj;
    }
    if(_timeinfor._countGetObjValue > 0){
        _timeinfor._timeGetObjValue = _timeinfor._timeGetObjValue/(CLOCKS_PER_SEC);
        _timeinfor._avgtimeGetObjValue = _timeinfor._timeGetObjValue / _timeinfor._countGetObjValue;
    }
}

void LocalSearch::printTimeInfor(){
    std::cout<<"timeTotal; \t\t"<<_timeinfor._timeTotal<<"\r\n";
//    double percent_getinisol = _timeinfor._timeGetIniSol/_timeinfor._timeTotal;
    std::cout<<"timeGetIniSol; \t\t"<<_timeinfor._timeGetIniSol<<";\t"<< "\r\n";
    std::cout<<"timeGetMove; \t\t"<<_timeinfor._timeGetMove
            <<";\t"<<_timeinfor._countGetMove
            <<"; "<<_timeinfor._avgtimeGetMove
            <<"\r\n";
    std::cout<<"timeMXTnext; \t\t"<<_timeinfor._timeExpansionTnext
            <<";\t"<<_timeinfor._countExpansionTnext
            <<"; "<<_timeinfor._avgtimeExpansionTnext
             <<"\r\n";
    std::cout<<"timeCreateNeighbour; \t"<<_timeinfor._timeCreateNeighbor
            <<";\t"<<_timeinfor._countCreateNeighbor
            <<"; "<<_timeinfor._avgtimeCreateNeighbor
             <<"\r\n";
    std::cout<<"timeGetDelObj; \t\t"<<_timeinfor._timeGetDelObj
            <<";\t"<<_timeinfor._countGetDelObj
            <<"; "<<_timeinfor._avgtimeGetDelObj
             <<"\r\n";

    std::cout<<"timeGetObjValue; \t\t"<<_timeinfor._timeGetObjValue
             <<";\t"<<_timeinfor._countGetObjValue
             <<"; "<<_timeinfor._avgtimeGetObjValue
             <<"\r\n";
}

void LocalSearch::printResult() {
    std::cout<<"best obj: "<<_bestObjVal<<"\r\n";
    if(this->_iniSolIsGiven)
        std::cout<<"ini sol (given): "<<_iniObjVal<<"\r\n";
    else
        std::cout<<"ini sol: "<<_iniObjVal<<"\r\n";
    printTimeInfor();
}
std::shared_ptr<Structure> LocalSearch::getInitialSolution() {
    time_t start = clock();

    setOption(IntType::MXTIMEOUT, _mTimeLimitInitialSolution);

    Structure* structurein;
    if(_iniSolIsGiven == true){                 //then start with this initial solution
        structurein = _initialSol.get();
    }else{                                      //then start with the input structure
        structurein = _strucInput;
    }
    //TODO: what happen with the pointer _iniSolution here when point it to a new place?
    auto models = ModelExpansion::doModelExpansion(_theo, structurein, NULL)._models;

    _timeinfor._timeGetIniSol = clock() - start;
    int index = 0;                  //TODO: some randomness here?
    _initialSol = std::make_shared<Structure>(*models[index]);
    _iniObjVal = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, _initialSol.get());
    LocalSearchUtility::cleanUpAfterModelExpand(models,index);

    if(_verbosity >= 1) {
        if(_iniSolIsGiven)
            std::clog << "running local search with a given initial solution, obj = " << _iniObjVal << "\n";
        else
            std::clog << "finish getting initial solution, obj = " << _iniObjVal << "\n";
    }

    return _initialSol;
}


void LocalSearch::processInputData(){

    auto listfunc = ((MoveModelling*)_listmovemodelling[0])->_vocNext->getFuncs();        //std::map<std::string, Function*>&
    auto listpred = ((MoveModelling*)_listmovemodelling[0])->_vocNext->getPreds();         //std::map<std::string, Predicate*>&

    auto prediter = listpred.begin();
    while(prediter != listpred.end()){
        auto tuple = *prediter;
        if(tuple.first.find("dec_") == 0 ) //&& != std::string::npos)// == "dec_Path/2")
        {
            this->_dec_vars = tuple.second;
        }
        if(tuple.first.find("next_dec_") == 0) { //== "next_dec_Path/2")
            this->_dec_next_vars = tuple.second;
        }
        ++prediter;
    }
    auto funcinter = listfunc.begin();              ////std::map<std::string, Function*>&
    while(funcinter != listfunc.end()){      // while(!funcinter._M_is_end()){
        auto tuple = *funcinter;
        if(tuple.first.find("dec_") == 0 ) { //&& != std::string::npos)// == "dec_Path/2")
            this->_dec_vars = tuple.second;
        }
        if(tuple.first.find("next_dec_") == 0) { //== "next_dec_Path/2")
            this->_dec_next_vars = tuple.second;
        }
        ++funcinter;
    }
}
std::shared_ptr<Structure> LocalSearch::getNeighborSolutionWithoutMX(Structure* solWithMoveExpanded)
{
    time_t start = clock();
    Structure* solution_next = solWithMoveExpanded;
    PredInter* inter_dec_next_vars = solution_next->inter(this->_dec_next_vars)->clone();

    Structure* neighbor = this->_strucInput->clone();
    if(this->_dec_vars->isPredicate()) {
        neighbor->changeInter(dynamic_cast<Predicate*>(this->_dec_vars),inter_dec_next_vars);
    }
    else{       //if function
        FuncInter* finter = neighbor->inter(dynamic_cast<Function*> (this->_dec_vars));
        finter->graphInter(inter_dec_next_vars);
    }
    neighbor->changeVocabulary(this->_voc);
    auto neighborexp = ModelExpansion::doModelExpansion(this->_theo, neighbor, NULL)._models;
    if(neighborexp.size() == 0)
        std::clog<<"something wrong here, no result on neighbor expansion"<<std::endl;

    LocalSearchUtility::cleanUpAfterModelExpand(neighborexp,0);
    delete(neighbor);

    std::shared_ptr<Structure> result(neighborexp[0]);

    _timeinfor._timeCreateNeighbor += clock() - start;
    _timeinfor._countCreateNeighbor++;

    return result;
}
/***
 *
 * @param solutionWithMove solution with move decoded already but not yet expanded
 */
std::shared_ptr<Structure> LocalSearch::getNeighborSolution(Structure* solutionWithMove){
    time_t start = clock();
    auto models = ModelExpansion::doModelExpansion(((MoveModelling*)_listmovemodelling[0])->_theoNext, solutionWithMove, NULL)._models;        //_models is std::vector<Structure*>&
    LocalSearchUtility::cleanUpAfterModelExpand(models,0);

    _timeinfor._timeExpansionTnext += clock() - start;
    _timeinfor._countExpansionTnext++;
    return getNeighborSolutionWithoutMX(models[0]);
}

std::shared_ptr<Structure> LocalSearch::getNextSolution(Structure* solutionWithMove){
    time_t start = clock();
    auto models = ModelExpansion::doModelExpansion(((MoveModelling*)_listmovemodelling[0])->_theoNext, solutionWithMove, NULL)._models;        //_models is std::vector<Structure*>& v

    std::shared_ptr<Structure> result(models[0]);
    LocalSearchUtility::cleanUpAfterModelExpand(models,0);

    _timeinfor._timeExpansionTnext += clock() - start;
    return result;
}

std::shared_ptr<Structure> LocalSearch::decodeMove(const Structure *s, const std::vector<const DomainElement *> &m)
{
    auto newstruc = std::shared_ptr<Structure>(s->clone());
    newstruc->changeVocabulary(((MoveModelling*)_listmovemodelling[0])->_vocNext);

    //TODO NOTE the query output must be in the same order as vocabulary Vmove
    int i = 0;
    auto func = ((MoveModelling*)_listmovemodelling[0])->_vocMove->firstFunc();         //std::map<std::string, Function*>&
    while(func != ((MoveModelling*)_listmovemodelling[0])->_vocMove->lastFunc()) {
        if (func->second->builtin()) {
            ++func;
            continue;
        }
        auto funciter = newstruc->inter(func->second);
        PredInter* graph = funciter->graphInter();
        graph->makeTrueExactly({m[i]});
        funciter->materialize();
        ++func;
        i++;
    }
    return newstruc;
}
int LocalSearchUtility::getIntegerFromQuery(Query *q, Structure const *const s){
    PredTable* result = Querying::doSolveQuery(q, s);
    if(result->empty())
        std::clog  << "hello, error in getIntegerFromQuery here" <<"\n";
    TableIterator iterator = result->begin();
    auto detup = *iterator;         //ElementTuple&, or std::vector<const DomainElement*>
    auto data = detup.data();
    //TODO: check for error here, data includes 1 element only and it must be an integer
    if(data[0]->type()!= DET_INT)
        std::clog  << "error in getIntegerFromQuery - wrong type" <<std::endl;
    auto obj = data[0]->value()._int;
    delete result;
    return obj;
}

NeighborhoodMove*  LocalSearch::getValidMovesByMX(Structure const * const s){
//      by using calculate definition
    Structure* ss = s->clone();
    Structure* result;
    ss->changeVocabulary(((MoveModelling*)_listmovemodelling[0])->_vocValidMoves);
    Theory* moveclone =  dynamic_cast<Theory*>(((MoveModelling*)_listmovemodelling[0])->_theoGetValidMoves->clone());
//    if (moveclone == NULL) {  throw notyetimplemented("Calculate definition of already ground theories"); }
    auto sols = CalculateDefinitions::doCalculateDefinitions(moveclone, ss, false);

//    if(sols._hasModel) {
    result = sols._calculated_model;

    Predicate* validmoves = ((MoveModelling*)_listmovemodelling[0])->_vocValidMoves->pred("validmove/2");
    PredInter* inter_validmoves = result->inter(validmoves)->clone();

    moveclone->recursiveDelete();
    delete ss;

    PredTable* table = inter_validmoves->ct();
    NeighborhoodMove* moves = new NeighborhoodMove(table);
    return moves;
}

//NeighborhoodMove*  LocalSearch::getValidMovesByMX(Structure const * const s){
//    //by using model expand
//    Structure* ss = s->clone();                 //it needs to be deleted at some point, right?
//    ss->changeVocabulary(this->_movemodelling->_vocValidMoves);
//
////    ss->put(std::clog);
//    auto models = ModelExpansion::doModelExpansion
//            (this->_movemodelling->_theoGetValidMoves, ss, NULL)._models;        //_models is std::vector<Structure*>& v
//    LocalSearchUtility::cleanUpAfterModelExpand(models,0);
//
//    Predicate* validmoves = this->_movemodelling->_vocValidMoves->pred("validmove/2");
//    PredInter* inter_validmoves = models[0]->inter(validmoves)->clone();
//
//    delete ss;
//    PredTable* table = inter_validmoves->ct();
//    NeighborhoodMove* moves = new NeighborhoodMove(table);
//    return moves;
//}

NeighborhoodMove*  LocalSearch::getValidMovesByQuery(Structure const * const s){
    PredTable* result = Querying::doSolveQuery(((MoveModelling*)_listmovemodelling[0])->_queryValidMoves, s);
    NeighborhoodMove* moves = new NeighborhoodMove(result);
    if (this->_verbosity >= 1) {
        if(result->empty()) {
            std::clog << "no valid moves found" << "\n";
        }
    }
    return moves;
}


NeighborhoodMove* LocalSearch::getValidMoves(Structure const * const s){
    time_t start = clock();

    if(_verbosity >= 2){
        std::clog<<"~~~~~~~~~~start getting moves~~~~~~~~~~~~~~~~~~~~~~~~~~"<<std::endl;
//        s->put(std::clog);
    }
    if(((MoveModelling*)_listmovemodelling[0])->_queryValidMoves != NULL) {
        NeighborhoodMove* moves = getValidMovesByQuery(s);
        if(_verbosity >= 2)
            std::clog<<"~~~~~~~~~~finish getting moves by query~~~~~~~~~~~~~~~~~~~~~~~~~~"<<std::endl;
        _timeinfor._timeGetMove += clock()-start;
        _timeinfor._countGetMove++;
        return moves;
    }
    else if(((MoveModelling*)_listmovemodelling[0])->_theoGetValidMoves != NULL){
        NeighborhoodMove* moves = getValidMovesByMX(s);
        if(_verbosity >= 2)
            std::clog<<"~~~~~~~~~~finish getting moves by MX~~~~~~~~~~~~~~~~~~~~~~~~~~" <<std::endl;
        _timeinfor._timeGetMove += clock()-start;
        _timeinfor._countGetMove++;
        return moves;
    }
    else{
        std::clog<<"error: no component defining moves is found!"<<std::endl;
    }
    return NULL;
}

std::ostream& LocalSearch::put(std::ostream& s) const{
    s<<"best obj: "<<this->_bestObjVal<<" running time: "<<this->_timeinfor._timeTotal<<"\n";
    return s;
}

void LocalSearchUtility::writeStructure(const Structure *s, std::string filename) {
//    std::string b = "hallo";
//    std::string nametext = "Your name is " + name;

    std::ofstream myfile;
    myfile.open (filename);
    s->put(myfile);
    myfile.close();
}

void LocalSearchUtility::cleanUpAfterModelExpand(std::vector<Structure*>& models, int index){
    //delete all models except the one at index
    for(int i = 0;i<models.size();i++)
        if(i != index)
            delete(models[i]);
}

LocalSearch::~LocalSearch() {
//    if(_initialSol != NULL)
//        delete _initialSol;
//    if(_movemodelling != NULL)
//        delete _movemodelling;
}

std::vector<std::string> LocalSearchUtility::split(std::string* s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}
template<typename Out>
void LocalSearchUtility::split(std::string* s, char delim, Out result) {
    std::stringstream ss(*s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        //s->erase(std::remove_if(s->begin(), s->end(), isspace), s->end());
        *(result++) = item;
    }
}

//temp, it's baddddddd
LocalSearchType LocalSearchUtility::getLSTypeFromString(std::string s){
    if(s.compare("firstimprove")==0)
        return LocalSearchType::FIRSTIMPROVE;
    else if(s.compare("bestimprove")==0)
        return LocalSearchType::BESTIMPROVE;
    else if(s.compare("tabu")==0)
        return LocalSearchType::TABU;
    else if(s.compare("LNS")==0)
        return LocalSearchType::LNS;
    else {                        //default
        Warning::warning("Encountered unsupported local type, assuming FIRSTIMPROVE.\n");
        return LocalSearchType::FIRSTIMPROVE;
    }
//    auto values = Options.possibleValues<LocalSearchType>();
//    for (auto i = values.cbegin(); i != values.cend(); ++i) {
//        if (s->compare(str(*i)) == 0) {
//            return *i;
//        }
//    }
//    Warning::warning("Encountered unsupported local type, assuming FIRSTIMPROVE.\n");
//    return LocalSearchType::FIRSTIMPROVE;
}

//LocalSearchType  Options::localsearchType() const {
//	auto values = possibleValues<LocalSearchType>();
//	const std::string& value = StringPol::getValue(StringType::LOCALSEARCHTYPE);
//    std::clog<<value<<" hi \n";
//	for (auto i = values.cbegin(); i != values.cend(); ++i) {
//		if (value.compare(str(*i)) == 0) {
//			return *i;
//		}
//	}
//	Warning::warning("Encountered unsupported language option, assuming NONE.\n");
//	return LocalSearchType::FIRSTIMPROVE;
//}