// MSVC (Microsoft Visual C++), C++23 (/std:c++latest), configuration Release
// Build without optimization: /Od   Build with maximum optimization: /O2
// Usage: additional-pluses-lab-2.exe [build label] 
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
		Predicate complexPredicate = [](dataT x) -> bool {
			long long v = x;
			for (int i = 1; i <= 20; ++i) {
				v = (v * i + x) % 1000003;
			}
			return v < 0;
			};

		const vector<pair<string, Predicate>> predicates = {
			{ "simple: x <= 0", isNonPositive },
			{ "complex: arithmetic loop", complexPredicate }
		};

		printer.printDescription(buildLabel);

		for (const auto& [name, predicate] : predicates)
		{
			cout << "\nPredicate " << name << endl;
			printer.printTitle("Predicate " + name);
			AlgorithmSet algorithms = makeAlgorithms(predicate);

			experiments.libraryWithoutPolicy(algorithms);
			experiments.libraryWithPolicies(algorithms);
			experiments.ownThreadedAlgorithm(algorithms);
		}

		cout << "\nDone. Tables written to " << outputPath << endl;
	}
	catch (const exception& e)
	{
		cout << "Error: " << e.what() << endl;
	}
	return 0;
}