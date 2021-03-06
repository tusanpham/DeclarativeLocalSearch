/*
 * Copyright 2007-2011 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat and Maarten Mariën, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */
#include "external/Tasks.hpp"

#include "space/Remapper.hpp"
#include "external/Translator.hpp"
#include "space/SearchEngine.hpp"
#include "theorysolvers/PropagatorFactory.hpp"
#include "modules/aggsolver/AggProp.hpp"
#include "modules/aggsolver/AggSet.hpp"
#include "modules/IntVar.hpp"
#include "external/SearchMonitor.hpp"
#include "external/FlatZincRewriter.hpp"
#include "external/ECNFPrinter.hpp"
#include "constraintvisitors/CNFPrinter.hpp"
#include "constraintvisitors/ECNFGraphPrinter.hpp"
#include "constraintvisitors/HumanReadableParsingPrinter.hpp"
#include "Printer.hpp"
#include "ModelManager.hpp"
#include "external/utils/ResourceManager.hpp"
#include "external/Space.hpp"

#include "TaskHelpers.hpp"

#include <map>
#include <vector>
#include <bitset>

using namespace std;
using namespace MinisatID;


void Monitor::notifyMonitor(const Lit& propagation, int decisionlevel) {
	for (auto i = monitors.cbegin(); i < monitors.cend(); ++i) {
		if (remapper->wasInput(propagation)) {
			(*i)->notifyPropagated(remapper->getLiteral(propagation), decisionlevel);
		}
	}
}

void Monitor::notifyMonitor(int untillevel) {
	for (auto i = monitors.cbegin(); i < monitors.cend(); ++i) {
		(*i)->notifyBacktracked(untillevel);
	}
}

VarID VarCreation::createID(){
	return {remapper->getNewID()};
}

Atom VarCreation::createVar() {
	return remapper->getNewVar();
}

/*
 * Task
 */
void Task::notifyTerminateRequested() {
	terminate = true;
}

/*
 * SpaceTask
 */
SpaceTask::SpaceTask(Space* space)
		: Task(space->getOptions()), space(space) {

}
SearchEngine& SpaceTask::getSolver() const {
	return *getSpace()->getEngine();
}
void SpaceTask::notifyTerminateRequested() {
	Task::notifyTerminateRequested();
	space->getEngine()->notifyTerminateRequested();
}

/**
 * FindModels
 */

FindModels::FindModels(Space* space, ModelExpandOptions opts)
  : ModelExpand(space,opts), nbModels(opts.nbmodelstofind){
}

FindModels::~FindModels(){
}

void FindModels::execute(){
  printer->notifyStartSolving();
	if (getSpace()->isCertainlyUnsat()) {
		printer->notifySolvingFinished();
		return;
	}
  
	printSearchStart(clog, getOptions().verbosity);
  
  auto state = getSolver().solve(true);
  if (state == l_Undef || terminateRequested()) {
    printer->notifySolvingAborted();
    return;
  }else if(state==l_False){
    printer->notifySolvingFinished();
    getSpace()->notifyUnsat();
    return;
  }else{ // model found :)
    addModel(getSpace()->getEngine()->getModel());
  }
  
  while(nbModels==0 || nbModels > _solutions->getNbModelsFound()){
    Disjunction invalidation({});
    getSolver().invalidate(invalidation.literals);
    invalidateModel(invalidation);
    
    state = getSolver().solve(true);
    if (state == l_Undef || terminateRequested()) {
      printer->notifySolvingAborted();
      return;
    }else if(state==l_False){
        printer->notifyNoMoreModels();
        printNoMoreModels(clog, getOptions().verbosity);
        break;
    }
    
    addModel(getSpace()->getEngine()->getModel());
  }
  printer->notifySolvingFinished();
}


/**
 * FindOptimalModels
 */

FindOptimalModels::FindOptimalModels(Space* space, ModelExpandOptions opts)
  : ModelExpand(space,opts), nbModels(opts.nbmodelstofind){
}

FindOptimalModels::~FindOptimalModels(){
}

