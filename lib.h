#ifndef LIB
#define LIB

typedef struct {
  unsigned char no_seq;
  unsigned char* payload;
  unsigned char checksum;
} frame;

typedef struct {
	int len;
	char data[1400];
} msg;

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout);

#endif

