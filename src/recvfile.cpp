#include <stdio.h>      // Default System Calls
#include <stdlib.h>     // Needed for OS X
#include <string.h>     // Needed for Strlen
#include <sys/socket.h> // Needed for socket creating and binding
#include <netinet/in.h> // Needed to use struct sockaddr_in
#include <arpa/inet.h>
#include <component/packet.cpp>
#include <component/ack.cpp>
#include <time.h>       /* time */
#include <unistd.h>
using namespace std; 

 
#define buffersize 256  //Max length of buffer
void die(char* s)
{
    perror(s);
    exit(1);
}

void flushbuffer(char* buffer, FILE * of, int len) {
    printf("writting: %s\n", buffer);
    for(int i=0; i< len; i++) {
        fprintf(of, "%c", buffer[i]);
        buffer[i] = 0x0;
    }
}

int main(int argc, char** args) {
	if(argc < 5) {
		fprintf(stderr, "Arguments must be: <filename> <windowsize> <buffersize> <port>\n");
		return 1;
	}
	int windowsize = atoi(args[2]);
	int port = atoi(args[4]);
    char* filename = args[1];
    char* buffer = new char[buffersize];
    char* windowbuff = new char[windowsize];
    for(int i= 0; i< windowsize;i++) {
        windowbuff[i] = 0x00;
    }
    struct sockaddr_in si_me, si_other;
     
    unsigned int s, i, slen = sizeof(si_other) , recv_len;
    char buf[9];
     
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
    FILE * of;
    of = fopen (filename,"w");
    if(!of) {
        exit(1);
    }
    //keep listening for data
    int bp(0), sbp(0), exp_packet(0), lfa;
    bool endloop = false;
    while(!endloop)
    {
        lfa = exp_packet + windowsize;
        printf("Waiting for data...");
        fflush(stdout);
        

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, 9, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        Packet receivedPacket(buf);

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %d %x\n", receivedPacket.getSeqNum(), receivedPacket.getData());
        if(receivedPacket.getSeqNum() == exp_packet) {
            printf("Expected data found, writting in buffer, sliding window\n");
            windowbuff[0] = receivedPacket.getData();
            int cwh = 0;
            while(windowbuff[cwh] != 0x00) {
                if(bp == buffersize) {
                    printf("buffer filled flushing buffer\n") ;
                    flushbuffer(buffer, of, buffersize);
                    bp = 0;
                }
                if((windowbuff[cwh] & 0xff) == 0xff) {
                    printf("EOF found\n");
                    endloop = true;
                    flushbuffer(buffer, of, bp);
                    fclose(of);
                } else {
                    printf("writting in buffer: %c\n", windowbuff[cwh]);
                    buffer[bp] = windowbuff[cwh];
                    bp++;
                }
                windowbuff[cwh]  = 0x00;
                Ack sendack;
                sendack.setSeqNum(receivedPacket.getSeqNum()+1);
                sendack.setAWS(buffersize - bp);
                sendack.setChecksum();
                printf("Sending ACK: %d\n", sendack.getSeqNum());
                //now reply the client with the same data
                if (sendto(s, sendack.getRawData(), 7, 0, (struct sockaddr*) &si_other, slen) == -1)
                {
                    die("sendto()");
                }
                exp_packet++;
                cwh++;
                
            }
        } else {
            printf("Expected data not found, ");
            if(receivedPacket.getSeqNum() <= lfa && receivedPacket.getSeqNum() >= exp_packet) {
               printf("writting in window\n;"); 
               windowbuff[receivedPacket.getSeqNum() - exp_packet] = receivedPacket.getData();
            } else {
                printf("rejecting packet\n");
                continue;
            }
        }   
        srand(time(NULL));
        int sleep = rand() % 1000000;
        printf("sleeping for(microsec):%d\n", sleep);
        usleep(sleep);
    }
    printf("\n");
    shutdown(s, 2);
    return 0;
}
