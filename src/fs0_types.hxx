
#pragma once

#include <memory>

#include <vector>
#include <map>
#include <set>
#include <boost/container/flat_set.hpp>

#include <exception>

/**
 * Custom datatypes and typedefs.
 */
namespace fs0 {

	// A unique identifier for any of the domain actions.
	typedef unsigned ActionIdx;

	// A sequence of actions is a plan.
	typedef std::vector<ActionIdx> ActionPlan;

	// A unique identifier for any of the domain data types.
	typedef unsigned TypeIdx;

	// A unique identifier for any of the problem objects (might be an integer)
	typedef int ObjectIdx;

	//! An action signature is a list of (positional) parameters with a given type.
	typedef std::vector<TypeIdx> Signature;

	//! The index identifying a state variable.
	typedef unsigned VariableIdx;

	typedef std::vector<VariableIdx> VariableIdxVector;
	typedef std::set<VariableIdx> VariableIdxSet;

	typedef std::vector<ObjectIdx> ObjectIdxVector;


	//! A domain is a set of values (of a state variable)
 	typedef boost::container::flat_set<ObjectIdx> Domain;
	typedef std::shared_ptr<Domain> DomainPtr;

	//! A vector of domains.
	typedef std::vector<DomainPtr> DomainVector;

	//! A map mapping state variable indexes to their possible domains.
	typedef std::map<VariableIdx, DomainPtr> DomainMap;
	
	//! A map mapping a subset of state variables to possible values
	typedef std::map<VariableIdx, ObjectIdx> PartialAssignment;

	//! A function in the logical sense.
	typedef std::function<ObjectIdx(const ObjectIdxVector&)> Function;
	
	
	/**
	 * Custom exceptions
	 */
	class UnimplementedFeatureException : public std::runtime_error {
	public:
		UnimplementedFeatureException(const std::string& msg) : std::runtime_error(msg) {}
	};
	
	class InvalidConfiguration : public std::runtime_error {
	public:
		InvalidConfiguration(const std::string& msg) : std::runtime_error(msg) {}
	};

} // namespaces
