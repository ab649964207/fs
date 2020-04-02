
#pragma once

#include <fs/core/constraints/gecode/handlers/lifted_action_csp.hxx>
#include <gecode/int.hh>

namespace fs0 { class AtomIndex; }

namespace fs0::language::fstrips { class StateVariable; }
namespace fs = fs0::language::fstrips;

namespace fs0::gecode {

class RPGIndex;

//! A CSP modeling and solving the effect of an action effect on a certain RPG layer
class LiftedEffectCSP : public LiftedActionCSP {
public:
	//! Factory method
	static std::vector<std::unique_ptr<LiftedEffectCSP>> create_smart(const std::vector<const PartiallyGroundedAction*>& schemata, const AtomIndex& tuple_index, bool approximate, bool novelty);
	
	static void prune_unreachable(std::vector<std::unique_ptr<LiftedEffectCSP>>& managers, const RPGIndex& rpg);


	//! The only constructor
	LiftedEffectCSP(const PartiallyGroundedAction& action, const fs::ActionEffect* effect, const AtomIndex& tuple_index, bool approximate);
	~LiftedEffectCSP() override = default;
	LiftedEffectCSP(const LiftedEffectCSP&) = delete;
	LiftedEffectCSP(LiftedEffectCSP&&) = delete;
	LiftedEffectCSP& operator=(const LiftedEffectCSP&) = delete;
	LiftedEffectCSP& operator=(LiftedEffectCSP&&) = delete;	
	
	bool init(bool use_novelty_constraint) override;

	const fs::ActionEffect* get_effect() const;
	
	unsigned get_lhs_symbol() const { return _lhs_symbol; }
	
	void seek_novel_tuples(RPGIndex& rpg) const;
	
	AtomIdx get_achievable_tuple() const { return _achievable_tuple_idx; }
	
	//! Raises an exception if the given effect is not valid for this type of effect handler, i.e. because it has nested fluents on the effect head.
	static const fs::StateVariable* check_valid_effect(const fs::ActionEffect* effect);
	
	const fs::Formula* get_precondition() const override;

	void process(RPGIndex& graph);

protected:
	//! In an effect f(t) := w, '_lhs_symbol' is the index of symbol 'f'
	unsigned _lhs_symbol;
	
	//! The indexes (in the CSP) of the CSP variables that correspond to the tuple 't' in an effect LHS of the form f(t) := w.
	ValueTuple _lhs_subterms;
	
	//! In an effect f(t) := w, '_lhs_symbol' is the index of the CSP variable corresponding to the term 'w'.
	unsigned _rhs_variable;
	
	//! A list with all tuples that are relevant to the action effect. The first element of the pair
	//! is the index of the symbol, then come the indexes of the subterms (Indexes are CSP variable indexes).
	std::vector<std::pair<unsigned, std::vector<unsigned>>> _tuple_indexes;
	
	ValueTuple _effect_tuple;
	
	//! If the effect has a fixed achievable tuple  (e.g. because it is simple and has the form X := c), we store
	//! here the index of that tuple to optimize a number of processing aspects. Otherwise, _achievable_tuple_idx == INVALID_TUPLE.
	//! Note that all predicative (add-) effects without nested-fluent heads are simple enough to have a fixed achievable tuple,
	//! and in this type of effect processor we disallow nested-fluent heads.
	AtomIdx _achievable_tuple_idx;
	
	// Returns a tuple index if the current effect has a fixed achievable tuple, or INVALID_TUPLE otherwise.
	AtomIdx detect_achievable_tuple() const;
	
	void create_novelty_constraint() override;
	
	void post_novelty_constraint(FSGecodeSpace& csp, const RPGIndex& rpg) const override;

	void process_effect_solution(const FSGecodeSpace* solution, RPGIndex& rpg) const;
	
	//! Returns the novel tuple generated by the current effect in the given CSP solution
	AtomIdx compute_reached_tuple(const FSGecodeSpace* solution) const;

	static ValueTuple index_tuple_indexes(const fs::ActionEffect* effect);
	
	static unsigned index_lhs_symbol(const fs::ActionEffect* effect);
	
	void log() const override;
};


} // namespaces
