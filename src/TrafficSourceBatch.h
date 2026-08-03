/*
	TrafficSourceBatch 
*/

#define BATCHSIZE 16 // Batch arrivals!

component TrafficSourceBatch : public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();

	public:
		// Connections
		outport void out(Packet &packet);
		outport void trace(char* input);

		// Timer
		Timer <trigger_t> inter_packet_timer;
		inport inline void new_packet(trigger_t& t); // action that takes place when timer expires

		TrafficSourceBatch () { connect inter_packet_timer.to_component,new_packet; }

	public:
		long seq_number;
		int type_generation; // Markovian (0) or Deterministic (1)
		int type_size; // Markovian (0) or Deterministic (1)
		double bandwidth; // Source bandwidth
		double packet_generation_rate;
		double L;
		int source_id;
		int priority;
			
	private:
		char msg[100];
};

void TrafficSourceBatch :: Setup()
{
	// Nothing here!
};

void TrafficSourceBatch :: Start()
{
	seq_number = 0;
	packet_generation_rate = bandwidth/(L*BATCHSIZE); // Batch arrivals!
	inter_packet_timer.Set(Exponential(1/packet_generation_rate));
};
	
void TrafficSourceBatch :: Stop()
{
	// Nothing here!
};

void TrafficSourceBatch :: new_packet(trigger_t &)
{
	for(int b=0;b<BATCHSIZE;b++)
  {
		Packet packet;

	// Packet size
	if(type_size == 0)
	{	
		// Markovian
		packet.L = (int) Exponential(L);
	}
	else
	{
		// Deterministic
		packet.L = L ;
	}

	packet.seq_number = seq_number;
	seq_number++;
	
	packet.source_id = source_id;
	packet.priority = priority;

	packet.send_time = SimTime(); // To calculate the delay

	sprintf(msg,"%f - Traffic Source: New packet generated (seq_number = %li)",SimTime(),packet.seq_number);
	trace(msg);
	
	out(packet);
	
	}
	
	// Time until next packet is generated
	if(type_generation == 0) 
	{	
		// Markovian
		inter_packet_timer.Set(SimTime()+Exponential(1/packet_generation_rate));
	}
	else
	{
		// Deterministic
		inter_packet_timer.Set(SimTime()+(1/packet_generation_rate));
	}
};
