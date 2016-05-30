
#include <search/drivers/gbfs_novelty.hxx>
#include <aptk2/search/algorithms/breadth_first_search.hxx>
#include <aptk2/search/algorithms/best_first_search.hxx>
#include <actions/ground_action_iterator.hxx>

namespace fs0 { namespace drivers {
	
std::unique_ptr<FSGroundSearchAlgorithm> GBFSNoveltyDriver::create(const Config& config, const GroundStateModel& model) const {
	FSGroundSearchAlgorithm* engine = nullptr;
	
	unsigned max_novelty = config.getOption<int>("engine.max_novelty");
	bool delayed = config.useDelayedEvaluation();

	NoveltyFeaturesConfiguration feature_configuration(config);
	
	NoveltyHeuristic heuristic(model, max_novelty, feature_configuration);
	engine = new aptk::StlBestFirstSearch<SearchNode, NoveltyHeuristic, GroundStateModel>(model, std::move(heuristic), delayed);
	
	LPT_INFO("main", "Heuristic options:");
	LPT_INFO("main", "\tMax novelty: " << max_novelty);
	LPT_INFO("main", "\tFeatiue extaction: " << feature_configuration);
	
	return std::unique_ptr<FSGroundSearchAlgorithm>(engine);
}

} } // namespaces
