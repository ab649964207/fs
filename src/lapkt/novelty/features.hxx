
#pragma once

#include <vector>
#include <iostream>
#include <memory>


namespace lapkt { namespace novelty {

//! Features are (ATM) integer values
using FeatureValueT = int;

//! A feature valuation is an ordered set of feature values
using FeatureValuation = std::vector<FeatureValueT>;

//! Base interface. A single novelty feature basically takes a state of a given type and returns a feature value.
template <typename StateT>
class NoveltyFeature {
public:
	virtual ~NoveltyFeature() = default;
	virtual NoveltyFeature* clone() const = 0;
	
	//!
	virtual FeatureValueT evaluate(const StateT& s) const = 0;
	
	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const NoveltyFeature& o) { return o.print(os); }
	virtual std::ostream& print(std::ostream& os) const = 0;
};


//! An (ordered) set of novelty features
template <typename StateT>
class FeatureSet {
public:
	using FeatureT = std::unique_ptr<NoveltyFeature<StateT>>;
	
	FeatureSet() = default;
	~FeatureSet() = default;
	
	FeatureSet(const FeatureSet&) = delete;
	FeatureSet(FeatureSet&&) = default;
	FeatureSet& operator=(const FeatureSet&) = delete;
	FeatureSet& operator=(FeatureSet&&) = default;
	
	//!
	void add(FeatureT&& feature) {
		_features.push_back(std::move(feature));
	}
	
	//!
	FeatureValuation evaluate(const StateT& state) const {
	// 	LPT_INFO("novelty-evaluations", "Evaluating state " << state);
		FeatureValuation values;
		
		values.reserve(_features.size());
		for (const auto& feature:_features) {
			values.push_back(feature->evaluate(state));
			// LPT_INFO("novelty-evaluations", "\t" << _featureMap.feature(k) << ": " << values[k]);
		}

	// 	LPT_DEBUG("heuristic", "Feature evaluation: " << std::endl << print::feature_set(varnames, values));
		return values;
	}
	
	//! The number of features in the set
	unsigned size() const { return _features.size(); }
	
protected:
	//! The features in the set
	std::vector<FeatureT> _features;
};

} } // namespaces
