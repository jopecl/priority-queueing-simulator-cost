/*
	Sink
*/

component Sink : public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();

	public:
		// Connections
		inport void in(Packet &packet);
		outport void trace(char* input);
		outport void result(char* input);

	public:
		double aggregate_delay_LP,aggregate_delay_HP;
		double received_packets_LP,received_packets_HP;
		double aggregate_L_LP,aggregate_L_HP; // Average packet length

	private:
		char msg[100];
};

void Sink :: Setup()
{

};

void Sink :: Start()
{
	aggregate_delay_HP = 0;
	received_packets_HP = 0;
	aggregate_L_HP=0;

	aggregate_delay_LP = 0;
	received_packets_LP = 0;
	aggregate_L_LP=0;
};

void Sink :: Stop()
{
	printf("# --- Information per traffic source --------------------------------------\n");
	printf("Average Packet Delay E[D] (Queueing + Transmission) [s] = (TS1=%f|TS2=%f)\n",aggregate_delay_HP/received_packets_HP,aggregate_delay_LP/received_packets_LP);
	printf("Number of Received Packets [packets] = (TS1=%f|TS2=%f)\n",received_packets_HP,received_packets_LP);
	printf("Throughput [bps] = (TS1=%f|TS2=%f)\n",aggregate_L_HP/SimTime(),aggregate_L_LP/SimTime());
	
	// Save the results to a file!
	sprintf(msg,"%f,%f,%f,%f",aggregate_delay_HP/received_packets_HP,aggregate_delay_LP/received_packets_LP,aggregate_L_HP/SimTime(),aggregate_L_LP/SimTime());
	result(msg);
};

void Sink :: in(Packet &packet)
{
	sprintf(msg, "%f - Sink: Packet %li Received! Priority Category = %d",SimTime(),packet.seq_number,packet.source_id);
	trace(msg);

	if(packet.source_id == 0)		
	{
		aggregate_delay_HP += SimTime() - packet.send_time;
		aggregate_L_HP += packet.L;
		received_packets_HP++;
	}
	else
	{
		aggregate_delay_LP += SimTime() - packet.send_time;
		aggregate_L_LP += packet.L;
		received_packets_LP++;
	}
};
