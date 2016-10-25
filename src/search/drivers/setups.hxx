
#pragma once

#include <lifted_state_model.hxx>
#include <ground_state_model.hxx>
#include <search/algorithms/aptk/events.hxx>
#include <search/events.hxx>

namespace fs0 { class Problem; }

namespace fs0 { namespace drivers {

//! A catalog of common setups for grounding actions for both search and heuristic computations.
class GroundingSetup {
public:
	static LiftedStateModel fully_lifted_model(Problem& problem);
	
	//! A simple model with all grounded actions
	static GroundStateModel fully_ground_model(Problem& problem);
	
	//! We'll use all the ground actions for the search plus the partially ground actions for the heuristic computations
	static GroundStateModel ground_search_lifted_heuristic(Problem& problem);

};

class EventUtils {
public:
	using HandlerPtr = std::unique_ptr<lapkt::events::EventHandler>;
	
	template <typename NodeT, typename HeuristicT>
	static void setup_evaluation_observer(const Config& config, HeuristicT& heuristic, SearchStats& stats, std::vector<HandlerPtr>& handlers) {
		using EvaluatorT = EvaluationObserver<NodeT, HeuristicT>;
		handlers.push_back(std::unique_ptr<EvaluatorT>(new EvaluatorT(heuristic, config.getNodeEvaluationType(), stats)));
	}
	
	template <typename NodeT>
	static void setup_stats_observer(SearchStats& stats, std::vector<HandlerPtr>& handlers) {
		using StatsT = StatsObserver<NodeT>;
		handlers.push_back(std::unique_ptr<StatsT>(new StatsT(stats)));
	}
	
	template <typename NodeT>
	static void setup_HA_observer(std::vector<HandlerPtr>& handlers) {
		using HAObserverT = HelpfulObserver<NodeT>;
		handlers.push_back(std::unique_ptr<HAObserverT>(new HAObserverT()));
	}
	
};

} } // namespaces