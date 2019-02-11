//
// Created by san on 21.07.18.
//

#ifndef IDP_LARGENEIGHBOURHOODSEARCH_H
#define IDP_LARGENEIGHBOURHOODSEARCH_H

class LocalSearch;
class Structure;
class AbstractTheory;
class Vocabulary;
class Theory;
class Query;
class Sort;
class NeighborhoodMove;
class MoveModelling;
class PFSymbol;
class PredInter;
class Term;
class PredTable;


//materialize each movemodelling with the given current initial solution
class LNSMove{
public:
    bool _enable;
    int _verbosity;
    LNSMoveModelling* _move;
    Structure* _strucInput;
    std::vector<PFSymbol*> _randVars;
    std::vector<Sort*> _randVars_Sort;
    std::vector<const DomainElement**> _randVars_DomainList;            //those two are to serve
    std::vector<int> _randVars_size;                                    //the random generated domain elements

    PredTable* _randomVariablesDomain;            //everytime current solution is updated, solve it
    std::vector<int> _tabulist;         //is valid when current solution is not updated yet

    LNSMove(LNSMoveModelling* movemodelling, Structure* strucinput);
    bool resetRandomVariablesDomain(const Structure* cursol);       //everytime current solution is updated; return false if domain is empty
    bool isTabu(int index);
    ElementTuple getaRandomVar();
    ElementTuple getVarAtIndex(int index);
//    typedef std::vector<const DomainElement*> ElementTuple;
//    typedef std::vector<ElementTuple> ElementTable;
};


class LargeNeighbourhoodSearch: public LocalSearch{
protected:
    int _averagesize;
    int _mMinDisturbSize = 10;
    int _mMaxDisturbSize = 50;
    int _mMaxItersNoImprove = 10;
    int _mTimeLimitMinimizeStep=6000;

    std::vector<int> _lEnabledMoves;                    //index of enabled moves in _lConcreteMovemodelling
    std::vector<LNSMove*> _lConcreteMovemodelling;
public:
    LargeNeighbourhoodSearch(AbstractTheory* T, Query* queryGetObj,Term* termObj, Structure* inputstr, Structure *iniSol);
    ~LargeNeighbourhoodSearch();
    void doSearch();
    void processInputData();


    std::shared_ptr<Structure> disturbSolutionRandomly(const Structure *currentsol, int strength);
    std::shared_ptr<Structure> disturbFromMove(const Structure *currentsol, int moveindex);
protected:
    void enableMove(int index);
    void disableMove(int index);
    int getNextMove(int currentmoveindex);        //without random, I mean
    int getRandomMove(int iterationsWithoutImprovement);            //get a random move in list of enabled moves


    Structure* chooseRandomVars(const Structure *currentsol, LNSMove* chosenmove);
    void changeInterRandomly(PredInter *inter, int strength);
    void UpdateInterWithDeletedTable(PredTable* cttable, PredTable* cftable, PredTable *deletedTable);
    void signalChangeCurrentSolution(const Structure *currentsol);         //process all the LNSMove correspondingly
};



//typedef std::vector<const DomainElement*> ElementTuple;
//typedef std::vector<ElementTuple> ElementTable;

#endif //IDP_LARGENEIGHBOURHOODSEARCH_H
