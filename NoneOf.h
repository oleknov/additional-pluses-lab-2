#pragma once
#include <string>
#include <vector>
#include <span>
#include "funcs.h"

using Predicate = bool (*)(dataT);

class LibraryNoneOf
{
public:
	explicit LibraryNoneOf(Predicate predicate);
	bool operator()(const std::vector<dataT>& data) const;

private:
	Predicate _predicate;
};

// lengths of the parts differ by at most 1
class ThreadedNoneOf
{
public:
	ThreadedNoneOf(size_t parts, Predicate predicate);
	bool operator()(const std::vector<dataT>& data) const;

private:
	size_t _parts;
	Predicate _predicate;

	std::vector<std::span<const dataT>> splitIntoParts(std::span<const dataT> data) const;
	void runInThreads(const std::vector<std::span<const dataT>>& chunks, std::vector<char>& partResults) const;
};

AlgorithmSet makeAlgorithms(Predicate predicate);