vpath=./pb
CINCLUDES=-I./pb -I/usr/include/google/protobuf/stubs/ 
all: ev1 client

ev1:ev1.o net.pb.o
	g++ -lev -lprotobuf -o ev1 ev1.o net.pb.o $(CINCLUDES)

ev1.o:ev1.cpp 
	g++ -c ev1.cpp -g $(CINCLUDES)

client:client.o net.pb.o	
	g++ -o client client.o net.pb.o -lprotobuf $(CINCLUDES)  

client.o:client.cpp
	g++ -c -g client.cpp -g  $(CINCLUDES)

net.pb.o:./pb/net.pb.cc
	g++ -c ./pb/net.pb.cc -g $(CINCLUDES)
clean:
	rm -f ev1 client *.o


