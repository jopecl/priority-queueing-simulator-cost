/*
	Logger
*/

#include <fstream>

component Logger : public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();

	public:
		// Connections
		inport void trace(char* input);
		inport void result(char* input);

	private:
		char filenameTraces[100] = "traces.txt";
		char filenameResults[100] = "results.csv";

	public:
		int collectTraces = 0;
		int collectResults = 0;
		int hardcodedLimit = 1000; // This is for the traces
		char myLabels[100]; // This is for the results

	private:
		FILE *fileTraces;
		FILE *fileResults;

	private:
		bool DoesThisFileExist(const char* fileName);
		int counter = 0;
};

void Logger :: Setup()
{
	// Nothing here!
};

void Logger :: Start()
{
	if (collectTraces)
	{
		fileTraces = fopen(filenameTraces, "w"); // Creates a new file
	}

	if (collectResults)
	{
		if (DoesThisFileExist(filenameResults) != 1)
		{
			fileResults = fopen(filenameResults, "w");
			fprintf(fileResults,"%s\n", myLabels);
		}
		else
		{
			fileResults = fopen(filenameResults, "a");
		}
	}
};

void Logger :: Stop()
{
	if (collectTraces) fclose(fileTraces);
	if (collectResults) fclose(fileResults);
};

// Saves a trace in a file
void Logger :: trace(char* input)
{
	if (collectTraces && counter < hardcodedLimit)
	{
		fprintf(fileTraces,"%s\n", input);
		counter++;
	}
};

// Saves results in a file
void Logger :: result(char* input)
{
	if (collectResults)
	{
		fprintf(fileResults,"%s\n", input);
	}
};

// Check if a file exists
bool Logger :: DoesThisFileExist(const char* filename)
{
	std::ifstream infile(filename);
	return infile.good();
};