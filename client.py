import socket
import threading
import sys
import time 

port = 1234
ip_addr="127.0.0.1"
s=socket.socket()

def connect():
	global s
	try:
		s=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		s.connect((ip_addr,port))
		print("connected")
	except:
		print("sth goes wrong")
		sys.exit(1)

	


def send_data(socket_des,message):
    try: 
        socket_des.send(message.encode())
    except:
        print("send goes wrong")

def receive_data(socket_des):
	dataFromServer = socket_des.recv(1024);
	print(dataFromServer.decode())
	print("Podaj nick")

def receive_data2(socket_des):
	go = True
	while go:
		dataFromServer = socket_des.recv(1024);
		decoded=dataFromServer.decode()
		if decoded =="2":
			print("nick zajety podaj inny")
			send_data(socket_des,"kasia")
		elif decoded =="1":
			print("Witaj")
			print("podaj numer pokoju do ktorego chcesz sie przylaczyc 1-5")
			room="1"
			send_data(socket_des,room)
		elif decoded =="0":
			print("Podaj Nick")
			send_data(socket_des,"agata")
		elif decoded =="3":
			print(f"witaj w pokoju: {room}")
			go=False
			#send_data(socket_des,"agata")
		elif decoded =="4":
			print("Pokoj juz pelny/n 1-sprobuj poniej/n 2 -wybierz inny numer")
			#send_data(socket_des,"agata")
	
	

connect()
receive_data2(s)
#time.sleep(2)
#send_data(s)
#receive_data2(s)
h=1
while h==1:
	h=int(input("podaj 1-dalej,0-koniec"))
s.close()
