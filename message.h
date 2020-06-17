#ifndef MESSAGE_H_INCLUDED
#define MESSAGE_H_INCLUDED

typedef struct message_header_t {

	char type;
	char error;
    in_addr_t   m_addr;				// Peer's IP address
    in_port_t   m_port;				// Peer's port number
	unsigned int room;
	unsigned int payload_length;
} message_header;

typedef struct packet_t {
	struct message_header_t header;
	char payload[1000];
} packet;

#endif // MESSAGE_H_INCLUDED
