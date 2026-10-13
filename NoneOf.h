#pragma once
#include <string>
#include <vector>
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

AlgorithmSet makeAlgorithms(Predicate predicate);