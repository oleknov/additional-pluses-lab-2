#include "funcs.h"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <thread>

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
		<< "Own algorithm: K is the number of threads, speedup is relative to K = 1.\n"
		<< "Hardware threads: " << cpuThreadCount() << '\n'
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


size_t cpuThreadCount()
{
	return max(1u, thread::hardware_concurrency());
}

vector<size_t> Experiments::threadCountsToTest(size_t cpuThreads) const
{
	vector<size_t> counts;
	for (size_t k = 1; k <= cpuThreads; ++k)
	{
		counts.push_back(k);
	}
	for (size_t factor : { 2, 4, 8 })
	{
		counts.push_back(factor * cpuThreads);
	}
	return counts;
}

vector<ThreadMeasurement> Experiments::measureThreadCounts(const AlgorithmSet& algorithms, size_t cpuThreads, size_t size) const
{
	vector<ThreadMeasurement> rows;
	for (size_t threadCount : threadCountsToTest(cpuThreads))
	{
		rows.push_back({ threadCount, _timer.measureMedian(size, algorithms.threaded(threadCount)) });
	}
	return rows;
}

void TablePrinter::printThreads(const vector<ThreadMeasurement>& rows, size_t cpuThreads) const
{
	const auto fastest = ranges::min_element(rows, {}, [](const ThreadMeasurement& row) -> timeT {
		return row.measurement.computeTime;
		});
	printThreadsTable(rows, fastest->threadCount);
	printFastestThreadCount(rows, *fastest, cpuThreads);
	printGrowthLaw(rows, cpuThreads);
	_out << flush;
}

void TablePrinter::printThreadsTable(const vector<ThreadMeasurement>& rows, size_t fastestThreadCount) const
{
	_out << "\nOwn parallel none_of\n";
	_out << format("{:>5} | {:>14} | {:>14} | {:>8}\n", "K", "compute, ms", "total, ms", "speedup");
	for (const ThreadMeasurement& row : rows)
	{
		const timeT computeTime = row.measurement.computeTime;
		_out << format("{:>5} | {:>14.4f} | {:>14.4f} | {:>8.2f}{}\n", row.threadCount, computeTime,
			row.measurement.totalTime, speedupOverOneThread(rows, row),
			(row.threadCount == fastestThreadCount) ? "  <-- best" : "");
	}
}

void TablePrinter::printFastestThreadCount(const vector<ThreadMeasurement>& rows, const ThreadMeasurement& fastest, size_t cpuThreads) const
{
	const timeT speedup = speedupOverOneThread(rows, fastest);
	const double ratio = (double)fastest.threadCount;
	_out << format("Best K = {} (by compute time): {:.2f} times faster than K = 1; hardware threads = {}; K / threads = {:.2f}\n",
		fastest.threadCount, speedup, cpuThreads, ratio / cpuThreads);
}

void TablePrinter::printGrowthLaw(const vector<ThreadMeasurement>& rows, size_t cpuThreads) const
{
	const auto first = ranges::find_if(rows, [&](const ThreadMeasurement& row) -> bool {
		return row.threadCount >= cpuThreads;
		});
	const ThreadMeasurement& last = rows.back();
	const timeT timeChange = last.measurement.computeTime - first->measurement.computeTime;
	const timeT timePerThread = timeChange / (last.threadCount - first->threadCount);
	_out << format("For K from {} to {}: the compute time changes by {:+.4f} ms per added thread on average\n",
		first->threadCount, last.threadCount, timePerThread);
}

timeT TablePrinter::speedupOverOneThread(const vector<ThreadMeasurement>& rows, const ThreadMeasurement& row) const
{
	return rows.front().measurement.computeTime / row.measurement.computeTime;
}

void Experiments::ownThreadedAlgorithm(const AlgorithmSet& algorithms) const
{
	const size_t threads = cpuThreadCount();
	for (size_t size : SEQUENCE_SIZES)
	{
		_printer.printTitle("Task 3: own parallel none_of", size);
		_printer.printThreads(measureThreadCounts(algorithms, threads, size), threads);
	}
}
