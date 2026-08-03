/*
	FIFO
*/

component FIFO : public TypeII
{	
	private:
		deque <Packet> m_queue;

	public:
		Packet GetFirstPacket();
		Packet GetPacketAt(int n);
		void DelFirstPacket();
		void DeletePacketIn(int i);
		void PutPacket(Packet &packet);
		void PutPacketFront(Packet &packet);
		void PutPacketIn(Packet &packet, int);
		int QueueSize();
};

Packet FIFO :: GetFirstPacket()
{
	return(m_queue.front());	
};

Packet FIFO :: GetPacketAt(int n)
{
	return(m_queue.at(n));	
};

void FIFO :: DelFirstPacket()
{
	m_queue.pop_front();
};

void FIFO :: PutPacket(Packet &packet)
{	
	m_queue.push_back(packet);
};

void FIFO :: PutPacketFront(Packet &packet)
{	
	m_queue.push_front(packet);
};

int FIFO :: QueueSize()
{
	return(m_queue.size());
};

void FIFO :: PutPacketIn(Packet & packet,int i)
{
	m_queue.insert(m_queue.begin()+i,packet);
};

void FIFO :: DeletePacketIn(int i)
{
	m_queue.erase(m_queue.begin()+i);
};
