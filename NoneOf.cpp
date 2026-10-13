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

ThreadedNoneOf::ThreadedNoneOf(size_t parts, Predicate predicate)
	: _parts(parts), _predicate(predicate) {
}

bool ThreadedNoneOf::operator()(const vector<dataT>& data) const
{
	const auto chunks = splitIntoParts(data);
	vector<char> partResults(chunks.size(), 0);
	runInThreads(chunks, partResults);
	return all_of(partResults.begin(), partResults.end(), [](char noneInPart) -> bool { return noneInPart != 0; });
}

vector<span<const dataT>> ThreadedNoneOf::splitIntoParts(span<const dataT> data) const
{
	vector<span<const dataT>> chunks;
	chunks.reserve(_parts);
	const size_t base = data.size() / _parts;
	const size_t extra = data.size() % _parts;
	size_t offset = 0;
	for (size_t i = 0; i < _parts; ++i)
	{
		const size_t length = base + ((i < extra) ? 1 : 0);
		chunks.push_back(data.subspan(offset, length));
		offset += length;
	}
	return chunks;
}

void ThreadedNoneOf::runInThreads(const vector<span<const dataT>>& chunks, vector<char>& partResults) const
{
	vector<jthread> workers;
	workers.reserve(chunks.size());
	for (size_t i = 0; i < chunks.size(); ++i)
	{
		workers.emplace_back([this, &chunks, &partResults, i]() -> void {
			partResults[i] = none_of(chunks[i].begin(), chunks[i].end(), _predicate);
			});
	}
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

	algorithms.threaded = [predicate](size_t parts) -> NamedAlgorithm {
		return { "own, K=" + to_string(parts), ThreadedNoneOf(parts, predicate) };
		};

	return algorithms;
}