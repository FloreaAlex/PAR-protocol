// Florea Alexandru - Ionut
// 323CA
// Tema1 - PC

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

// fucntie care returneaza data si ora curenta
char* currentDateTime(char* str) {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "[%d-%m-%Y %X] ", &tstruct);
	strcpy(str,  &buf[0]);
	return str;
}

// functie pentru transformarea unui octet intr-un sir format din valoarea sa 
// in binar
char *byte_to_binary(unsigned char x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

// functie pentru determinarea checksumu-ului unui sir de octeti prin 
//efectuarea operatiei xor intre ei
unsigned char get_checksum(msg m) 
{
	unsigned char checksum = m.data[0];
	int i;
	for (i = 1; i < m.len; i ++)
	{
		checksum ^= m.data[i];
	}
	return  checksum;
}

int main(int argc, char** argv){
	init(HOST,PORT);
	msg m1, *m2;
	frame f;
	
	//fisierul de log
	FILE* log_file;
	log_file = fopen ("log.txt","w");
	fclose(log_file);
	
	// fisierul de intrare	
	int in = open(argv[1], O_RDONLY);
	lseek(in, 0, SEEK_SET);
	
	struct stat s;
	fstat (in, &s);
	int file_size = (int) s.st_size;
	
	unsigned char checksum;
	int got_correct_ack = 1;
	int size;
	unsigned char nr_frame = 0;
	char str[80];
	
	while (file_size != 0)
	{
		log_file = fopen ("log.txt","a");
		
		// daca s-a primit ACK corect se intra pe aceasta ramura
		if (got_correct_ack)
		{
			// se trimite marimea fisierului de intrare, am considerat acest 
			// mesaj ca fiind un caz separat  
			if (nr_frame == 0)
			{
				m1.len = sizeof(int);
				f.payload = malloc(sizeof(int));
				memcpy(&m1.data[0], &nr_frame, 1);
				sprintf(&m1.data[1],"%d", file_size);
				sprintf((char*)f.payload,"%d", file_size);
				checksum = get_checksum(m1);
				memcpy(&m1.data[5], &checksum, 1);
				send_message(&m1);
				
				char to_binary[9];
				strcpy(to_binary, byte_to_binary(checksum));
				fprintf (log_file, "%s", currentDateTime(str));
				fprintf(log_file, "[%s] Am trimis pachetul cu:\n\tSeq no:" 
						"%d\n\tPayload: %s\n\tChecksum: %s\n", argv[0], 0, 
						f.payload, to_binary); 
				fprintf(log_file,"------------------------------------------"
					"-----------------------------------------------------\n");
			}
			// daca a fost trimisa marimea fisierului atunci se intra numai pe 
			// aceasta ramura
			else
			{
				int payload_size = (rand() % (60 + 1 - 1)) + 1;
				if (file_size <= payload_size)
				{
					payload_size = file_size;
					f.payload = malloc(payload_size);
					read(in, f.payload, file_size);
				}
				else
				{
					f.payload = malloc(payload_size);
					read(in, f.payload, payload_size);
				}
				
				m1.len = payload_size;
				size = m1.len;
				memcpy(&m1.data[0], &nr_frame, 1);
				memcpy (&m1.data[1], &f.payload[0], payload_size);
				checksum = get_checksum(m1);
				memcpy(&m1.data[1 + payload_size], &checksum, 1);
				send_message(&m1);
				
				char to_binary[9];
				strcpy(to_binary, byte_to_binary(checksum));
				fprintf (log_file, "%s", currentDateTime(str));
				fprintf(log_file, "[%s] Am trimis pachetul cu:\n\tSeq no:" 
						"%d\n\tPayload: %s\n\tChecksum: %s\n", argv[0], 
						nr_frame, f.payload, to_binary); 
				fprintf(log_file,"------------------------------------------"
					"-----------------------------------------------------\n");
			}
		}
		
		// daca nu s-a primit ACK corect se retrimite ultimul pachet
		else
		{
			send_message(&m1);
		}
		
		// verific daca s-a primit timeout
		if ((m2 = receive_message_timeout(50)) == NULL)
		{
			got_correct_ack = 0;
			fprintf (log_file, "%s", currentDateTime(str));
			fprintf(log_file, "[%s] Am primit timeout, retrimit pachetul" 
				"cu no_seq: %d\n", argv[0], nr_frame); 
			fprintf(log_file,"------------------------------------------"
					"-----------------------------------------------------\n");
			fflush(log_file);

		}
		// daca nu s-a primit timeout verific daca s-a primit ACK corect
		else 
		{
			memcpy(&f.no_seq, m2->data, 1);
			memcpy(&f.checksum, m2->data + 1, 1);
			if (f.no_seq == f.checksum)
			{
				fprintf (log_file, "%s", currentDateTime(str));
				fprintf(log_file, "[%s] Am primit ACK corect pentru " 
						"pachetul cu no_seq: %d, trimit urmatorul pachet\n", 
						argv[0], nr_frame); 
				fprintf(log_file,"------------------------------------------"
					"-----------------------------------------------------\n");
				fclose(log_file);
				got_correct_ack = 1;
				file_size -= size;
				nr_frame ++;
				free(f.payload);
			}
			else
			{
				fprintf (log_file, "%s", currentDateTime(str));
				fprintf(log_file, "[%s] Am primit ACK gresit, retrimit " 
						"pachetul cu no_seq: %d", argv[0], nr_frame); 
				fprintf(log_file,"------------------------------------------"
					"-----------------------------------------------------\n");
				fclose(log_file);
				got_correct_ack = 0;
			}
		}
	}
	
	log_file = fopen ("log.txt","a");
	fprintf (log_file, "%s", currentDateTime(str));
	fprintf(log_file, "[%s] Am trimis tot fisierul %s\n", argv[0], argv[1]); 
	fclose(log_file);
	return 0;
}
