
#include <problem.hxx>
#include <search/drivers/registry.hxx>
#include <search/drivers/gbfs_constrained.hxx>
#include <search/drivers/iterated_width.hxx>
#include <search/drivers/breadth_first_search.hxx>
#include <search/drivers/gbfs_novelty.hxx>
// #include <search/drivers/asp_engine.hxx>
#include <search/drivers/unreached_atom_driver.hxx>
#include <search/drivers/smart_effect_driver.hxx>
#include <search/drivers/native_driver.hxx>
#include <heuristics/relaxed_plan/direct_crpg.hxx>
#include <heuristics/relaxed_plan/gecode_crpg.hxx>
#include <constraints/gecode/gecode_rpg_builder.hxx>
#include <actions/applicable_action_set.hxx>

using namespace fs0::gecode;

namespace fs0 { namespace drivers {

EngineRegistry& EngineRegistry::instance() {
	static EngineRegistry theInstance;
	return theInstance;
}

EngineRegistry::EngineRegistry() {
	// We register the pre-configured search drivers on the instantiation of the singleton
	add("gbfs_chff",  new GBFSConstrainedHeuristicsCreator<GecodeCRPG, DirectCRPG>());
	add("gbfs_chmax",  new GBFSConstrainedHeuristicsCreator<GecodeCHMax, DirectCHMax>());
	
	add("gbfs_native",  new NativeDriver());
	add("gbfs_unreached_atom",  new UnreachedAtomDriver());
	add("gbfs_smart",  new SmartEffectDriver());	
	
	add("iterated_width",  new IteratedWidthEngineCreator());
// 	add("asp_engine",  new ASPEngine());
	add("gbfs_novelty",  new GBFSNoveltyEngineCreator());
	add("breadth_first_search",  new BreadthFirstSearchEngineCreator());
	
}

EngineRegistry::~EngineRegistry() {
	for (const auto elem:_creators) delete elem.second;
}

void EngineRegistry::add(const std::string& engine_name, EngineCreator::cptr creator) {
auto res = _creators.insert(std::make_pair(engine_name, creator));
	if (!res.second) throw new std::runtime_error("Duplicate registration of engine creator for symbol " + engine_name);
}


EngineCreator::cptr EngineRegistry::get(const std::string& engine_name) const {
	auto it = _creators.find(engine_name);
	if (it == _creators.end()) throw std::runtime_error("No engine creator has been registered for given engine name '" + engine_name + "'");
	return it->second;
}


} } // namespaces
