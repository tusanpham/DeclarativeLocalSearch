//
// Created by coolkat on 6-11-17.
//

#pragma once

#include <iostream>
#include "commandinterface.hpp"

#include "inferences/localsearch/Localsearch.hpp"
#include "lua/luaconnection.hpp"


InternalArgument convertToInternalArguments(std::vector<Structure*> models){
    LuaTraceMonitor* tracer = NULL;
    if (getOption(BoolType::TRACE)) {
        tracer = LuaConnection::getLuaTraceMonitor();
    }
    // Convert to internal arguments
    InternalArgument result;
    result._type = AT_TABLE;
    result._value._table = new std::vector<InternalArgument>();
    addToGarbageCollection(result._value._table);
    for (auto it = models.cbegin(); it != models.cend(); ++it) {
        result._value._table->push_back(InternalArgument(*it));
    }
    return result;
}

typedef TypedInference<LIST(AbstractTheory*, Structure*)> LocalSearchInferenceBase;
//typedef TypedInference<LIST(AbstractTheory*, Query*, AbstractTheory*,Query*,Query*,Structure*, Vocabulary*, std::string*)> LocalSearchInferenceBase;
class LocalsearchInference: public LocalSearchInferenceBase{
public:
    LocalsearchInference()
            : LocalSearchInferenceBase("localsearch",
                                "Local search inference. Input: bunch of vocabulary and theory. Output: best, hmm, structure",
                                false) {
        setNameSpace(getInternalNamespaceName());
    }

    InternalArgument execute(const std::vector<InternalArgument>& args) const {
//        std::string* str = get < 2 > (args);
//        std::cout<<"string input: "<<*str<<std::endl;
        auto info = LocalSearch::doLocalSearch(get < 0 > (args), get < 1 > (args));          //return lsData& info
        InternalArgument result = convertToInternalArguments(info);
        return result;
    }
};

typedef TypedInference<LIST(AbstractTheory*, Structure*, Structure*)> LocalSearchWithIniSolInferenceBase;
class LocalsearchWithIniSolInference: public LocalSearchWithIniSolInferenceBase{
public:
    LocalsearchWithIniSolInference()
            : LocalSearchWithIniSolInferenceBase("localsearch",
                                       "Local search inference with initial solution. Input: bunch of vocabulary and theory. Output: best, hmm, structure",
                                       false) {
        setNameSpace(getInternalNamespaceName());
    }
    //AbstractTheory* Tproblem, Structure* instanceS, Structure* initialsolution, std::string* neighbourhoodname

    InternalArgument execute(const std::vector<InternalArgument>& args) const {
        auto info = LocalSearch::doLocalSearch(get < 0 > (args), get < 1 > (args),get < 2 > (args));          //return lsData& info
        InternalArgument result = convertToInternalArguments(info);
        return result;
    }
};