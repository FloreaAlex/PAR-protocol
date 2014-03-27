// Florea Alexandru - Ionut
// 323CA
// Tema1 - PC

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

// fucntie pentru afisarea unui mesaj de eroare
void fatal(char * mesaj_eroare)
{
        perror(mesaj_eroare);
        exit(1);
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

// fucntie care returneaza data si ora curenta
char* currentDateTime(char* str) {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "[%d-%m-%Y %X] ", &tstruct);
	strcpy(str,  &buf[0]);
	return str;
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
	
	// formez numele pentru fisierul de iesire
	char* file_name = malloc(strlen(argv[1]) + 1);
	int i;
	int a = 0;
	for (i = 0; i < strlen(argv[1]) + 1; i ++)
	{
		if (a)
		{
			file_name[i] = argv[1][i - 1];
		}
		else
			file_name[i] = argv[1][i];
		if(argv[1][i] == '.')
		{
			a = 1;
			file_name[i] = '1';
			file_name[i + 1] = '.';
		}
	}

	msg r,t;
	frame f;
	init(HOST,PORT);
	
	FILE* log_file;
	unsigned char frame_expected = 0;
	char str[80];

	// deschid fisierul de iesire
	int file_size;
	int out = open(file_name, O_WRONLY | O_CREAT, 0644);
	lseek(out, 0, SEEK_SET);
	
	while(1)
	{
		log_file = fopen ("log.txt","a");
		if (recv_message(&r) < 0)
		{
			perror("Receive message");
			return -1;
		}
		// caz specila pentru primirea marimii fiserului, necesara pentru a 
		//stii cand s-a trimis tot fisierul
		else
		{

			f.payload = malloc(r.len);
			memcpy(f.payload, &r.data[1], 4);	
			memcpy(&f.no_seq, &r.data[0], 1);	
			memcpy(&f.checksum, &r.data[5], 1);
		
			// daca s-a primit un pachet valid trimit ACK si ies din bucla
			unsigned char checksum = get_checksum(r);
			if (f.no_seq == frame_expected && f.checksum == checksum)
			{
				char to_binary[9];
				strcpy(to_binary, byte_to_binary(checksum));
				fprintf (log_file, "%s", currentDateTime(str));
				fprintf(log_file, "[%s] Am primit mesaj cu:\n\tSeq no:" 
						"%d\n\tPayload: %s\n\tChecksum: %s\n", argv[0], 
						f.no_seq, f.payload, to_binary);
				file_size = atoi(&r.data[1]);
				memcpy(&t.data[0], &frame_expected, 1);
				memcpy(&t.data[1], &frame_expected, 1);
				send_message(&t);
				
				fprintf(log_file, "\tTrimit ACK cu no_seq: %d si astept " 
						"urmatorul packet\n", frame_expected); 
				fprintf(log_file,"------------------------------------------"
					"-----------------------------------------------------\n");
				frame_expected ++;
				fclose(log_file);
				break;
			}
			// daca am primit mesaj eronat trimit un ACK gresit pentru a primii 
			// iar pachetul asteptat
			else
			{
				int nr = 2;
				memcpy(&t.data[0], &nr, 1);
				memcpy(&t.data[1], &frame_expected, 1);
				send_message(&t);
			}
			
		}
	}
	
	// procedez la fel ca in bucla de mai sus, doar ca acesta repeta pana s-au 
	// primit pachete ale caror suma dau in total marimea fisierului
	while(file_size != 0)
	{
		log_file = fopen ("log.txt","a");
		if (recv_message(&r) < 0){
			perror("Receive message");
			return -1;
		}
		else
		{
			f.payload = malloc(r.len);
			memcpy(f.payload, &r.data[1], r.len);
			memcpy(&f.no_seq, &r.data[0], 1);
			memcpy(&f.checksum, &r.data[1 + r.len], 1);
			unsigned char checksum = get_checksum(r);
			
			if (f.no_seq == frame_expected && f.checksum == checksum)
			{
				char to_binary[9];
				strcpy(to_binary, byte_to_binary(checksum));
				fprintf (log_file, "%s", currentDateTime(str));
				fprintf(log_file, "[%s] Am primit mesaj cu:\n\tSeq no:" 
						"%d\n\tPayload: %s\n\tChecksum: %s\n", argv[0], 
						f.no_seq, f.payload, to_binary);
				write(out, f.payload, r.len);
				frame_expected ++;
				file_size -= r.len;
				int to_set = frame_expected - 1;
				memcpy(&t.data[0], &to_set, 1);
				memcpy(&t.data[1], &to_set, 1);
				send_message(&t);
				fprintf(log_file, "Trimit ACK cu no_seq: %d si astept " 
						"urmatorul mesaj\n", frame_expected); 
				fprintf(log_file,"------------------------------------------"
					"-----------------------------------------------------\n");
				fclose(log_file);
			}
			else
			{
				fprintf (log_file, "%s", currentDateTime(str));
				fprintf(log_file, "[%s] Am primit mesaj corupt, retrimit " 
						"ACK cu no_seq: %d\n", argv[0], frame_expected);
				int to_set1 = 0;
				int to_set2 = 1;
				memcpy(&t.data[0], &to_set1, 1);
				memcpy(&t.data[1], &to_set2, 1);
				send_message(&t);
				fclose(log_file);
			}
		}
	}
	
	return 0;
}
