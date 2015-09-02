
#pragma once

#include <fs0_types.hxx>
#include <vector>
#include <ostream>

namespace fs0 {
	
class Utils {
public:
	typedef std::vector<std::vector<unsigned>*> ValueSet;
	typedef std::vector<unsigned> Point;
	
	template <typename T>
	static std::vector<T> merge(const std::vector<T>& vector1, const std::vector<T>& vector2) {
		std::vector<T> result(vector1);
		result.insert(result.end(), vector2.begin(), vector2.end());
		return result;
	}
	
	//! Flattens a container of containers
	template <typename T>
	static std::vector<T> flatten(const std::vector<std::vector<T>>& vector) {
		std::vector<T> result;
		for(const std::vector<T>& sub:vector) {
			for(T var:sub) {
				result.push_back(var);
			}
		}
		return result;
	}
	
	//! "Uniquifies" the given vector.
	template <typename T>
	static std::vector<T> unique(const std::vector<T>& vector) {
		boost::container::flat_set<T> unique(vector.cbegin(), vector.cend());
		return std::vector<T>(unique.cbegin(), unique.cend());
	}
	
	//! Returns true iff the intersection of the first container (first1-last1) with the second container (first2-last2) is not empty.
	//! Complexity: O(min(n1, n2)), where n1 and n2 are the sizes of the first and second containers.
	template<class InputIt1, class InputIt2>
	static bool empty_intersection(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
		while (first1 != last1 && first2 != last2) {
			if (*first1 < *first2) {
				++first1;
			} else  {
				if (!(*first2 < *first1)) { // Pointers point to equal element - we can tell the intersection won't be empty
					return false;
				}
				++first2;
			}
		}
		return true;
	}
	
	//! Flips the elements of a one-to-one map.
	template <typename T1, typename T2>
	static std::map<T2, T1> flip_map(const std::map<T1, T2>& input) {
		std::map<T2, T1> output;
		for (const auto& elem:input) {
			assert(output.find(elem.second) == output.end());
			output[elem.second] = elem.first;
		}
		return output;
	}
};


} // namespaces
