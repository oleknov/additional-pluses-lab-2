#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

const std::string DATA_DIRECTORY = "data";
const std::string OUTPUT_FILE = "output.txt";

using dataT = int;

const std::array<size_t, 5> SEQUENCE_SIZES{ 1'000, 10'000, 100'000, 1'000'000, 10'000'000 };
const dataT MIN_VALUE = 1;
const dataT MAX_VALUE = 1'000'000;
const unsigned RANDOM_SEED = 20250;
const size_t RUNS_PER_MEASUREMENT = 5;

using timeT = long double;
using TimeUnit = std::milli;
using Algorithm = std::function<bool(const std::vector<dataT>&)>;

struct NamedAlgorithm
{
	std::string name;
	Algorithm run;
};

struct AlgorithmSet
{
	NamedAlgorithm noPolicy;
	std::vector<NamedAlgorithm> withPolicies;
};

struct Measurement
{
	timeT totalTime;
	timeT computeTime;
	bool result;
};

struct NamedMeasurement
{
	std::string name;
	Measurement measurement;
};

timeT measureTime(std::function<void()> operation);
std::vector<dataT> randomSequence(size_t size, std::mt19937& gen);
std::vector<dataT> readSequence(std::istream& s);
void writeSequence(std::ostream& os, const std::vector<dataT>& v);
std::string testFilePath(size_t size);
bool testFilesExist();
void generateTestFiles();
std::vector<dataT> readTestFile(size_t size);

class AlgorithmTimer
{
public:
	explicit AlgorithmTimer(std::ostream& console);
	Measurement measureMedian(size_t size, const NamedAlgorithm& algorithm) const;
private:
	std::ostream& _console;
	Measurement timeOneRun(size_t size, const NamedAlgorithm& algorithm, std::string& resultLine) const;
	std::string makeResultLine(size_t size, const std::string& algorithm, bool result) const;
	Measurement medianOfRuns(std::vector<Measurement> runs) const;
};

class TablePrinter
{
public:
	explicit TablePrinter(std::ostream& out);
	void printDescription(const std::string& buildLabel) const;
	void printTitle(const std::string& task) const;
	void printTitle(const std::string& task, size_t size) const;
	void printTable(const std::string& firstColumn, const std::vector<NamedMeasurement>& rows) const;
private:
	std::ostream& _out;
};

class Experiments
{
public:
	Experiments(const AlgorithmTimer& timer, const TablePrinter& printer);
	void libraryWithoutPolicy(const AlgorithmSet& algorithms) const;
	void libraryWithPolicies(const AlgorithmSet& algorithms) const;
private:
	const AlgorithmTimer& _timer;
	const TablePrinter& _printer;
	std::vector<NamedMeasurement> measureAll(const std::vector<NamedAlgorithm>& algorithms, size_t size) const;
};