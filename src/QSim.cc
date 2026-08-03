/*
	QSim
*/

#include "./COST/cost.h"
#include "TrafficSourceBatch.h"
#include "QueueModule.h"
#include "Sink.h"
#include "Tools.h"
#include "Logger.h"

using namespace std;

component QSim : public CostSimEng
{
	public:
		void Setup(double R,double Q_1,double Q_2,double B_1,int EL_1,int type_gen_1, int type_size_1,int source_1,int priority_1,double B_2,int EL_2,int type_gen_2, int type_size_2,int source_2,int priority_2);
		void Start();		
		void Stop();
		
	public:
		TrafficSourceBatch[] source;
		QueueModule queue;
		Sink sink;
		Logger log;
};

void QSim :: Setup(double R,double Q_1,double Q_2,double B_1,int EL_1,int type_gen_1, int type_size_1,int source_1,int priority_1,double B_2,int EL_2,int type_gen_2, int type_size_2,int source_2,int priority_2)
{
	source.SetSize(2);

	// Source 0: 
	source[0].type_generation = type_gen_1;
	source[0].type_size = type_size_1;		
	source[0].L = EL_1; // bits
	source[0].bandwidth = B_1; // bps
	source[0].source_id = source_1;
	source[0].priority = priority_1;
	
	// Source 1: 
	source[1].type_generation = type_gen_2;
	source[1].type_size = type_size_2;		
	source[1].L = EL_2; // bits
	source[1].bandwidth = B_2; // bps
	source[1].source_id = source_2;
	source[1].priority = priority_2;
	
	// Queue
	queue.Q_HP = Q_1;
	queue.Q_LP = Q_2;
	queue.R = R;

	// Connection between components
	connect source[0].out,queue.in;
	connect source[1].out,queue.in;
	connect queue.out,sink.in;

	// Logging!
	log.collectTraces = 1; // Collect the traces (traces.txt)
	log.collectResults = 1; // Collect the results (results.csv)
	sprintf(log.myLabels,"TS1_DELAY,TS2_DELAY,TS1_THROUGHPUT,TS2_THROUGHPUT"); // Put some labels to your results!

	// Connections (Traces)
	connect source[0].trace,log.trace;
	connect source[1].trace,log.trace;
	connect queue.trace,log.trace;
	connect sink.trace,log.trace;

	// Connections (Results)
	connect sink.result,log.result;
};

void QSim :: Start()
{
	// Nothing here!
}

void QSim :: Stop()
{
	// Nothing here!
}

int main(int argc, char *argv[])
{
	double R = atof(argv[1]); // Transmission Rate
	double Q_1 = atof(argv[2]); // Buffer size
	double Q_2 = atof(argv[3]); // Buffer size
	// Traffic HP
	double B_1 = atof(argv[4]); // Load (bps)
	int EL_1 = atoi(argv[5]); // Av. packet size (bits)
	int type_gen_1 = atoi(argv[6]); // Markovian (0) or Deterministic (1)
	int type_size_1 = atoi(argv[7]); // Markovian (0) or Deterministic (1)
	int source_1 = atoi(argv[8]); // Source
	int priority_1 = atoi(argv[9]); // Low priority (0) or High priority (1)
	// Traffic LP
	double B_2 = atof(argv[10]); // Load (bps)
	int EL_2 = atoi(argv[11]); // Av. packet size (bits)
	int type_gen_2 = atoi(argv[12]); // Markovian (0) or Deterministic (1)
	int type_size_2 = atoi(argv[13]); // Markovian (0) or Deterministic (1)
	int source_2 = atoi(argv[14]); // Source
	int priority_2 = atoi(argv[15]); // Low priority (0) or High priority (1)

	QSim QS;

 	long int seed = 2114;

	QS.Seed = seed;
	QS.StopTime(100);
	QS.Setup(R,Q_1,Q_2,B_1,EL_1,type_gen_1,type_size_1,source_1,priority_1,B_2,EL_2,type_gen_2,type_size_2,source_2,priority_2);

	QS.Run();

	return 0;
};
