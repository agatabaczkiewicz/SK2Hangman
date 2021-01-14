server: server.cpp
	c++ -Wall -pthread server.cpp -o ser.out
	./ser.out

client: client.py
	python3 client.py
