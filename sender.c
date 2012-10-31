// Alexandra Qin
// aq263
//
// HW3 Sender

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>


#define ECHOMAX 1004     /* Longest string to echo */
#define BUF_SIZE 1000
#define FRAGS 10000


typedef struct {
	int fragment[10];
	char buf[10][BUF_SIZE];
	int empty[10];
	} Buffer;
	
	
int sock;                        /* Socket descriptor */
struct sockaddr_in echoServAddr; /* Echo server address */
struct sockaddr_in fromAddr;     /* Source address of echo */
unsigned short echoServPort;     /* Echo server port */
char *servIP;                    /* IP address of server */
char *echoString;                /* String to send to echo server */
int echoStringLen;               /* Length of string to echo */

Buffer b;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void DieWithError(char *errorMessage) {
	printf("%s", errorMessage);
};  /* External error handling function */


void initsocket() {
	
	servIP = "127.0.0.1";           /* First arg: server IP address (dotted quad) */
	echoServPort = 2000;
			    /* Create a datagram/UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

			    /* Construct the server address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
	echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
	echoServAddr.sin_port   = htons(echoServPort);     /* Server port */
	
}


void *th1(void *ptr);
void *th2(void *ptr);

void SendFileInPackets() {
	
	char *x;
	pthread_t thread1, thread2;
	int iret1, iret2;
	
	iret1 = pthread_create(&thread1, NULL, th1, (void*) x);
	iret2 = pthread_create(&thread2, NULL, th2, (void*) x);
	
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	
	exit(0);
	
}


int main(int argc, char *argv[]) {
	
	int i;
	for (i = 0; i < 10; i++) {
		b.fragment[i] = -1;
		b.empty[i] = 0;
	}
	
	initsocket();	
	SendFileInPackets();
	close(sock);
}


void *th1(void *ptr) {
	
	int fd = open("random.txt", O_RDONLY); 
	int x = 0; 
	int success = 1;
	int pos = 0;
	char s[BUF_SIZE];
	int cont1 = 1;
	int i;
	while (cont1) {
		
		i = 0;
		// a
		if (success) {
			
			success = 0;
			if (lseek(fd, pos, SEEK_SET) >= 0) {
				read(fd, s, BUF_SIZE);
				pos += BUF_SIZE;
				x++;
			}
			else { 
				printf("Could not read file");
				return;
			}		
		}
		// b
		pthread_mutex_lock(&mutex);
		// c
		while ((b.empty[i] != 0) && i < 10) 
			i++;
		// d
		if (b.empty[i] == 0) {
			success = 1;
			b.empty[i] = 1;
			b.fragment[i] = x;
			strcpy(b.buf[i], s);
		}
		// e
		else {
			success = 0;
		}
		// f
		pthread_mutex_unlock(&mutex);
		
		if (x == FRAGS - 1) // if this is the last fragment
			cont1 = 0;		// stop running thread
		
		usleep(1000); // sleep for 1ms
	}
	close(fd);
}


void *th2(void *ptr) {
	
	char pkt[BUF_SIZE + 4];
	int cont2 = 1;
	int i, j, min, x, frag, fragnum, same_frag;
	int current_frag = 0;
	echoString = (char *) malloc(sizeof(pkt));

	while (cont2) {
		min = FRAGS;
		i = 0;
		j = 0;
		same_frag = 0;
		
		
		// a
		pthread_mutex_lock(&mutex);
		// b
		for (i = 0; i < 10; i++) {
			if ((b.empty[i] != 0) && (b.fragment[i] < min)) {
				min = b.fragment[i];
				x = i;
			}			
		}
		if (b.fragment[x] == FRAGS - 1) // if this is the last fragment
			cont2 = 0;					// stop running thread
		// c
		if (b.fragment[x] != -1){
			
			frag = b.fragment[x];		
			pkt[3] = (frag >> 24) & 0xFF;
			pkt[2] = (frag >> 16) & 0xFF;
			pkt[1] = (frag >> 8) & 0xFF;
			pkt[0] = frag & 0xFF;	
			
			for (j = 0; j < BUF_SIZE; j++) {
				pkt[j+4] = b.buf[x][j];
			}
			
							
			b.empty[x] = 0;
			b.fragment[x] = -1;
		}
			
		// d
		pthread_mutex_unlock(&mutex);
		// e
		if (frag > current_frag) 
			current_frag = frag;
		else if (frag == current_frag)
			same_frag = 1;
			
		fragnum = 0;
		fragnum += (pkt[0] & 0xFF);
		fragnum += (pkt[1] & 0xFF) << 8;
		fragnum += (pkt[2] & 0xFF) << 16; 
		fragnum += (pkt[3] & 0xFF) << 24;
		
		if (same_frag == 0) { // makes sure to only send each packet once
			
			printf("%d\n", fragnum); // print number of frag you are sending		

		    // send frag 
			if (sendto(sock, pkt, 1004, 0, (struct sockaddr *)
				&echoServAddr, sizeof(echoServAddr)) != 1004)
				DieWithError("sendto() sent a different number of bytes than expected");
		}
	}
}


