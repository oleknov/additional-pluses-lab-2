#include <cmath>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include "funcs.h"
#include "NoneOf.h"

using namespace std;

int main(int argc, char* argv[])
{
	try
	{
		string buildLabel = (argc > 1) ? argv[1] : "not specified";
		const string outputPath = (argc > 1) ? "output_" + buildLabel + ".txt" : OUTPUT_FILE;

		if (!testFilesExist())
		{
			generateTestFiles();
		}

		ofstream outFile(outputPath, ios::out | ios::trunc);
		if (!outFile.is_open())
		{
			throw runtime_error("cannot open file for writing: " + outputPath);
		}

		AlgorithmTimer timer(cout);
		TablePrinter printer(outFile);
		Experiments experiments(timer, printer);

		Predicate isNonPositive = [](dataT x) -> bool { return x <= 0; };
		Predicate isLogGreaterThanX = [](dataT x) -> bool { return log(x) > x; };

		const vector<pair<string, Predicate>> predicates = {
			{ "simple: x <= 0", isNonPositive },
			{ "complex: ln(x) > x", isLogGreaterThanX }
		};

		printer.printDescription(buildLabel);

		for (const auto& [name, predicate] : predicates)
		{
			cout << "\nPredicate " << name << endl;
			printer.printTitle("Predicate " + name);
			AlgorithmSet algorithms = makeAlgorithms(predicate);

			experiments.libraryWithoutPolicy(algorithms);
			experiments.libraryWithPolicies(algorithms);
		}

		cout << "\nDone. Tables written to " << outputPath << endl;
	}
	catch (const exception& e)
	{
		cout << "Error: " << e.what() << endl;
	}
	return 0;
}