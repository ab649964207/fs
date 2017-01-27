
#include <memory>
#include <utils/lexical_cast.hxx>

#include <problem.hxx>
#include <utils/loader.hxx>
#include <actions/actions.hxx>
#include <actions/grounding.hxx>
#include <utils/component_factory.hxx>
#include <languages/fstrips/loader.hxx>
#include <aptk2/tools/logging.hxx>
#include <constraints/gecode/helper.hxx>
#include <constraints/registry.hxx>
#include <utils/printers/registry.hxx>
#include <utils/config.hxx>
#include <utils/static.hxx>
#include <state.hxx>
#include <problem_info.hxx>
#include <languages/fstrips/formulae.hxx>
#include <validator.hxx>


namespace fs = fs0::language::fstrips;

namespace fs0 {

Problem* Loader::loadProblem(const rapidjson::Document& data) {
	const Config& config = Config::instance();
	const ProblemInfo& info = ProblemInfo::getInstance();
	
	LPT_INFO("main", "Loading initial state...");
	auto init = loadState(data["init"]);
	
	LPT_INFO("main", "Loading action data...");
	auto action_data = loadAllActionData(data["action_schemata"], info);
	
	LPT_INFO("main", "Loading goal formula...");
	auto goal = loadGroundedFormula(data["goal"], info);
	
	LPT_INFO("main", "Loading state constraints...");
	auto sc = loadGroundedFormula(data["state_constraints"], info);
	
	//! Set the singleton global instance
	Problem* problem = new Problem(init, action_data, goal, sc, AtomIndex(info));
	Problem::setInstance(std::unique_ptr<Problem>(problem));
	
	LPT_INFO("components", "Bootstrapping problem with following external component repository\n" << print::logical_registry(LogicalComponentRegistry::instance()));

	if (config.validate()) {
		LPT_INFO("main", "Validating problem...");
		Validator::validate_problem(*problem, info);
	}
	
	return problem;
}

void 
Loader::loadFunctions(const BaseComponentFactory& factory, ProblemInfo& info) {
	
	// First load the extensions of the static symbols
	for (auto name:info.getSymbolNames()) {
		unsigned id = info.getSymbolId(name);
		if (info.getSymbolData(id).isStatic()) {
			info.set_extension(id, StaticExtension::load_static_extension(name, info));
		}
	}
	
	// Load the function objects for externally-defined symbols
	for (auto elem:factory.instantiateFunctions(info)) {
		info.setFunction(info.getSymbolId(elem.first), elem.second);
	}
}

ProblemInfo&
Loader::loadProblemInfo(const rapidjson::Document& data, const std::string& data_dir, const BaseComponentFactory& factory) {
	// Load and set the ProblemInfo data structure
	auto info = std::unique_ptr<ProblemInfo>(new ProblemInfo(data, data_dir));
	loadFunctions(factory, *info);
	return ProblemInfo::setInstance(std::move(info));
}

State*
Loader::loadState(const rapidjson::Value& data) {
	// The state is an array of two-sized arrays [x,v], representing atoms x=v
	unsigned numAtoms = data["variables"].GetInt();
	Atom::vctr facts;
	for (unsigned i = 0; i < data["atoms"].Size(); ++i) {
		const rapidjson::Value& node = data["atoms"][i];
		facts.push_back(Atom(node[0].GetInt(), node[1].GetInt()));
	}
	return new State(numAtoms, facts);
}


std::vector<const ActionData*>
Loader::loadAllActionData(const rapidjson::Value& data, const ProblemInfo& info) {
	std::vector<const ActionData*> schemata;
	for (unsigned i = 0; i < data.Size(); ++i) {
		if (const ActionData* adata = loadActionData(data[i], i, info)) {
			schemata.push_back(adata);
		}
	}
	return schemata;
}

const ActionData*
Loader::loadActionData(const rapidjson::Value& node, unsigned id, const ProblemInfo& info) {
	const std::string& name = node["name"].GetString();
	const Signature signature = parseNumberList<unsigned>(node["signature"]);
	const std::vector<std::string> parameters = parseStringList(node["parameters"]);
	
	const fs::Formula* precondition = fs::Loader::parseFormula(node["conditions"], info);
	const std::vector<const fs::ActionEffect*> effects = fs::Loader::parseEffectList(node["effects"], info);
	
	ActionData adata(id, name, signature, parameters, precondition, effects);
	if (adata.has_empty_parameter()) {
		LPT_INFO("cout", "Action schema \"" << adata.getName() << "\" discarded because of empty parameter type.");
		return nullptr;
	}
	
	// We perform a first binding on the action schema so that state variables, etc. get consolidated, but the parameters remain the same
	// This is possibly not optimal, since for some configurations we might be duplicating efforts, but ATM we are happy with it
	return ActionGrounder::process_action_data(adata, info);
}

std::vector<const GroundAction*>
Loader::loadGroundActionsIfAvailable(const ProblemInfo& info, const std::vector<const ActionData*>& action_data) {
	std::vector<const GroundAction*> grounded;
	if (action_data.empty()) return grounded;
	
	std::string filename = info.getDataDir() + "/groundings.data";
	std::ifstream is(filename);
	
    if (!is.good()) { // File groundings.data does not exist
		return grounded;
	}
	
	unsigned current_schema_groundings = 0;
	unsigned id = 0;
	unsigned schema_id = -1;
	const ActionData* current = action_data[0];
	std::string line;
	
	while (std::getline(is, line)) {
		if (line.length() > 0 && line[0] == '#') { // We switch to the next action schema
			
			++schema_id;
			if (schema_id >= action_data.size()) {
				throw std::runtime_error("The number of action schemas in the groundings file does not match that in the problem description");
			}
			
			if (schema_id > 0) {
				LPT_INFO("cout", "Action schema \"" << current->getName() << "\" results in " << current_schema_groundings << " grounded actions");
			}
			
			current = action_data[schema_id];
			current_schema_groundings = 0;
			continue;
		}
		
		std::vector<ObjectIdx> deserialized = Serializer::deserializeLine(line, ",");
		if (current->getSignature().size() != deserialized.size()) {
			throw std::runtime_error("Wrong number of action parameters");
		}
		
		
		if (deserialized.empty()) {
			LPT_INFO("cout", "Grounding action schema '" << current->getName() << "' with no binding");
			id = ActionGrounder::ground(id, current, Binding::EMPTY_BINDING, info, grounded);
		} else {
			Binding binding(std::move(deserialized));
			id = ActionGrounder::ground(id, current, binding, info, grounded);
		}
		++current_schema_groundings;
	}
	
	LPT_INFO("cout", "Action schema \"" << current->getName() << "\" results in " << current_schema_groundings << " grounded actions");
	LPT_INFO("cout", "Grounding process stats:\t" << grounded.size() << " grounded actions");
	return grounded;
}

const fs::Formula*
Loader::loadGroundedFormula(const rapidjson::Value& data, const ProblemInfo& info) {
	const fs::Formula* unprocessed = fs::Loader::parseFormula(data["conditions"], info);
	// The conditions are by definition already grounded, and hence we need no binding, but we process the formula anyway
	// to detect tautologies, contradictions, etc., and to consolidate state variables
	auto processed = unprocessed->bind(Binding::EMPTY_BINDING, info);
	delete unprocessed;
	return processed;
}

rapidjson::Document
Loader::loadJSONObject(const std::string& filename) {
	// Load and parse the JSON data file.
	std::ifstream in(filename);
	if (in.fail()) throw std::runtime_error("Could not open filename '" + filename + "'");
	std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	rapidjson::Document data;
	data.Parse(str.c_str());
	return data;
}


template<typename T>
std::vector<T>
Loader::parseNumberList(const rapidjson::Value& data) {
	std::vector<T> output;
	for (unsigned i = 0; i < data.Size(); ++i) {
		output.push_back(boost::lexical_cast<T>(data[i].GetInt()));
	}
	return output;
}

std::vector<std::string>
Loader::parseStringList(const rapidjson::Value& data) {
	std::vector<std::string> output;
	for (unsigned i = 0; i < data.Size(); ++i) {
		output.push_back(data[i].GetString());
	}
	return output;
}


template<typename T>
std::vector<std::vector<T>> Loader::parseDoubleNumberList(const rapidjson::Value& data) {
	std::vector<std::vector<T>> output;
	assert(data.IsArray());
	if (data.Size() == 0) {
		output.push_back(std::vector<T>());
	} else {
		for (unsigned i = 0; i < data.Size(); ++i) {
			output.push_back(parseNumberList<T>(data[i]));
		}
	}
	return output;
}

} // namespaces
