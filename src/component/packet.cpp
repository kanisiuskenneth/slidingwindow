#include <string.h>
#include <stdint.h>
#include <stdio.h>

class Packet {
	private:

	public:
	char* data;
	Packet(char _data, uint32_t seqNum) {
		data = new char[9];
		for(size_t i=0; i < sizeof(data); ++i) {
			data[i] = 0x00;
		}
		data[0] = 0x01; //STH
		setSeqNum(seqNum);
		data[5] = 0x02; //STX
		data[6] = _data;
		data[7] = 0x03; //ETX
		setChecksum();
	}

	

	Packet(char* _data) {
		data =  _data;
	}

	void setSeqNum(uint32_t seqNum) {
		memcpy(data+1, &seqNum, 4);
		setChecksum();
	}

	void setData(char _data) {
		data[6] = _data;
		setChecksum();
	}

	uint32_t getSeqNum() {
		uint32_t ret;
		memcpy(&ret, data+1, 4);
		return ret;
	}

	int calculateChecksum() {
		char sum = 0;
		for(int i=0; i<8; i++) {
			sum += data[i];
		}
		return sum;
	}

	char getData() {
		return data[6];
	}

	void setChecksum() {
		data[8] = calculateChecksum();
	}

	bool checkChecksum() {
		char sum = 0;
		for(int i=0; i<8;i++) {
			sum += data[i];
		} 
		return data[8] == sum;
	}

	void setAsEnd() {
		data[0] = 0xff;
	}

	int getSize() {
		return sizeof(data);
	}
	char* getRawData() {
		return data;
	}
};