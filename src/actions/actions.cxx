
#include <limits>
#include <sstream>

#include <actions/actions.hxx>
#include <problem.hxx>
#include <utils/printers/binding.hxx>
#include <utils/printers/actions.hxx>
#include <utils/utils.hxx>
#include <languages/fstrips/language.hxx>


namespace fs0 {
	
ActionData::ActionData(unsigned id, const std::string& name, const Signature& signature, const std::vector<std::string>& parameter_names,
					   const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects)
	: _id(id), _name(name), _signature(signature), _parameter_names(parameter_names), _precondition(precondition), _effects(effects)
{
	assert(parameter_names.size() == signature.size());
}

ActionData::~ActionData() {
	delete _precondition;
	for (const auto pointer:_effects) delete pointer;
}

std::ostream& ActionData::print(std::ostream& os) const { 
	os <<  print::action_data_name(*this);
	return os;
}

bool
ActionData::has_empty_parameter() const {
	const ProblemInfo& info = ProblemInfo::getInstance();
	for (TypeIdx type:_signature) {
		if (info.getTypeObjects(type).empty()) return true;
	}
	return false;
}

ActionBase::ActionBase(const ActionData& action_data, const Binding& binding, const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects) :
	_data(action_data), _binding(binding), _precondition(precondition), _effects(effects) {}

ActionBase::~ActionBase() {
	delete _precondition;
	for (const auto pointer:_effects) delete pointer;
}

ActionBase::ActionBase(const ActionBase& o) :
	_data(o._data), _binding(o._binding), _precondition(o._precondition->clone()), _effects(Utils::clone(o._effects))
{}

/*
void ActionBase::addPrecondition(const fs::Formula* precondition) {
	auto old = _precondition;
	_precondition = _precondition->conjunction(precondition);
	delete old;
}

void ActionBase::replaceTerm(const fs::Term* before, const fs::Term* after) {
	WORK_IN_PROGRESS;
}

void ActionBase::addParameter(fs::BoundVariable* parameter) {
	WORK_IN_PROGRESS;
}
*/

std::ostream& ActionBase::print(std::ostream& os) const {
	os << print::action_header(*this);
	return os;
}


PartiallyGroundedAction::PartiallyGroundedAction(const ActionData& action_data, const Binding& binding, const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects) :
	ActionBase(action_data, binding, precondition, effects)
{}


GroundAction::GroundAction(unsigned id, const ActionData& action_data, const Binding& binding, const fs::Formula* precondition, const std::vector<const fs::ActionEffect*>& effects) : 
	ActionBase(action_data, binding, precondition, effects), _id(id)
{}


const ActionIdx GroundAction::invalid_action_id = std::numeric_limits<unsigned int>::max();


} // namespaces
