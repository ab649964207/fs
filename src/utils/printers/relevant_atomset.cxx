
#include <utils/printers/relevant_atomset.hxx>
#include <problem_info.hxx>
#include <search/drivers/sbfws/iw_run.hxx>

namespace fs0 { namespace print {

std::ostream& relevant_atomset::print(std::ostream& os) const {
	assert(_set._atomidx);
	const AtomIndex& atomidx = *_set._atomidx;
	
	os << "{";
	for (unsigned i = 0; i < _set._status.size(); ++i) {
		const Atom& atom = atomidx.to_atom(i);
		const auto& status = _set._status[i];
		if (status == bfws::RelevantAtomSet::STATUS::IRRELEVANT) continue;
		
		std::string mark = (status == bfws::RelevantAtomSet::STATUS::REACHED) ? "*" : "";
		os << atom << mark << ", ";
	}
	os << "}";
	
	return os;
}



} } // namespaces
