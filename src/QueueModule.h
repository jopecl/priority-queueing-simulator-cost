/*
	QueueModule
*/

#include "FIFO.h"

#define HIGH 1
#define LOW 0

component QueueModule : public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();

	public:
		// Connections
		inport void in(Packet &packet);
		outport void out(Packet &packet);
		outport void trace(char* input);

		// Timer
		Timer <trigger_t> service_time;
		inport inline void endService(trigger_t& t);

		QueueModule () { connect service_time.to_component,endService; }

	public:
		double Q_LP,Q_HP; // Buffer size
		double R; // Transmission Rate
		double blocked_packets_LP,blocked_packets_HP;
		double arrived_packets,arrived_packets_LP,arrived_packets_HP;
		double queue_length_LP,queue_length_HP;

	private:
		FIFO queue_LP;
		FIFO queue_HP;
		int active;
		int priority;

	private:
		char msg[100];
};

void QueueModule :: Setup()
{

};

void QueueModule :: Start()
{
	active = 0;
	priority = HIGH;

	// Statistics
	arrived_packets = 0;
	blocked_packets_HP = 0;
	arrived_packets_HP = 0;
	queue_length_HP = 0;
	blocked_packets_LP = 0;
	arrived_packets_LP = 0;
	queue_length_LP = 0;
};

void QueueModule :: Stop()
{
	// --
	printf("--- Results ---------------------------------------------------------------\n");
	printf("Blocking Probability = (HP=%f|LP=%f)\n",blocked_packets_HP/arrived_packets_HP,blocked_packets_LP/arrived_packets_LP);
	printf("Average number of packets in the queue E[Nq] = (HP=%f|LP=%f) \n",queue_length_HP/arrived_packets,queue_length_LP/arrived_packets);
};

void QueueModule :: in(Packet &packet)
{
	sprintf(msg,"%f - Queue Module: New packet %li (source = %d)! Packets in the buffer: HP=%d | LP=%d",SimTime(),packet.seq_number,packet.source_id,queue_HP.QueueSize(),queue_LP.QueueSize());
	trace(msg);

	arrived_packets++;
	queue_length_LP += queue_LP.QueueSize();
	queue_length_HP += queue_HP.QueueSize();

	if(packet.priority == HIGH) // High Priority
	{
		arrived_packets_HP++;
		if(queue_HP.QueueSize() < Q_HP)
		{
			queue_HP.PutPacket(packet);

			if(queue_HP.QueueSize() == 1 && active == 0)
			{
				priority = HIGH;
				active = 1;
				
				sprintf(msg,"%f - Queue Module HP: Packet %li Scheduled for Transmission of size L=%d",SimTime(),packet.seq_number,packet.L);
				trace(msg);

				service_time.Set(SimTime()+(packet.L/R));
			}
		}
		else 
		{
			blocked_packets_HP++;
		}
	}
	else // Low Priority
	{
		arrived_packets_LP++;

		if(queue_LP.QueueSize() < Q_LP)
		{
			queue_LP.PutPacket(packet);

			if(queue_LP.QueueSize() == 1 && active == 0)
			{
				priority = LOW;
				active = 1;

				sprintf(msg,"%f - Queue Module LP: Packet %li Scheduled for Transmission of size L=%d",SimTime(),packet.seq_number,packet.L);
				trace(msg);

				service_time.Set(SimTime()+(packet.L/R));
			}
		}
		else 
		{
			blocked_packets_LP++;
		}
	}
};

void QueueModule :: endService(trigger_t &)
{
	// Service time ends, we remove the packet from the queue, and send it to the sink module

	if(priority == HIGH)
	{
		Packet packet = queue_HP.GetFirstPacket();
		queue_HP.DelFirstPacket();
		out(packet);
		
		sprintf(msg,"%f - Service Time Ends: Packet of priority %d transmitted",SimTime(),priority);
		trace(msg);

		// We check if the buffer is not empty, and if that is the case, we select another packet for transmission
		if(queue_HP.QueueSize() > 0)
		{
			Packet packet = queue_HP.GetFirstPacket();
			priority = HIGH;
			
			sprintf(msg,"%f - Queue Module HP: Packet %li Scheduled for Transmission of size L=%d",SimTime(),packet.seq_number,packet.L);
			trace(msg);

			service_time.Set(SimTime()+(packet.L/R));
		}
		else
		{
			if(queue_LP.QueueSize() > 0)
			{
				Packet packet = queue_LP.GetFirstPacket();
				priority = LOW;

				sprintf(msg,"%f - Queue Module LP: Packet %li Scheduled for Transmission of size L=%d",SimTime(),packet.seq_number,packet.L);
				trace(msg);

				service_time.Set(SimTime()+(packet.L/R));
			}
			else
			{
				active = 0;
			}
		}
	}	
	else
	{
		Packet packet = queue_LP.GetFirstPacket();
		queue_LP.DelFirstPacket();
		out(packet);

		sprintf(msg,"%f - Service Time Ends: Packet of priority %d transmitted",SimTime(),priority);
		trace(msg);

		// We check if the buffer is not empty, and if that is the case, we select another packet for transmission
		if(queue_HP.QueueSize() > 0)
		{
			Packet packet = queue_HP.GetFirstPacket();
			priority = HIGH;
			
			sprintf(msg,"%f - Queue Module HP: Packet %li Scheduled for Transmission of size L=%d",SimTime(),packet.seq_number,packet.L);
			trace(msg);

			service_time.Set(SimTime()+(packet.L/R));
		}
		else
		{
			if(queue_LP.QueueSize() > 0)
			{
				Packet packet = queue_LP.GetFirstPacket();
				priority = LOW;

				sprintf(msg,"%f - Queue Module LP: Packet %li Scheduled for Transmission of size L=%d",SimTime(),packet.seq_number,packet.L);
				trace(msg);

				service_time.Set(SimTime()+(packet.L/R));
			}
			else
			{
				active = 0;
			}
		}
	}
};
