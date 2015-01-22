

#include <constraints/manager.hxx>
#include <utils/utils.hxx>

namespace fs0 {

//! Note that we use both types of constraints as goal constraints
ConstraintManager::ConstraintManager(const ScopedConstraint::vcptr& goalConstraints, const ScopedConstraint::vcptr& stateConstraints)
	: sconstraints(stateConstraints), gconstraints(Utils::merge(goalConstraints, stateConstraints))
{
	initialize();
}

//! Precompute some of the structures that we'll need later on.
void ConstraintManager::initialize() {
	// Index the different constraints by arity
	indexConstraintsByArity(sconstraints, s_unary_contraints, s_binary_contraints, s_n_ary_contraints);
	indexConstraintsByArity(gconstraints, g_unary_contraints, g_binary_contraints, g_n_ary_contraints);
	
	// Initialize the worklists for both the state and goal constraints.
	initializeAC3Worklist(s_binary_contraints, SCWorklist);
	initializeAC3Worklist(g_binary_contraints, GCWorklist);
	
	s_relevant = indexRelevantVariables(sconstraints);
	g_relevant = indexRelevantVariables(gconstraints);
}

const VariableIdxVector& ConstraintManager::getStateConstraintRelevantVariables() const { return s_relevant; }
const VariableIdxVector& ConstraintManager::getGoalConstraintRelevantVariables() const { return g_relevant; }

//! Indexes pointers to the constraints in three different vectors: unary, binary and n-ary constraints.
void ConstraintManager::indexConstraintsByArity(const ScopedConstraint::vcptr& constraints,
								PConstraintPtrVct& unary,
								PConstraintPtrVct& binary,
								PConstraintPtrVct& n_ary
							) {
	for (const ScopedConstraint::cptr ctr:constraints) {
		ScopedConstraint::Filtering filtering = ctr->filteringType();
		if (filtering == ScopedConstraint::Filtering::Unary) {
			unary.push_back(ctr);
		} else if (filtering == ScopedConstraint::Filtering::ArcReduction) {
			binary.push_back(ctr);
		} else {
			n_ary.push_back(ctr);
		}
	}
}

//! Initializes a worklist. `constraints` is expected to have only binary constraints.
void ConstraintManager::initializeAC3Worklist(const PConstraintPtrVct& constraints, ArcSet& worklist) {
	for (ScopedConstraint::cptr ctr:constraints) {
		assert(ctr->getArity() == 2);
		worklist.insert(std::make_pair(ctr, 0));
		worklist.insert(std::make_pair(ctr, 1));
	}
}

ScopedConstraint::Output ConstraintManager::unaryFiltering(const DomainMap& domains, const PConstraintPtrVct& constraints) const {
	ScopedConstraint::Output output = ScopedConstraint::Output::Unpruned;
	
	for (ScopedConstraint::cptr ctr:constraints) {
		assert(ctr->getArity() == 1);
		ScopedConstraint::Output o = ctr->filter(domains);
		if (o == ScopedConstraint::Output::Pruned) {
			output = ScopedConstraint::Output::Pruned;
		} else if (o == ScopedConstraint::Output::Failure) {
			return o;  // Early termination
		}
	}
	return output;
}

ScopedConstraint::Output ConstraintManager::filter(const DomainMap& domains, 
						const PConstraintPtrVct& unary, 
						const PConstraintPtrVct& binary,
						const PConstraintPtrVct& n_ary,
						const ArcSet& AC3Worklist
) const {
	ScopedConstraint::Output result = unaryFiltering(domains, unary);
	if (result == ScopedConstraint::Output::Failure) return result;

	ArcSet worklist(AC3Worklist);  // Copy the state constraint worklist
	// printArcSet(worklist);
	
	// Pre-load the non-unary constraints
	loadConstraintDomains(domains, binary);
	loadConstraintDomains(domains, n_ary);
	
	// First apply both types of filtering
	ScopedConstraint::Output b_result = filter_binary_constraints(binary, worklist);
	if (b_result == ScopedConstraint::Output::Failure) {
		// Empty the non-unary constraints
		emptyConstraintDomains(binary);
		emptyConstraintDomains(n_ary);
		return b_result;
	}
	
	ScopedConstraint::Output g_result = filter_global_constraints(n_ary);
	if (g_result == ScopedConstraint::Output::Failure) {
		// Empty the non-unary constraints
		emptyConstraintDomains(binary);
		emptyConstraintDomains(n_ary);
		return g_result;
	}
	
	// The global result won't be affected: if it was "Pruned", it'll continue to be prune regardless of what happens inside the loop.
	if (b_result == ScopedConstraint::Output::Pruned || g_result == ScopedConstraint::Output::Pruned) result = ScopedConstraint::Output::Pruned;
		
	// Keep pruning until we reach a fixpoint.
	while (b_result == ScopedConstraint::Output::Pruned && g_result == ScopedConstraint::Output::Pruned) {
		// Each type of pruning (global or binary) needs only be performed
		// if the other type of pruning actually modified some domain.
		b_result = filter_binary_constraints(binary, worklist);
		if (b_result == ScopedConstraint::Output::Pruned) g_result = filter_global_constraints(n_ary);
	}

	// Empty the non-unary constraints
	emptyConstraintDomains(binary);
	emptyConstraintDomains(n_ary);
	
	return result;
}

ScopedConstraint::Output ConstraintManager::filterWithStateConstraints(const DomainMap& domains) const {
	if (sconstraints.empty()) return ScopedConstraint::Output::Unpruned;
 	// std::cout << "Num. binary constraints: " << s_binary_contraints.size() << std::endl;
 	// std::cout << "Num. unary constraints: " << s_unary_contraints.size() << std::endl;
 	// std::cout << "Num. n-ary constraints: " << s_n_ary_contraints.size() << std::endl;
 	// std::cout << "Worklist size: " << SCWorklist.size() << std::endl;
	return filter(domains, s_unary_contraints, s_binary_contraints, s_n_ary_contraints, SCWorklist);
}

//! Prunes the domains of the given state
ScopedConstraint::Output ConstraintManager::filterWithGoalConstraints(const DomainMap& domains) const {
	if (gconstraints.empty()) return ScopedConstraint::Output::Unpruned;
	return filter(domains, g_unary_contraints, g_binary_contraints, g_n_ary_contraints, GCWorklist);
}

void ConstraintManager::emptyConstraintDomains(const PConstraintPtrVct& constraints) const {
	for (ScopedConstraint::cptr constraint:constraints) {
		constraint->emptyDomains();
	}
}

void ConstraintManager::loadConstraintDomains(const DomainMap& domains, const PConstraintPtrVct& constraints) const {
	for (ScopedConstraint::cptr constraint:constraints) {
		constraint->loadDomains(domains);
	}
}

ScopedConstraint::Output ConstraintManager::filter_global_constraints(const PConstraintPtrVct& constraints) const {
	ScopedConstraint::Output output = ScopedConstraint::Output::Unpruned;
	for (ScopedConstraint::cptr constraint:constraints) {
		ScopedConstraint::Output o = constraint->filter();
		if (o == ScopedConstraint::Output::Failure) return o;
		else if (o == ScopedConstraint::Output::Pruned) output = o;
	}
	return output;
}


//! AC3 filtering
ScopedConstraint::Output ConstraintManager::filter_binary_constraints(const PConstraintPtrVct& constraints, ArcSet& worklist) const {
	
	ScopedConstraint::Output result = ScopedConstraint::Output::Unpruned;
	
	// 1. Analyse pending arcs until the worklist is empty
	while (!worklist.empty()) {
		Arc a = select(worklist);
		ScopedConstraint::cptr constraint = a.first;
		unsigned variable = a.second;  // The index 0 or 1 of the relevant variable.
		assert(variable == 0 || variable == 1);

		// 2. Arc-reduce the constraint with respect to the variable `variable`
		ScopedConstraint::Output o = constraint->filter(variable);
		if (o == ScopedConstraint::Output::Failure) return o;
		

		// 3. If we have removed some element from the domain, we insert the related constraints into the worklist in order to reconsider them again.
		if (o == ScopedConstraint::Output::Pruned) {
			result = ScopedConstraint::Output::Pruned;
			VariableIdx pruned = constraint->getScope()[variable];  // This is the index of the state variable whose domain we have pruned
			for (ScopedConstraint::cptr ctr:constraints) {
				if (ctr == constraint) continue;  // No need to reinsert the same constraint we just used.
				
				// Only if the constraint has overlapping scope, we insert in the worklist the constraint paired with _the other_ variable, to be analysed later.
				const VariableIdxVector& scope = ctr->getScope();
				assert(scope.size() == 2);
				
				if (pruned == scope[0]) worklist.insert(std::make_pair(ctr, 1));
				else if (pruned == scope[1]) worklist.insert(std::make_pair(ctr, 0));
				else continue;
			}
		}
	}
	
	return result;
}

bool ConstraintManager::checkGoalConstraintsSatisfied(const State& s) const {
	for (ScopedConstraint::cptr constraint:gconstraints) {
		if (!constraint->isSatisfied(s)) return false;
	}
	return true;
}

bool ConstraintManager::checkConsistency(const DomainMap& domains) const {
	for (const auto& domain:domains) {
		if (domain.second->size() == 0) return false; // If any pruned domain is empty, the CSP has no solution.
	}
	return true;
}


//! We select an arbitrary arc, indeed the first according to the order between pairs of procedure IDs and variable IDs.
//! and remove it from the worklist
ConstraintManager::Arc ConstraintManager::select(ArcSet& worklist) const {
	assert(!worklist.empty());
	auto it = worklist.end();
	--it;
	auto elem = *(it);
	worklist.erase(it);
	return elem;
}
VariableIdxVector ConstraintManager::indexRelevantVariables(PConstraintPtrVct& constraints) {
	boost::container::flat_set<VariableIdx> relevant;
	for (const ScopedConstraint::cptr constraint:constraints) {
		for (VariableIdx variable:constraint->getScope()) {
			relevant.insert(variable);
		}
	}
	return VariableIdxVector(relevant.begin(), relevant.end());
}


} // namespaces
