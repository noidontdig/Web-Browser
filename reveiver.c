// Alexandra Qin
// aq263
//
// HW3 Receiver

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>


#define ECHOMAX 1004     /* Longest string to echo */
#define BUF_SIZE 1000
#define FRAGS 10000


typedef struct {
	int fragment[10];
	char buf[10][BUF_SIZE];
	int empty[10];
	} Buffer;
	
	
int sock;                        /* Socket */
struct sockaddr_in echoServAddr; /* Local address */
struct sockaddr_in echoClntAddr; /* Client address */
unsigned int cliAddrLen;         /* Length of incoming message */
char echoBuffer[ECHOMAX];        /* Buffer for echo string */
unsigned short echoServPort;     /* Server port */
int recvMsgSize;                 /* Size of received message */

Buffer b;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void DieWithError(char *errorMessage) {
	printf("%s", errorMessage);
};  /* External error handling function */


void initsocket() {
	
	echoServPort = 2000;
	/* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");
}


void *th3(void *ptr);
void *th4(void *ptr);

void AssembleFileFromPackets() {
	
	char *x;
	pthread_t thread3, thread4;
	int iret3, iret4;

	
	iret3 = pthread_create(&thread3, NULL, th3, (void*) x);
	iret4 = pthread_create(&thread4, NULL, th4, (void*) x);
	
	pthread_join(thread3, NULL);
	pthread_join(thread4, NULL);
	
	exit(0);
	
}

int main(int argc, char *argv[]) {
	

	int i;
	for (i = 0; i < 10; i++) {
		b.fragment[i] = -1;
		b.empty[i] = 0;
	}

	initsocket();
	AssembleFileFromPackets();
	close(sock);
	
}


void *th3(void *ptr) {
	
	int cont1 = 1;
	int i, fragnum, x, try;
	char pkt[BUF_SIZE];
	int success = 1;
	int num_of_pkts = 0;
	
	while (cont1) {
				
		try = 0;
		
		if (success) {
			cliAddrLen = sizeof(echoClntAddr);

    		/* Block until receive message from a client */
    		if ((recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0,
        		(struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
        		DieWithError("recvfrom() failed");

		
			fragnum = 0;
			fragnum += (echoBuffer[0] & 0xFF);
			fragnum += (echoBuffer[1] & 0xFF) << 8;
			fragnum += (echoBuffer[2] & 0xFF) << 16; 
			fragnum += (echoBuffer[3] & 0xFF) << 24;
			
			printf("%d\n", fragnum);
			for (i = 0; i < 1000; i++) 
				pkt[i] = echoBuffer[i + 4];	
			num_of_pkts++;
		}
			
		pthread_mutex_lock(&mutex);
		
		x = 0;
		while ((b.empty[x] != 0) && x < 10) 
			x++;
		
		if (b.empty[x] == 0) {
			success = 1;
			b.empty[x] = 1;
			b.fragment[x] = fragnum;
			for (i = 0; i < 1000; i++) 
				b.buf[x][i] = pkt[i];
		}
		else
			success = 0;
		
		if ((success == 0) && (try < 1)) {
			try++;
			pthread_mutex_unlock(&mutex);
			usleep(5000); // sleep for 5ms
		}
		else if ((success == 0) && (try == 1))
			success = 1;
		
		pthread_mutex_unlock(&mutex);
		
		if ((num_of_pkts == FRAGS - 1) && (try == 1)) // if has received all packets
			cont1 = 0;		// stop running thread
		
	}
}


void *th4(void *ptr) {
	 
	int cont2 = 1;
	int i, x, min;
	char s[BUF_SIZE];
	int pos = 0;
	int fragnum = 0;
	time_t t1, t2;
	int timer;
	int fd = open("copy.txt", O_RDWR);
	
	while (cont2) {
		
		time(&t1);
		min = FRAGS;
		
		pthread_mutex_lock(&mutex);
		
		for (i = 0; i < 10; i++) {
			if ((b.empty[i] != 0) && (b.fragment[i] < min)) {
				min = b.fragment[i];
				x = i;
			}			
		}
		
		if (b.fragment[x] != -1) {
			for (i = 0; i < 1000; i++) 
				s[i] = b.buf[x][i];
			if (lseek(fd, pos, SEEK_SET) >= 0) {
				write(fd, s, BUF_SIZE);
				fragnum = b.fragment[x];
				b.empty[x] = 0;
				b.fragment[x] = -1;
				pos += BUF_SIZE;
			}
			else { 
				printf("Could not write to file");
				return;
			}	
		}
		else if (b.fragment[x] == -1){
			time(&t2);
			timer += (int) t2-t1;
			if (timer > 60) {
				printf("Program timeout.");
				cont2 = 0;
			}
		}

		pthread_mutex_unlock(&mutex);
	}
	close(fd);
	return;
}




