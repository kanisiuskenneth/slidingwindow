#include <string.h>
#include <stdint.h>
#include <stdio.h>
class Ack {
	char *data;
public:
	Ack() {
		data = new char[7];
		data[0] = 0x06;
	}		

	Ack(char* rawData) {
		data = rawData;
	}

	uint32_t getSeqNum() {
		uint32_t ret;
		memcpy(&ret, data+1, 4);
		return ret;
	}

	void setSeqNum(uint32_t seqNum) {
		memcpy(data+1, &seqNum, 4);
	}
	int calculateChecksum() {
		char sum = 0;
		for(int i=0; i<6; i++) {
			sum += data[i];
		}
		return sum;
	}
	void setChecksum() {
		data[6] = calculateChecksum();
	}
	bool checkChecksum() {
		char sum = 0;
		for(int i=0; i<6;i++) {
			sum += data[i];
		} 
		return data[6] == sum;
	}

	void setAWS(uint8_t x) {
		data[5] = x;
	}
	uint8_t getAWS() {
		return data[5];
	}

	char* getRawData() {
		return data;
	}
};