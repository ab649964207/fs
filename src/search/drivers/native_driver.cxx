
#include <search/drivers/native_driver.hxx>
#include <search/utils.hxx>
#include <problem.hxx>
#include <problem_info.hxx>
#include <state.hxx>
#include <actions/ground_action_iterator.hxx>
#include <actions/grounding.hxx>
#include <constraints/direct/direct_rpg_builder.hxx>
#include <constraints/direct/action_manager.hxx>
#include <languages/fstrips/formulae.hxx>
#include <search/drivers/setups.hxx>
#include <search/events.hxx>


namespace fs0 { namespace drivers {

NativeDriver::EnginePT
NativeDriver::create(const Config& config, const GroundStateModel& model, SearchStats& stats) {
	LPT_INFO("main", "Using the Native RPG Driver");
	const Problem& problem = model.getTask();
	const std::vector<const GroundAction*>& actions = problem.getGroundActions();

	if (!check_supported(problem)) {
		throw std::runtime_error("This problem is too complex for the \"native\" driver, try a different one.");
	}
	
	auto direct_builder = DirectRPGBuilder::create(problem.getGoalConditions(), problem.getStateConstraints());
	
	_heuristic = std::unique_ptr<HeuristicT>(new HeuristicT(problem, DirectActionManager::create(actions), std::move(direct_builder)));
	
	auto engine = EnginePT(new EngineT(model, *_heuristic));
	
	EventUtils::setup_stats_observer<NodeT>(stats, _handlers);
	EventUtils::setup_evaluation_observer<NodeT, DirectCRPG>(config, *_heuristic, _handlers);
	lapkt::events::subscribe(*engine, _handlers);
	
	return engine;
}

GroundStateModel
NativeDriver::setup(Problem& problem) const {
	return GroundingSetup::fully_ground_model(problem);
}

bool
NativeDriver::check_supported(const Problem& problem) {
	
	// Check that the actions are supported by the native CSP handlers
	for (const auto action:problem.getGroundActions()) {
		if (!DirectActionManager::is_supported(*action)) return false;
	}
	
	auto state_constraints = problem.getStateConstraints();
	auto goal = problem.getGoalConditions();
	
	// Now check that the goal is supported
	auto goal_conjunction = dynamic_cast<const fs::Conjunction*>(goal);
	
	// Goal formulas other than a conjunction are not supported
	if (!goal_conjunction) return false;
	
	// State constraints other than a conjunction are not supported
	if (!state_constraints->is_tautology() && !dynamic_cast<const fs::Conjunction*>(state_constraints)) return false;
	
	// Nested fluents in any of the formulas are not supported
	if (goal->nestedness() > 0 || state_constraints->nestedness() > 0) return false;
	
	return true;
}


void 
NativeDriver::search(Problem& problem, const Config& config, const std::string& out_dir, float start_time) {
	GroundStateModel model = setup(problem);
	SearchStats stats;
	auto engine = create(config, model, stats);
	Utils::do_search(*engine, model, out_dir, start_time, stats);
}

} } // namespaces
