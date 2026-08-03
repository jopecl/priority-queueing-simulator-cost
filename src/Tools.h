/*
	Tools
*/

struct Packet
{
	long seq_number; // Packet ID
	int L; // Packet length
	double send_time; // Time at which the packet is generated
	int source_id; // Low latency (0) or Data (1)
	int priority; // Low latency (0) or Data (1)
};
