
#include <actions/checker.hxx>
#include <actions/actions.hxx>
#include <actions/action_id.hxx>
#include <problem.hxx>
#include <applicability/applicability_manager.hxx>
#include <applicability/formula_interpreter.hxx>
#include <state.hxx>

namespace fs0 {


bool Checker::check_correctness(const Problem& problem, const ActionPlan& plan, const State& s0) {
	const auto& actions = problem.getGroundActions();
	ApplicabilityManager manager(problem.getStateConstraints());
	
	// First we make sure that the whole plan is applicable
	State state(s0);
	for (unsigned idx:plan) {
		const GroundAction& action = *actions[idx];
// 		std::cout << "Action: " << action << std::endl;
		if (!manager.isApplicable(state, action)) {
// 			std::cout << "Action not applicable on state " << state << std::endl;
			return false;
		}
		state.accumulate(manager.computeEffects(state, action)); // Accumulate the newly-produced atoms
	}
	
	// Now check that the resulting state is indeed a goal
	return problem.getGoalSatManager().satisfied(state);
}

bool Checker::check_correctness(const Problem& problem, const std::vector<LiftedActionID>& plan, const State& s0) {
	ApplicabilityManager manager(problem.getStateConstraints());
	
	// First we make sure that the whole plan is applicable
	State state(s0);
	for (const LiftedActionID& action_id:plan) {
		std::unique_ptr<const GroundAction> action(action_id.generate());
		if (!manager.isApplicable(state, *action)) return false;
		state.accumulate(manager.computeEffects(state, *action)); // Accumulate the newly-produced atoms
	}
	
	// Now check that the resulting state is indeed a goal
	return problem.getGoalSatManager().satisfied(state);
}


} // namespaces
