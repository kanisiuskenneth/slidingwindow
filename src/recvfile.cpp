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

using namespace std; 

 
#define buffersize 256  //Max length of buffer
void die(string s)
{
    perror(s.c_str());
    exit(1);
}

void flushbuffer(char* buffer, FILE * of, int len) {
    fprintf(stderr, "writting: %s\n", buffer);
    for(int i=0; i< len; i++) {
        fprintf(of, "%c", buffer[i]);
        buffer[i] = 0x0;
    }
}

void writeLog(string message) {
    time_t now = time (0);
    char time[50];
    FILE * log;
    strftime (time, 100, "%Y-%m-%d %H:%M:%S", localtime (&now));
    log = fopen("log/recvfile.log", "a");
    fprintf(stderr, "%s\n", message.c_str());
    fprintf (log,"%s: ", time);
    fprintf(log, "%s\n", message.c_str());
    fclose(log);
}


int main(int argc, char** args) {
	if(argc < 5) {
		fprintf(stderr, "Arguments must be: <filename> <windowsize> <buffersize> <port>\n");
		return 1;
	}

    time_t now = time (0);
    char timex[50];
    srand(time(NULL));
    FILE * log;
    log = fopen("log/recvfile.log", "w");
    fprintf(log, "Program Started at: ");
    strftime (timex, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
    fprintf (log,"%s\n", timex);
    fclose(log);

    string msg; 

	uint32_t windowsize = atoi(args[2]);
	uint32_t port = atoi(args[4]);
    char* filename = args[1];
    char* buffer = new char[buffersize];
    char* windowbuff = new char[windowsize];
    for(int i= 0; i< windowsize;i++) {
        windowbuff[i] = 0x00;
    }
    struct sockaddr_in si_me, si_other;
     
    unsigned int s, slen = sizeof(si_other) , recv_len;
    char buf[9];
     
    //create a UDP socket
    if ((int)(s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
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
        writeLog("Binding failed");
        die("bind");
    }

    writeLog("Socket Created, binded to port");
    FILE * of;
    of = fopen (filename,"w");
    if(!of) {
        exit(1);
    }
    writeLog("Output file created");
    //keep listening for data
    uint32_t bp(0), exp_packet(0), lfa;
    bool endloop = false;
    while(!endloop)
    {
        lfa = exp_packet + windowsize;
        writeLog("Waiting for data...");
        fflush(stdout);
        

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, 9, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        Packet receivedPacket(buf);

        //print details of the client/peer and the data received
        msg = "Received Packet from ";
        msg += inet_ntoa(si_other.sin_addr);
        msg += " ";
        msg += to_string(ntohs(si_other.sin_port));
        writeLog(msg);
        msg = "Data(SeqNum, Data): ";
        msg += to_string(receivedPacket.getSeqNum());
        char hexData[4];
        sprintf(hexData, "0x%02x", receivedPacket.getData());
        msg += " ";
        msg += hexData;
        writeLog(msg);

        if(receivedPacket.getSeqNum() == exp_packet) {
            writeLog("Expected data found, writting in buffer, sliding window\n");
            windowbuff[0] = receivedPacket.getData();
            bool eoffound = (receivedPacket.getRawData()[0] & 0xff) == 0xff;
            int cwh = 0;
            do { 
                if(eoffound) {
                    continue;
                }
                buffer[bp] = windowbuff[cwh];
                bp++;
                if(bp == buffersize) {
                    writeLog("Buffer filled flushing buffer\n") ;
                    flushbuffer(buffer, of, buffersize);
                    bp = 0;
                }
                windowbuff[cwh]  = 0x00;
                Ack sendack;
                exp_packet++;
                sendack.setSeqNum(exp_packet);
                sendack.setAWS(min(buffersize - bp, windowsize));
                sendack.setChecksum();

                msg = "Sending ACK: ";
                msg += to_string(sendack.getSeqNum());
                //now reply the client with the same data
                if (sendto(s, sendack.getRawData(), 7, 0, (struct sockaddr*) &si_other, slen) == -1)
                {
                    die("sendto()");
                }
                cwh++;
                
            } while(windowbuff[cwh] != 0x00);
            if(eoffound) {
                    writeLog("EOF found\n");
                    writeLog("Terminating Connection");
                    endloop = true;
                    flushbuffer(buffer, of, bp);
                    fclose(of);
            }
        } else {
            writeLog("Expected data not found, ");
            Ack sendack;
            sendack.setSeqNum(exp_packet);
            sendack.setAWS(min(buffersize - bp, windowsize));
            sendack.setChecksum();

            if(receivedPacket.getSeqNum() <= lfa && receivedPacket.getSeqNum() >= exp_packet) {
               writeLog("Writting in window"); 
               windowbuff[receivedPacket.getSeqNum() - exp_packet] = receivedPacket.getData();
            } else {
                writeLog("Rejecting packet");
                continue;
            }
        }   
   
    }
    shutdown(s, 2);
    return 0;
}
