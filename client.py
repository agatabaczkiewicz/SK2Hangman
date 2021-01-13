import socket
import threading
import sys
import time 
import signal

port = 1235
ip_addr="127.0.0.1"
s=socket.socket()

def signal_handler(signal, frame):
	s.close()
	print("sss")
	sys.exit(0)

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
        message+="$$"
        socket_des.send(message.encode())
    except:
        print("send goes wrong")

def receive_data(socket_des):
	dataFromServer = socket_des.recv(1024);
	print(dataFromServer.decode())
	print("Podaj nick")

def receive_data2(socket_des):
	go = True
	decoded=""
	while go:
		while decoded.find("$$") == -1:
			dataFromServer = socket_des.recv(3);
			decoded=dataFromServer.decode()
		decoded=decoded[:-2]
		if decoded =="2":
			print("nick zajety podaj inny\n")
			name2=input("Podaj Nick\n")
			send_data(socket_des,name2)
			
		elif decoded =="1":
			print("Witaj\n")
			#print("podaj numer pokoju do ktorego chcesz sie przylaczyc 1-5")
			room=input("podaj numer pokoju do ktorego chcesz sie przylaczyc 1-5\n")
			send_data(socket_des,room)
		elif decoded =="0":
			print(f"czesc {name}\n")
			#name=input("Podaj Nick\n")
			#send_data(socket_des,name)
		elif decoded =="3":
			print(f"witaj w pokoju: {room}\n")
			name=input("Podaj Nick\n")
			send_data(socket_des,name)
			#go=False
			#send_data(socket_des,"agata")
		elif decoded =="4":
			print("Pokoj juz pelny/n 1-sprobuj pozniej/n 2 -wybierz inny numer")
			#send_data(socket_des,"agata")
		elif decoded =="5":
			wait=input("chcesz czekac za kolejnym graczem - 0 , gramy - 1\n")
			send_data(socket_des,wait)
		elif decoded =="6":
			print("jest was 5 automatycznie zaczynamy gre");
			go=False;
			

	
	
try:
	connect()
	receive_data2(s)
#time.sleep(2)
#send_data(s)
#receive_data2(s)
	f=1
	signal.signal(signal.SIGINT, signal_handler)
	while f==1:
		f=int(input("podaj 1-dalej,0-koniec"))
	'''if h==0:
		send_data(s,"666")
		f=0'''
	s.close()
except KeyboardInterrupt:  #obsluga ctr+C
	s.close()
	sys.exit(0)













