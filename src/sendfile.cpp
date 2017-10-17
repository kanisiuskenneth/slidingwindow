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
#include <algorithm>
#define buffersize 256
using namespace std; 

void writeLog(string message) {
	time_t now = time (0);
	char time[50];
	FILE * log;
	strftime (time, 100, "%Y-%m-%d %H:%M:%S", localtime (&now));
	log = fopen("log/sendfile.log", "a");
	fprintf(stderr, "%s\n", message.c_str());
	fprintf (log,"%s: ", time);
	fprintf(log, "%s\n", message.c_str());
	fclose(log);
}



void flushbuffer(char* buffer, FILE * ifile, uint32_t &len, bool &lastflush) {
	int it= 0;
	char data;
	writeLog("Flushing Buffer, rewriting buffer with:");
	while(it < buffersize && fscanf(ifile,"%c",&data) != EOF) {
		buffer[it] = data;
		it++;
	}
	if(it != buffersize) {
		lastflush = true;
	}
	writeLog(buffer);
	len = it;
}

void itostring(uint32_t x, string &s) {
	s = "";
	if(x == 0) {
		s = "0";
		return;
	}
	while (x > 0) {
		s.insert(s.begin(),((x%10) + '0'));
		x = x/10;
	}
}

int main(int argc, char** args) {
	char* filename;
	uint32_t windowsize;
	char* destination_ip;
	uint32_t destination_port;
	char* buffer;
	time_t now = time (0);
	char time[100];
	
	FILE * log;
	log = fopen("log/sendfile.log", "w");
	fprintf(log, "Program Started at: ");
	strftime (time, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
    fprintf (log,"%s\n", time);
    fclose(log);

	if(argc != 6) {
		printf("Arguments must be: <filename> <windowsize> <buffersize> <destination_ip> <destination_port>\n");
		writeLog("Argument not found, program exiting");
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
        writeLog("Socket Failed");
        return 1;
    }

    for(uint32_t i=0;i<buffersize;i++) {
    	buffer[i] = 0;
    }

    struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	   writeLog("Socket Error");
	}
	if (setsockopt (fd, SOL_SOCKET, SO_SNDTIMEO, &tv,sizeof(tv)) < 0) {
		writeLog("Socket Error");
	}

    struct sockaddr_in serveraddr, rbuffer;
    memset( &serveraddr, 0, sizeof(serveraddr) );
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons( destination_port );              
    serveraddr.sin_addr.s_addr = inet_addr(destination_ip);
    
	FILE * ifile;
 	ifile = fopen (filename,"r");
 	writeLog(("Opening source file"));
 	 if(ifile) {
 	 	uint32_t filledlength;
 	 	bool lastflush;
 	 	uint32_t cws(0), bp(0);
 	 	uint32_t lfs = 0, lar = 0, start_lfs, start_bp;
 	 	cws = windowsize;
 	 	lfs = 0;
 	 	flushbuffer(buffer, ifile, filledlength, lastflush);
 	 	bool endloop = 0;
 	 	start_lfs = lfs;
 	 	start_bp = bp;
 	 	while(!endloop) {
 	 		fprintf(stderr, "%d %d\n", lastflush, endloop);
 	 		string buf;
 	 		string msg = "Starting Window (LAR, LFS, CWS): ";
 	 		itostring(lar, buf);
 	 		msg+=buf;
 	 		itostring(lfs, buf);
 	 		msg += " ";
 	 		msg+=buf;
 	 		msg += " ";
 	 		itostring(cws, buf);
 	 		msg+=buf;
 	 		writeLog(msg);
 	 		fprintf(stderr, "%d %d %d\n",start_bp, bp, filledlength);
 	 		while((lfs - lar) <= cws && bp < filledlength) {
 	 			char datastring[5];
 	 			sprintf(datastring,"0x%02x" , buffer[bp]);

 	 			Packet sendpacket(buffer[bp], lfs);
 	 			lfs++;
 	 			bp++; 
 	 			string msg = "Sending Data (data, seqnum): ";
 	 			msg += datastring;
 	 			msg += " ";
 	 			msg += to_string(sendpacket.getSeqNum());

 	 			writeLog(msg);
		    	if (sendto( fd, sendpacket.getRawData(), 9, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0 ) {
		            writeLog("Sending Failed");
	    	        break;
		        }
		        writeLog( "Message Sent" );
	    	}


	    	char data[7];
	    	uint32_t slen;

	    	if(recvfrom(fd, data, 7, 0, (struct sockaddr *) &rbuffer,  &slen) < 0) {
	    		writeLog("Timeout, resetting window");
	    		bp = start_bp;
	    		lfs = start_lfs;
	 		} else {
		 		Ack rack(data);
		 		string seqnumstring;
		 		itostring(rack.getSeqNum(), seqnumstring);
		 		string msg = "Got ACK (seqnum): ";
		 		msg += seqnumstring;
		 		writeLog(msg);
		 		if(rack.getSeqNum() < lar+1 || !rack.checkChecksum()) {
		 			writeLog("Expected ack not found, reseting window\n");
		  			bp = start_bp;
		 			lfs = start_lfs;
					cws = min(min((uint32_t)windowsize, (uint32_t)rack.getAWS()), (uint32_t)(filledlength - bp));
		 		}
		 		else {
		 			while(rack.getSeqNum() > lar && !endloop) {
		 				fprintf(stderr, "%d %d %d %d %d\n", rack.getSeqNum(), lar, lfs, bp, start_bp);
						lar++;
		 				start_bp++;
		 				if(start_bp>=filledlength) {
		 					if(!lastflush) {
			 					writeLog("Buffer used, flushing buffer");
			 					flushbuffer(buffer,ifile,filledlength,lastflush);
				 				bp=0;
				 				start_bp=0;
			 				} else {
				 				endloop = true;
				 				writeLog("Transfer Done");
				 				break;
		 					}
		 				}
		 			}

		 			bp = max(bp, start_bp);
		 			lfs = max(bp, start_lfs);
		 			start_lfs = lar;
		 			cws = min(cws, (uint32_t)rack.getAWS());
		 		}
		 	}
	    }
	    fclose(ifile);

	 	Packet terminator(0x00, lfs);
	 	terminator.setAsEnd();
	 	int count = 0;
	    while( count < 10) {
	    	writeLog("Sending Terminator Packet...");
	 		sendto( fd, terminator.getRawData(), 9, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	 		char data[7];
	 		socklen_t slen;
	 		if(recvfrom(fd, data, 7, 0, (struct sockaddr *) &rbuffer,  &slen) >= 0) {
	 			Ack tack(data);
	 			if(tack.getSeqNum() > lfs  && tack.checkChecksum()) {
	 				writeLog("ACK for terminator packet has been received, ending program...");
	 				break;
	 			} 
	 		} else{
	 			writeLog("Timeout");
	 		}
	 		writeLog("ACK for terminator packet haven't been received, resending terminator packet");
	 		count ++;
	 	}
	 	if(count == 10) {
	 		writeLog("Receiver doesn't responding, ending program");
	 	}

     shutdown(fd, 2);
	}
	return 0;
}