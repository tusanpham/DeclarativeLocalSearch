//
// Created by coolkat on 6-11-17.
//
#pragma once

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
class AbstractMoveModelling;
class MoveModelling;

#include "theory/theory.hpp"
#include "MoveModelling.hpp"
//struct lsData{
//    Theory* _theory;
//    Structure* _iniSol;
//    Structure* _bestSol;
//};

struct lsTimeInfor{
    int _timeout_second;
    long _timeout_clock;              //clock, be careful
    clock_t _timeStart;
    double _timeTotal = 0;
    double _timeGetIniSol =0;
    double _timeGetMove =0;
    double _timeExpansionTnext =0;
    double _timeCreateNeighbor =0;
    double _timeGetDelObj = 0;
    double _timeGetObjValue = 0;
    int _countGetMove = 0;
    int _countGetDelObj;
    int _countExpansionTnext=0;
    int _countCreateNeighbor=0;
    int _countGetObjValue = 0;

    double _avgtimeGetMove =0;
    double _avgtimeExpansionTnext =0;
    double _avgtimeCreateNeighbor =0;
    double _avgtimeGetDelObj = 0;
    double _avgtimeGetObjValue = 0;
};

class LocalSearch {
protected:
    AbstractTheory* _theo;
    Vocabulary* _voc;
    Query* _queryGetObj;
    Term* _termObj;
    std::vector<AbstractMoveModelling*> _listmovemodelling;
    std::vector<std::string> _listMoveName;                        //bad design babe
    std::vector<LocalSearchType> _listLSType;
    PFSymbol* _dec_vars;             //    std::pair<PFSymbol*, PredInter*> dec_vars;
    PFSymbol* _dec_next_vars;        //    std::pair<PFSymbol*, PredInter*> dec_next_path;

    Structure* _strucInput;

public:
    std::shared_ptr<Structure> _initialSol;
    std::shared_ptr<Structure> _bestSol;
    int _iniObjVal;
    int _bestObjVal;

    lsTimeInfor _timeinfor;
    int _verbosity;
    bool _iniSolIsGiven;

    int _mTimeLimitInitialSolution=6000;


public:
    static std::vector<Structure*> doLocalSearch(AbstractTheory *T, Structure *str);
    static std::vector<Structure*> doLocalSearch(AbstractTheory *T, Structure *str, Structure *iniSol);


    virtual void doSearch();

    LocalSearch(AbstractTheory* T, Query*  queryGetObj,Term *termObj, Structure* str, Structure *iniSol);
    LocalSearch(LocalSearch* ls);
    LocalSearch(LocalSearch* ls, AbstractMoveModelling* move);

    void setInitialSolution(std::shared_ptr<Structure> iniSol);
    std::shared_ptr<Structure> getInitialSolution();
    std::shared_ptr<Structure> getNextSolution(Structure* solutionWithMove);
    std::shared_ptr<Structure> getNeighborSolutionWithoutMX(Structure* s);
    std::shared_ptr<Structure> getNeighborSolution(Structure* s);
    int getDeltaObj(Structure const *const structureWithMove);
    int getObjValue(Structure const *const structureWithMove);

    std::shared_ptr<Structure> decodeMove(const Structure *s, const std::vector<const DomainElement *> &m);
    NeighborhoodMove*  getValidMoves(Structure const * const s);
    NeighborhoodMove*  getValidMovesByMX(Structure const * const s);
    NeighborhoodMove*  getValidMovesByQuery(Structure const * const s);
    std::ostream& put(std::ostream& s) const;

    static void processLocalSearchString(std::string* string);
//    void readMoveModellings(std::string* string);
    void readMoveModellings();
    virtual void processInputData();

    bool timeout();
    void refineTimeInfor();
    void printTimeInfor();
    void printResult();
    void testmem(const Structure *str);

    ~LocalSearch();
};

class LocalSearchUtility{
public:
    static int getIntegerFromQuery(Query *q, Structure const *const s);     //query must return a single integer
    static void writeStructure(const Structure* s, std::string filename);
    static void cleanUpAfterModelExpand(std::vector<Structure*>& models, int index);
    static AbstractMoveModelling* getMoveModelling(Namespace* ns, std::string* movename);
    static LNSMoveModelling* getLNSMoveModelling(Namespace* ns, std::string* movename);
    static int getRandom(int a, int b);         //get a random number in [a,b]
    static int getRandomWithSeed(int a, int b, int seed);
    static std::vector<std::string> split(std::string* s, char delim);
    template<typename Out>
    static void split(std::string* s, char delim, Out result);
    static LocalSearchType getLSTypeFromString(std::string s);
};
