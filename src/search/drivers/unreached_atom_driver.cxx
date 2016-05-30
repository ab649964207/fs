
#include <search/drivers/unreached_atom_driver.hxx>
#include <problem.hxx>
#include <problem_info.hxx>
#include <state.hxx>
#include <aptk2/search/algorithms/best_first_search.hxx>
#include <heuristics/relaxed_plan/unreached_atom_rpg.hxx>
#include <constraints/gecode/handlers/ground_effect_csp.hxx>
#include <actions/ground_action_iterator.hxx>
#include <actions/grounding.hxx>
#include <utils/support.hxx>

using namespace fs0::gecode;

namespace fs0 { namespace drivers {

std::unique_ptr<FSGroundSearchAlgorithm> UnreachedAtomDriver::create(const Config& config, const GroundStateModel& model) const {
	LPT_INFO("main", "Using the lifted-effect base RPG constructor");
	const Problem& problem = model.getTask();
	bool novelty = config.useNoveltyConstraint() && !problem.is_predicative();
	bool approximate = config.useApproximateActionResolution();
	bool delayed = config.useDelayedEvaluation();

	
	const auto& tuple_index = problem.get_tuple_index();
	const std::vector<const GroundAction*>& actions = problem.getGroundActions();
	
	const auto managed = support::compute_managed_symbols(std::vector<const ActionBase*>(actions.begin(), actions.end()), problem.getGoalConditions(), problem.getStateConstraints());
	ExtensionHandler extension_handler(problem.get_tuple_index(), managed);
	
	UnreachedAtomRPG heuristic(problem, problem.getGoalConditions(), problem.getStateConstraints(),
									GroundEffectCSP::create(actions, tuple_index, approximate, novelty),
									extension_handler);
	
	return std::unique_ptr<FSGroundSearchAlgorithm>(new aptk::StlBestFirstSearch<SearchNode, UnreachedAtomRPG, GroundStateModel>(model, std::move(heuristic), delayed));
}

GroundStateModel UnreachedAtomDriver::setup(const Config& config, Problem& problem) const {
	// We ground all actions
	problem.setGroundActions(ActionGrounder::fully_ground(problem.getActionData(), ProblemInfo::getInstance()));
	return GroundStateModel(problem);
}

} } // namespaces
