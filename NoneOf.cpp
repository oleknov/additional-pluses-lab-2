#include "NoneOf.h"
#include <algorithm>
#include <execution>
#include <thread>

using namespace std;

LibraryNoneOf::LibraryNoneOf(Predicate predicate) : _predicate(predicate) {}

bool LibraryNoneOf::operator()(const vector<dataT>& data) const
{
	return none_of(data.begin(), data.end(), _predicate);
}

template <typename Policy>
NamedAlgorithm makeWithPolicy(const string& name, Policy policy, Predicate predicate)
{
	return { name, [policy, predicate](const vector<dataT>& data) -> bool {
		return none_of(policy, data.begin(), data.end(), predicate);
		} };
}

AlgorithmSet makeAlgorithms(Predicate predicate)
{
	AlgorithmSet algorithms;
	algorithms.noPolicy = { "none_of (no policy)", LibraryNoneOf(predicate) };

	algorithms.withPolicies = {
		makeWithPolicy("execution::seq", execution::seq, predicate),
		makeWithPolicy("execution::unseq", execution::unseq, predicate),
		makeWithPolicy("execution::par", execution::par, predicate),
		makeWithPolicy("execution::par_unseq", execution::par_unseq, predicate)
	};

	return algorithms;
}