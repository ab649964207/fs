
#pragma once

#include <string>
#include <vector>

#include <fs0_types.hxx>


namespace fs0 {

/**
  * A ProblemInfo instance holds all the relevant information about the problem, including the names and types of state variables, problem objects, etc.
  */
class ProblemInfo
{
public:
	typedef std::shared_ptr<ProblemInfo> ptr;
	typedef std::shared_ptr<const ProblemInfo> cptr;
	
	enum class ObjectType {INT, BOOL, OBJECT}; 

protected:
	//! A map from action index to action name
	std::vector<std::string> actionNames;
	
	//! A map from state variable index to action name
	std::vector<std::string> variableNames;
	
	//! A map from state variable index to the type of the state variable
	std::vector<ObjectType> variableTypes;
	std::vector<std::string> variableTypenames;
	
	//! A map from object index to object name
	std::vector<std::string> objectNames;
	std::map<std::string, ObjectIdx> objectIds;
	
	//! A map from typename to all of the object indexes of that type
	std::map<std::string, std::vector<ObjectIdx>> typeObjects;
public:

	ProblemInfo(const std::string& data_dir);
	~ProblemInfo() {}
	
	const std::string& getActionName(ActionIdx index) const;
	
	const std::string& getVariableName(VariableIdx index) const;
	
	const ObjectType getVariableType(VariableIdx index) const;
	
	unsigned getNumVariables() const;
	
	const std::string getObjectName(VariableIdx varIdx, ObjectIdx objIdx) const;
	const std::string getObjectName(const std::string& type, ObjectIdx objIdx) const;
	inline ObjectIdx getObjectId(const std::string& name) const { return objectIds.at(name); }
	
	inline const std::vector<ObjectIdx>& getObjectsOfType(const std::string& type) const { return typeObjects.at(type); }
	
	
	const std::string& getCustomObjectName(ObjectIdx objIdx) const;
	
	unsigned getNumObjects() const;

protected:
	
	//! Load the names of the (bound) actions from the specified file.
	void loadVariableIndex(const std::string& filename);
	
	ObjectType parseVariableType(const std::string& str) const;
	
	//! Load the names of the state variables from the specified file.
	void loadActionIndex(const std::string& filename);
	
	//! Load the names of the problem objects from the specified file.
	void loadObjectIndex(const std::string& filename);
	
	//! Load the map from variable types to possible objects.
	void loadTypeObjects(const std::string& filename);
};

	  
	  
	  
} // namespaces
