all: sendfile | recvfile

sendfile: sendfile.cpp
	g++ sendfile.cpp -o sendfile -std=c++11 -I .
recvfile: recvfile.cpp
	g++ recvfile.cpp -o recvfile -std=c++11 -I .
