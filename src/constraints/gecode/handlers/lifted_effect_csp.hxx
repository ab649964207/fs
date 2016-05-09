
#pragma once

#include <constraints/gecode/handlers/lifted_action_csp.hxx>
#include <gecode/int.hh>

namespace fs0 { class TupleIndex; }

namespace fs0 { namespace language { namespace fstrips { class StateVariable; } } }
namespace fs = fs0::language::fstrips;

namespace fs0 { namespace gecode {

class RPGIndex;

//! A CSP modeling and solving the effect of an action effect on a certain RPG layer
class LiftedEffectCSP : public LiftedActionCSP {
public:
	//! Factory method
	static std::vector<std::unique_ptr<LiftedEffectCSP>> create_smart(const std::vector<const PartiallyGroundedAction*>& schemata, const TupleIndex& tuple_index, bool approximate, bool novelty);

	//! The only constructor
	LiftedEffectCSP(const PartiallyGroundedAction& action, const fs::ActionEffect* effect, const TupleIndex& tuple_index, bool approximate);
	~LiftedEffectCSP();
	
	bool init(bool use_novelty_constraint) override;

	const fs::ActionEffect* get_effect() const;
	
	unsigned get_lhs_symbol() const { return _lhs_symbol; }
	
	void seek_novel_tuples(RPGIndex& rpg) const;
	
	TupleIdx get_achievable_tuple() const { return _achievable_tuple_idx; }
	
	//! Raises an exception if the given effect is not valid for this type of effect handler, i.e. because it has nested fluents on the effect head.
	static const fs::StateVariable* check_valid_effect(const fs::ActionEffect* effect);
	
	const std::vector<const fs::ActionEffect*>& get_effects() const override;

	const fs::Formula* get_precondition() const override;	

protected:
	//! This is the only effect managed by this CSP, which we store in a vector to comply with the parents' interfaces, which require
	//! to return a vector of effects. By construction, we have that _effects.size() == 1
	const std::vector<const fs::ActionEffect*> _effects;
	
	void process_effect_solution(const GecodeCSP* solution, RPGIndex& rpg) const;
	
	//! Returns the novel tuple generated by the current effect in the given CSP solution
	TupleIdx compute_reached_tuple(const GecodeCSP* solution) const;

	static ValueTuple index_tuple_indexes(const fs::ActionEffect* effect);
	
	void log() const;
	
	//! In an effect f(t) := w, '_lhs_symbol' is the index of symbol 'f'
	unsigned _lhs_symbol;
	
	//! The indexes (in the CSP) of the CSP variables that correspond to the tuple 't' in an effect LHS of the form f(t) := w.
	ValueTuple _lhs_subterms;
	
	//! In an effect f(t) := w, '_lhs_symbol' is the index of the CSP variable corresponding to the term 'w'.
	unsigned _rhs_variable;
	
	
	static unsigned index_lhs_symbol(const fs::ActionEffect* effect);
	
	//! A list with all tuples that are relevant to the action effect. The first element of the pair
	//! is the index of the symbol, then come the indexes of the subterms (Indexes are CSP variable indexes).
	std::vector<std::pair<unsigned, std::vector<unsigned>>> _tuple_indexes;
	
	ValueTuple _effect_tuple;
	
	//! If the effect has a fixed achievable tuple  (e.g. because it is simple and has the form X := c), we store
	//! here the index of that tuple to optimize a number of processing aspects. Otherwise, _achievable_tuple_idx == INVALID_TUPLE.
	//! Note that all predicative (add-) effects without nested-fluent heads are simple enough to have a fixed achievable tuple,
	//! and in this type of effect processor we disallow nested-fluent heads.
	TupleIdx _achievable_tuple_idx;
	
	// Returns a tuple index if the current effect has a fixed achievable tuple, or INVALID_TUPLE otherwise.
	TupleIdx detect_achievable_tuple() const;
	
	void create_novelty_constraint() override;
	
	void post_novelty_constraint(GecodeCSP& csp, const RPGIndex& rpg) const override;

};


} } // namespaces
