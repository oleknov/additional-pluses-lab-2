#include "funcs.h"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>

using namespace std;

timeT measureTime(function<void()> operation)
{
	const auto start = chrono::steady_clock::now();
	operation();
	const auto end = chrono::steady_clock::now();
	return chrono::duration<timeT, TimeUnit>(end - start).count();
}

vector<dataT> randomSequence(size_t size, mt19937& gen)
{
	uniform_int_distribution<dataT> distribution(MIN_VALUE, MAX_VALUE);
	vector<dataT> ans(size);
	ranges::generate(ans, [&]() -> dataT { return distribution(gen); });
	return ans;
}

vector<dataT> readSequence(istream& s)
{
	size_t size = 0;
	s >> size;
	vector<dataT> ans(size);
	for (dataT& x : ans)
	{
		s >> x;
	}
	if (!s)
	{
		throw runtime_error("the sequence file is damaged or incomplete: delete the data directory and run again");
	}
	return ans;
}

void writeSequence(ostream& os, const vector<dataT>& v)
{
	os << v.size() << '\n';
	for (dataT x : v)
	{
		os << x << '\n';
	}
}

string testFilePath(size_t size)
{
	return DATA_DIRECTORY + "/seq_" + to_string(size) + ".txt";
}

bool testFilesExist()
{
	for (size_t size : SEQUENCE_SIZES)
	{
		if (!filesystem::exists(testFilePath(size)))
		{
			return false;
		}
	}
	return true;
}

void generateTestFiles()
{
	filesystem::create_directories(DATA_DIRECTORY);
	mt19937 gen(RANDOM_SEED);
	for (size_t size : SEQUENCE_SIZES)
	{
		ofstream fo(testFilePath(size));
		if (!fo.is_open())
		{
			throw runtime_error("cannot open file for writing: " + testFilePath(size));
		}
		writeSequence(fo, randomSequence(size, gen));
	}
}

vector<dataT> readTestFile(size_t size)
{
	ifstream fi(testFilePath(size));
	if (!fi.is_open())
	{
		throw runtime_error("cannot open file: " + testFilePath(size));
	}
	return readSequence(fi);
}

AlgorithmTimer::AlgorithmTimer(ostream& console) : _console(console) {}

Measurement AlgorithmTimer::measureMedian(size_t size, const NamedAlgorithm& algorithm) const
{
	vector<Measurement> runs;
	string resultLine;
	for (size_t i = 0; i < RUNS_PER_MEASUREMENT; ++i)
	{
		runs.push_back(timeOneRun(size, algorithm, resultLine));
	}
	_console << resultLine << endl;
	return medianOfRuns(runs);
}

Measurement AlgorithmTimer::timeOneRun(size_t size, const NamedAlgorithm& algorithm, string& resultLine) const
{
	vector<dataT> data;
	bool result = false;
	timeT computeTime = 0;
	const timeT totalTime = measureTime([&]() -> void {
		data = readTestFile(size);
		computeTime = measureTime([&]() -> void { result = algorithm.run(data); });
		resultLine = makeResultLine(size, algorithm.name, result);
		});
	return { totalTime, computeTime, result };
}

string AlgorithmTimer::makeResultLine(size_t size, const string& algorithm, bool result) const
{
	return format("{} | {} | none_of = {}", testFilePath(size), algorithm, result);
}

Measurement AlgorithmTimer::medianOfRuns(vector<Measurement> runs) const
{
	const size_t middle = runs.size() / 2;

	std::sort(runs.begin(), runs.end(), [](const Measurement& a, const Measurement& b) {
		return a.totalTime < b.totalTime;
		});
	const timeT totalTime = runs[middle].totalTime;

	std::sort(runs.begin(), runs.end(), [](const Measurement& a, const Measurement& b) {
		return a.computeTime < b.computeTime;
		});

	return { totalTime, runs[middle].computeTime, runs[middle].result };
}

TablePrinter::TablePrinter(ostream& out) : _out(out) {}

void TablePrinter::printDescription(const string& buildLabel) const
{
	_out << "What is measured (median of " << RUNS_PER_MEASUREMENT << " runs, milliseconds):\n"
		<< "  compute - only the algorithm call, the data is already in RAM;\n"
		<< "  total   - reading the text file, filling RAM, computing and forming the result line\n"
		<< "Build: " << buildLabel << endl;
}

void TablePrinter::printTitle(const string& task) const
{
	_out << "\n==== " << task << " ====" << endl;
}

void TablePrinter::printTitle(const string& task, size_t size) const
{
	_out << "\n==== " << task << " | N = " << size << " ====" << endl;
}

void TablePrinter::printTable(const string& firstColumn, const vector<NamedMeasurement>& rows) const
{
	_out << format("\n{:<22} | {:>14} | {:>14} | {}\n", firstColumn, "compute, ms", "total, ms", "result");
	for (const NamedMeasurement& row : rows)
	{
		_out << format("{:<22} | {:>14.4f} | {:>14.4f} | {}\n", row.name,
			row.measurement.computeTime, row.measurement.totalTime, row.measurement.result);
	}
	_out << flush;
}

Experiments::Experiments(const AlgorithmTimer& timer, const TablePrinter& printer)
	: _timer(timer), _printer(printer) {
}

void Experiments::libraryWithoutPolicy(const AlgorithmSet& algorithms) const
{
	_printer.printTitle("Task 1: library none_of without a policy");
	vector<NamedMeasurement> rows;
	for (size_t size : SEQUENCE_SIZES)
	{
		rows.push_back({ "N = " + to_string(size), _timer.measureMedian(size, algorithms.noPolicy) });
	}
	_printer.printTable("sequence", rows);
}

void Experiments::libraryWithPolicies(const AlgorithmSet& algorithms) const
{
	for (size_t size : SEQUENCE_SIZES)
	{
		_printer.printTitle("Task 2: library none_of with execution policies", size);
		_printer.printTable("policy", measureAll(algorithms.withPolicies, size));
	}
}

vector<NamedMeasurement> Experiments::measureAll(const vector<NamedAlgorithm>& algorithms, size_t size) const
{
	vector<NamedMeasurement> rows;
	for (const NamedAlgorithm& algorithm : algorithms)
	{
		rows.push_back({ algorithm.name, _timer.measureMedian(size, algorithm) });
	}
	return rows;
}