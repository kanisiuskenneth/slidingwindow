//Author: 13515008 13515038 13515

#include <stdio.h>      // Default System Calls
#include <stdlib.h>     // Needed for OS X
#include <string.h>     // Needed for Strlen
#include <sys/socket.h> // Needed for socket creating and binding
#include <netinet/in.h> // Needed to use struct sockaddr_in
#include <arpa/inet.h>
#include <time.h>       // To control the timeout mechanism
#include <string>
#include <component/packet.cpp>
#include <component/ack.cpp>
#include <unistd.h>
#define buffersize 256
using namespace std; 

void flushbuffer(char* buffer, FILE * ifile, int &len, bool &lastflush) {
	int it= 0;
	char data;
	while(fscanf(ifile,"%c",&data) != EOF && it < buffersize) {
		buffer[it] = data;
		it++;
	}
	if(it!=buffersize) {
		buffer[it] = EOF;
		it++;
		lastflush = true;
	}
	len = it;
}

int main(int argc, char** args) {
	//Kamus 

	char* filename;
	int windowsize;
	char* destination_ip;
	int destination_port;
	char* buffer;

	if(argc != 6) {
		printf("Arguments must be: <filename> <windowsize> <buffersize> <destination_ip> <destination_port>\n");
		return 1;
	} 
	filename = args[1];
	windowsize = atoi(args[2]);
	destination_ip = args[4];
	destination_port = atoi(args[5]);
	buffer = new char[buffersize];
	//printf("%s %d %d %s %d", filename.c_str(), windowsize, buffersize, destination_ip.c_str(), destination_port);
	int fd;
    if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket failed");
        return 1;
    }

    struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    perror("Error");
	}
    struct sockaddr_in serveraddr, rbuffer;
    memset( &serveraddr, 0, sizeof(serveraddr) );
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons( destination_port );              
    serveraddr.sin_addr.s_addr = inet_addr(destination_ip);
    
	FILE * ifile;
 	 ifile = fopen (filename,"r");
 	 if(ifile) {
 	 	int filledlength;
 	 	bool lastflush;
 	 	uint32_t lsf(0), exp_ack(1), cwst ,cws, cwp(0), bp(0), sbp(0), slsf(0);
 	 	cws = windowsize;
 	 	flushbuffer(buffer, ifile, filledlength, lastflush);
 	 	bool endloop = 0;
 	 	printf("%d %d\n", filledlength, lastflush);
 	 	while(!endloop) {
 	 		printf("Starting Window: bufferpointer:%d startseqnum%d \n",sbp, slsf);
 	 		while(cwp < cws && bp < filledlength) {
 	 			cwst = cwp;
 	 			Packet sendpacket(buffer[bp], lsf);
 	 			lsf++;
 	 			bp++;
 	 			cwp++;
 	 			printf("sending: data:%c seqnum:%u\n", sendpacket.getData(), sendpacket.getSeqNum());
		    	if (sendto( fd, sendpacket.getRawData(), 9, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0 ) {
		            perror( "sendto failed" );
	    	        break;
		        }
		        fprintf(stderr, "%c", sendpacket.getData());
		        printf( "message sent\n" );
	    	}
	    	char data[7];
	    	uint32_t slen;

	    	if(recvfrom(fd, data, 7, 0, (struct sockaddr *) &rbuffer,  &slen) < 0) {
	    		printf("timout, reseting window\n");
	    		bp = sbp;
	    		lsf = slsf;
		 		cwp = 0;
	 		} else {
		 		Ack rack(data);
		 		printf("%u %u\n", rack.getSeqNum(), exp_ack);
		 		if(rack.getSeqNum() != exp_ack || !rack.checkChecksum()) {
		 			printf("expected ack not found, reseting window\n");
		 			bp = sbp;
		 			lsf = slsf;
		 			cwp = max(0, windowsize-(rack.getAWS()));
		 			
		 		} 
		 		else {
		 			exp_ack++;
		 			cwp--;
		 			sbp++;
		 			slsf++;
		 			if(bp == filledlength) {
		 				if(!lastflush) {
		 					printf("buffer used, flushing buffer\n");
		 					flushbuffer(buffer,ifile,filledlength,lastflush);
		 					
		 					printf("%s\n", buffer);
			 				bp=0;
			 				sbp = 0;
			 				cwp = 0;
		 				} else {
		 					cwp++;
		 					if(rack.getSeqNum() == lsf) {
			 					endloop = true;
			 					printf("Transfer Done\n");
			 					break;
		 					}	
		 				}
		 				
		 			}
		 			printf("expected ack found, sending next data\n");
		 		}
		 	}
	    }
	    fclose(ifile);
 	 }
     
     shutdown(fd, 2);
	return 0;
}