#include <iostream>
using namespace std; 

int main(int argc, char** args) {
	if(argc != 5) {
		printf("Arguments must be: <filename> <windowsize> <buffersize> <destination_ip> <destination_port>\n");
	}
	return 0;
}