import socket
import threading
import sys
import time
import signal

port = 1235
ip_addr = "127.0.0.1"
s = socket.socket()
word = ""
ids = []
hangman = 0
my_id = ""
hang_dic = {}
score = 0
players=0


def init():
    with open('configclient.txt', 'r') as reader:
        ip_add = reader.readline().rstrip()
        p = int(reader.readline().rstrip())
    return ip_add, p


def signal_handler(signal, frame):
    s.close()
    print("sss")
    sys.exit(0)


def connect(ip_addr, port):
    global s
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((ip_addr, port))
        print("connected")
    except:
        print("sth goes wrong")
        sys.exit(1)


def send_data(socket_des, message):
    try:
        message += "$$"
        socket_des.send(message.encode())
    except:
        print("send goes wrong")


def receive_ids(socket_des):
    global ids
    get = ''
    while get.find("$$") == -1:
        dataFrom = socket_des.recv(18)
        get = dataFrom.decode()
        if get.find(".") != -1:
            get = get[1:get.find(".")]
        else:
            get = get[:-2]
        print(get)
        for x in get:
            if x != ";":
                ids.append(x)
        print(ids)
        break


def receive_word(socket_des):
    global word
    global ids
    get = ''
    while len(get) < 38:
        dataFrom = socket_des.recv(38)
        get_part = dataFrom.decode()
        get += get_part
    get = get.replace(".", "")
    print(get)
    i = get.find(";")
    word = get[0:i]
    s = ""
    for sign in get[i+1::]:
        if sign != ";" and sign != "$":
            s += sign
        elif sign == ";":
            hang_dic[s] = 0
            ids.append(s)
            s = ""
        elif sign == "$":
            print(word)
            print(ids)
            return


def receive_game(socket_des):
    buffer_size = 1024
    read = 0
    data_final = ""
    while read < 6:
        data = socket_des.recv(buffer_size)
        data = data.decode()
        read += len(data)
        data_final += data
    if len(data_final) % 6 == 0:
        final_list = []
        for _ in range(int(len(data_final) / 6)):
            final_list.append(data_final[:4])
            data_final = data_final[6:]
        print(final_list)
            # '''for f in final_list:
            #     if f[0] == "8":  # hangman
            #         if f[2] == ".":
            #             hang_dic[f[1]] += int(f[3])
            #         else:
            #             hang_dic[f[1:3]] += int(f[3])  # id jest dwucyfrowy
            #         # if f[1:3] == my_id or f[1] == my_id:
            #             # zwieksz wisielca na ekranie
            #        # if hang_dic[my_id] == 6:
            #             # trzeba jakos zablokowac mozliwosc wysylanie i klikania gracza ale caly czas moze dostawac wiadomosci
            #             #print("koniec gry")

            #     elif f[0] == "9":
            #         if f[2] == ".":
            #             # odkryj litere f[3]
            #             if(f[1] == my_id):
            #                 score += word.count(f[3])

            #         # else:
            #             # odkryj litere f[3]
            #         if(f[1:3] == my_id):
            #             score += word.count(f[3])
            #             print(f"my score {score}\n")'''

    else:
        print("errr")
        print(data_final)
        print("\n")
       # return [("er", "01")]  # bad receive code


def receive_data2(socket_des):
    go = True
    decoded = ""
    while go:
        while decoded.find("$$") == -1:
            dataFromServer = socket_des.recv(5)
            decoded = dataFromServer.decode()
        decoded = decoded[:-2]
        if decoded == "200":
            print("nick zajety podaj inny\n")
            name2 = input("Podaj Nick\n")
            send_data(socket_des, name2)

        elif decoded == "100":
            print(f"Witaj {name}\n")
            # print("podaj numer pokoju do ktorego chcesz sie przylaczyc 1-5")
            room = input(
                "podaj numer pokoju do ktorego chcesz sie przylaczyc 1-5\n")
            send_data(socket_des, room)
        elif decoded == "000":
            # print(f"czesc {name}\n")
            name = input("Podaj Nick\n")
            send_data(socket_des, name)
        elif decoded[0]==3:
            print(f"witaj w pokoju: {room}\n")
            print(decoded)
            if decoded[1] == "0":
                my_id = decoded[2]
            else:
                my_id = decoded[1:]
            print(my_id)
            hang_dic[my_id] = 0
            # name=input("Podaj Nick\n")
            # send_data(socket_des,name)
            # go=False
            # send_data(socket_des,"agata")
        elif decoded == "400":
            print("Pokoj juz pelny/n 1-sprobuj pozniej/n 2 -wybierz inny numer")
            # send_data(socket_des,"agata")
        elif "50" in decoded:
            players=decoded[2]
            print(f"liczba graczy {players}")
            wait = input("chcesz czekac za kolejnym graczem - 0 , gramy - 1\n")
            send_data(socket_des, wait)
            # receive_ids(socket_des)
        elif decoded == "777":
            go = False
        elif decoded == "XXX":
            go = False

        elif "60" in decoded:
            # nplayers=decoded[1] # przesylamy ogolna liczbe graczy
            print("zaczynamy gre")
            receive_word(socket_des)
            v = ""
            while v != "/":
                v = input("podaj litere lub / gdy konczysz\n")

                send_data(socket_des, v)
                receive_game(socket_des)
            # while True:
            #	receive_game(socket_des);
            go = False


try:
    ip_addr, port = init()
    connect(ip_addr, port)
    receive_data2(s)
# time.sleep(2)
# send_data(s)
# receive_data2(s)
    f = 1
    signal.signal(signal.SIGINT, signal_handler)
    while f == 1:
        f = int(input("podaj 1-dalej,0-koniec"))
    '''if h==0:
		send_data(s,"666")
		f=0'''
    s.close()
except KeyboardInterrupt:  # obsluga ctr+C
    s.close()
    sys.exit(0)
