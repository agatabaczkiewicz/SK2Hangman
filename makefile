server: server.cpp
	c++ -Wall -pthread server.cpp -o ser.out
	./ser.out

client: main.py
	python3 main.py