void FindOptimalModels::execute(){
	printer->notifyStartSolving();
	if (getSpace()->isCertainlyUnsat()) {
		printer->notifySolvingFinished();
		return;
	}
	
	printer->notifyOptimizing();
	printSearchStart(clog, getOptions().verbosity);

	Disjunction lastInvalidationClause({});
	// find a first model
	auto state = getSolver().solve(true);
	if (state == l_Undef || terminateRequested()) {
		printer->notifySolvingAborted();
		return;
	}else if(state==l_False){
        printer->notifySolvingFinished();
		getSpace()->notifyUnsat();
		return;
	}else{ // model found :)
		addModel(getSpace()->getEngine()->getModel());
		getSolver().invalidate(lastInvalidationClause.literals);
	}
  
	// do the actual optimization
	while (getSolver().hasNextOptimum()) {
		auto optim = getSolver().getNextOptimum();
		bool optimumfound = false;
		while (!optimumfound) {
			// add stricter bound using assumptions
			Lit lastAssumption;
			switch (optim.optim) {
				case Optim::SUBSET:
				{
					throw idpexception("Invalid code path: subset minisation not yet supported.");
					break;
				}
				case Optim::AGG:
				{
					lastAssumption = invalidateAgg(optim);
					optimumfound = lastAssumption == Minisat::lit_Undef;
					break;
				}
				case Optim::VAR:
				{
					lastAssumption = invalidateVar(optim);
					optimumfound = lastAssumption == Minisat::lit_Undef;
					break;
				}
			}
			if (optimumfound) {
				break;
			}

			getSolver().addAssumption(lastAssumption);
			// NOTE: since we added an assumption literal, the invalidation will never propagate to false.
			// look for new model:  
			state = getSolver().solve(true);
			if (state == l_Undef || terminateRequested()) {
				printer->notifySolvingAborted();
				return;
			} else if (state == l_False) {
				optimumfound = true;
				getSolver().removeAssumption(lastAssumption);
				getSolver().addAssumption(~lastAssumption); // TODO add unit clauses instead...
				// Fix the optimization constraints to their optimal condition
				switch (optim.optim) {
					case Optim::SUBSET:
					{
						throw idpexception("Invalid code path: subset minimisation not yet supported.");
						break;
					}
					case Optim::AGG:
					{
						auto agg = optim.agg_to_minimize;
						space->add(Aggregate(~lastAssumption, agg->getSet()->getSetID(), _solutions->getBestValueFound(), agg->getType(), agg->getSign(), AggSem::COMP, -1, false));
						break;
					}
					case Optim::VAR:
					{
						internalAdd(Disjunction({optim.var->getEQLit(_solutions->getBestValueFound())}), getSolver().getBaseTheoryID(), getSolver());
						break;
					}
				}
			} else {
				addModel(getSpace()->getEngine()->getModel());
				lastInvalidationClause.literals.clear();
				getSolver().invalidate(lastInvalidationClause.literals);
			}
		}
	}

	// optimality reached
	_solutions->notifyOptimalModelFound(); // this keep the last added model
	getSolver().getOutOfUnsat();

	// now, to find as many models as needed:
	while (nbModels == 0 || nbModels > _solutions->getNbModelsFound()) {
		invalidateModel(lastInvalidationClause);

		state = getSolver().solve(true);
		if (state == l_Undef || terminateRequested()) {
			printer->notifySolvingAborted();
			return;
		} else if (state == l_False) {
			printer->notifyNoMoreModels();
			printNoMoreModels(clog, getOptions().verbosity);
			break;
		}
		addModel(getSpace()->getEngine()->getModel());
		lastInvalidationClause.literals.clear();
		getSolver().invalidate(lastInvalidationClause.literals);
	}
	printer->notifySolvingFinished();
}

/*
 * ModelExpand
 */
ModelExpand::ModelExpand(Space* space, ModelExpandOptions options)
		: SpaceTask(space), _options(options), _solutions(new ModelManager(options.savemodels)),
			printer(new Printer(_solutions, space, options.printmodels, space->getOptions())) {
}

ModelExpand::~ModelExpand() {
	delete (_solutions);
	delete (printer);
}

MXStatistics ModelExpand::getStats() const{
	return getSpace()->getStats();
}

int ModelExpand::getNbModelsFound() const {
	if (getSpace()->isOptimizationProblem() && not _solutions->hasOptimalModel()) {
		return 0;
	}
	return _solutions->getNbModelsFound();
}

const modellist& ModelExpand::getSolutions() const {
	return _solutions->getModels();
}
modellist ModelExpand::getBestSolutionsFound() const {
	if(not getSpace()->isOptimizationProblem()){
		throw idpexception("Cannot return best models when not doing optimization inference.");
	}
	return _solutions->getBestModelsFound();
}

Weight ModelExpand::getBestValueFound() const{
	if(not getSpace()->isOptimizationProblem()){
		throw idpexception("Cannot return best models when not doing optimization inference.");
	}
	return _solutions->getBestValueFound();
}

bool ModelExpand::isSat() const {
	return _solutions->isSat();
}
bool ModelExpand::isUnsat() const {
	return _solutions->isUnsat();
}

litlist ModelExpand::getUnsatExplanation() const {
	vector<Lit> outmodel;
	for (auto lit : getSolver().getUnsatExplanation()) {
		if (getSpace()->getRemapper()->wasInput(lit)) {
			outmodel.push_back(getSpace()->getRemapper()->getLiteral(lit));
		}
	}
	return outmodel;
}

void ModelExpand::notifySolvingAborted() {
	printer->notifySolvingAborted();
}

void ModelExpand::addModel(std::shared_ptr<Model> model) {
	addModelToSolution(model, *getSpace()->getRemapper(), *_solutions, *printer);
}

/**
 * Returns false if invalidating the model leads to UNSAT, meaning that no more models are possible. Otherwise true.
 */
SATVAL ModelExpand::invalidateModel(Disjunction& clause) {
	if (getOptions().verbosity >= 1) {
		clog << "Adding model-invalidating clause: [ ";
		clog << getSpace()->toString(clause.literals);
		clog << "]\n";
	}
  internalAdd(clause, getSolver().getBaseTheoryID(), getSolver());
	return getSolver().satState();
}

// OPTIMIZATION METHODS

Lit ModelExpand::invalidateAgg(OptimStatement& optim) {  
  // general idea: add a new -conditional- agg constraint over the same weighted set as the optimization criterion
  // problem: the weighted set used in the optimization criterion is normalized, and cannot be used to construct the agg constraint
  // solution: get the originally parsed weighted set (has the same id (which is a bad smell)).
  auto agg = optim.agg_to_minimize;
	auto s = agg->getSet();
  // Getting the original weighted set. Ugly line of code ahead:
  WLSet* wlset = space->getEngine()->getFactory(getSolver().getBaseTheoryID()).getParsedSet(s->getSetID());
  // calculating current optimization value
  Weight bestvalue = 0;
  for(auto wl: wlset->wl){
    if(getSolver().getModelValue(wl.getLit())==l_True){
      bestvalue += wl.getWeight();
    }
  }
  _solutions->setLatestOptimum(bestvalue);
	printer->notifyCurrentOptimum(bestvalue);
  
	if (getOptions().verbosity >= 1) {
		clog << "> Current optimal value " << bestvalue << " after " << (long long)((double)clock()*1000/(CLOCKS_PER_SEC)) << " ms\n";
	}
  
  Lit assumption = mkPosLit(getSolver().newAtom());  
  // adding new bound:
  space->add(Aggregate(assumption, agg->getSet()->getSetID(), bestvalue-1, agg->getType(), agg->getSign(), AggSem::COMP, -1, false));
  return assumption;
}

Lit ModelExpand::invalidateVar(OptimStatement& optim) {
	auto var = optim.var;
	auto bestvalue = var->maxValue();
	_solutions->setLatestOptimum(bestvalue);
	printer->notifyCurrentOptimum(bestvalue);
	if (getOptions().verbosity >= 1) {
		clog << "> Current optimal value " << bestvalue << " after " << (long long)((double)clock()*1000/(CLOCKS_PER_SEC)) << " ms\n";
	}

	if(optim.minimize){
		if (var->origMinValue() == bestvalue) {
            internalAdd(Disjunction({var->getEQLit(bestvalue)}), getSolver().getBaseTheoryID(), getSolver());
			return Minisat::lit_Undef;
		}else{
            return var->getLEQLit(bestvalue - 1);
        }
	}else{
		if (var->origMaxValue() == bestvalue) {
            internalAdd(Disjunction({var->getEQLit(bestvalue)}), getSolver().getBaseTheoryID(), getSolver());
			return Minisat::lit_Undef;
		}else{
            return var->getGEQLit(bestvalue + 1);
        }
	}
}

/*
 * UnitPropagate
 */
literallist UnitPropagate::getEntailedLiterals() const {
	auto lits = getSolver().getEntailedLiterals();
	literallist literals;
	auto r = *getSpace()->getRemapper();
	for (auto i = lits.cbegin(); i < lits.cend(); ++i) {
		if (getSolver().rootValue(*i) != l_Undef && r.wasInput(*i)) {
			literals.push_back(r.getLiteral(*i));
		}
	}
	return literals;
}

void UnitPropagate::execute() {
	getSolver().addAssumptions(assumptions);
	getSolver().solve(false);
}

void UnitPropagate::writeOutEntailedLiterals() {
	auto resman = createResMan(getOptions().outputfile);
	ostream output(resman->getBuffer());

	clog << ">>> Following is a list of literals entailed by the theory.\n";
	const auto& lits = getEntailedLiterals();
	bool begin = true;
	for (auto i = lits.cbegin(); i < lits.cend(); ++i) {
		if (not begin) {
			output << " ";
		}
		begin = false;
		output << getSpace()->getTranslator()->toString(*i);
	}
	output << "\n";
	resman->close();
}
/*
 * Transform
 */
void Transform::execute() {
	std::shared_ptr<ResMan> resfile(createResMan(getSpace()->getOptions().outputfile));
	ostream output(resfile->getBuffer());
	switch (outputlanguage) {
	case TheoryPrinting::FZ: {
		FlatZincRewriter<ostream> fzrw(getSpace()->getRemapper(), getSpace()->getTranslator(), getOptions(), output);
		getSolver().accept(fzrw);
		break;
	}
	case TheoryPrinting::ECNF: {
		RealECNFPrinter<ostream> pr(getSpace()->getEngine(), output);
		getSolver().accept(pr);
		break;
	}
	case TheoryPrinting::CNF: {
		RealCNFPrinter<ostream> pr(getSpace()->getEngine(), output);
		getSolver().accept(pr);
		break;
	}
	case TheoryPrinting::ECNFGRAPH: {
		ECNFGraphPrinter<ostream> pr(getSpace()->getEngine(), output);
		getSolver().accept(pr);
		break;
	}
	case TheoryPrinting::HUMAN: {
		HumanReadableParsingPrinter<ostream> pr(getSpace()->getEngine(), output);
		getSolver().accept(pr);
		break;
	}
	}
}
