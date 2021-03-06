import math

import socket
import threading
import sys
import time
import signal
import warnings
import pygame as pg
import random
warnings.simplefilter("ignore")
s = socket.socket()
connected = False

ids = []

booked_room = [0, False]

WIDTH, HEIGHT = 800, 600
pg.init()
bg = pg.image.load("bg-photo.png")
bg = pg.transform.scale(bg, (800, 600))
screen = pg.display.set_mode((800, 600))
screen.blit(bg, (0, 0))

pg.display.set_caption("Hangman")

colors = {"black": (0, 0, 0), "darkgray": (70, 70, 70), "gray": (128, 128, 128), "lightgray": (200, 200, 200),
          "white": (255, 255, 255), "red": (255, 0, 0),
          "darkred": (128, 0, 0), "green": (0, 255, 0), "darkgreen": (0, 128, 0), "blue": (0, 0, 255),
          "navy": (0, 0, 128), "darkblue": (0, 0, 128),
          "yellow": (255, 255, 0), "gold": (255, 215, 0), "orange": (255, 165, 0), "lilac": (229, 204, 255),
          "lightblue": (135, 206, 250), "teal": (0, 128, 128),
          "cyan": (0, 255, 255), "purple": (150, 0, 150), "pink": (238, 130, 238), "brown": (139, 69, 19),
          "lightbrown": (222, 184, 135), "lightgreen": (144, 238, 144),
          "turquoise": (64, 224, 208), "beige": (245, 245, 220), "honeydew": (240, 255, 240),
          "lavender": (230, 230, 250), "crimson": (220, 20, 60)}


FONT = pg.font.Font(None, 32)

nick_init = FONT.render('Nick:', False, (0, 0, 0))

hangman = 0
players = 0
vote = [False, False, 'x']
hang_dic = {}
id_nick = {}
my_id = ""
letter_realtime = ['', False]
score = 0
word1 = ""
end = False
exit_alert = False
get_another = True
game_started = False
thread1 = True
current_screen = True

player_hang_score_arr = []


def init():
    global exit_alert
    try:
        with open('configclient.txt', 'r') as reader:
            ip_add = reader.readline().rstrip()
            p = int(reader.readline().rstrip())
        return ip_add, p
    except:
        exit_alert = True
        return '-', '-'

def connect(ip_addr, port):
    global connected
    global s
    global thread1
    global exit_alert
    if not connected:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((ip_addr, port))
            print("connected")
            connected = True
            if thread1:
                t.start()
                thread1 = False

        except:
            exit_alert = True


def send_data(socket_des, message):
    global exit_alert
    try:
        message += "$$"
        socket_des.send(message.encode())
    except:
        exit_alert = True


def receive_word(socket_des):
    global word
    global word1
    global ids
    global hang_dic
    global exit_alert
    get = ''
    while len(get) < 138:
        try:
            dataFrom = socket_des.recv(138)
            get_part = dataFrom.decode()
            get += get_part
        except:
            exit_alert = True
    get = get.replace(".", "")
    get += "$"
    print(get)
    i = get.find(";")
    word = get[0:i]
    word1 = word

    s = ""
    turn = 0
    helper = ""
    for sign in get[i + 1::]:

        if turn == 0 and sign != ";" and sign != "$":  # pierwszy id
            s += sign
        elif sign == ";" and turn == 0:
            hang_dic[s] = 0
            ids.append(s)
            helper = s
            s = ""
            turn = 1
        elif turn == 1 and sign != ";" and sign != "$":  # nick
            s += sign
        elif sign == ";" and turn == 1:
            id_nick[helper] = s
            helper = ""
            s = ""
            turn = 0
        elif sign == "$":
            print(word)
            print(ids)
            print(id_nick)
            return


def receive_game():
    global score
    global hangman
    global player_hang_score_arr
    global choosen
    global word1
    global my_id
    global exit_alert
    global s
    socket_des = s
    while True:
        buffer_size = 1024
        read = 0
        data_final = ""
        while read < 6:
            try:
                data = socket_des.recv(buffer_size)
                data = data.decode()
                read += len(data)
                data_final += data
            except:
                exit_alert = True
        if len(data_final) % 6 == 0:
            final_list = []
            for _ in range(int(len(data_final) / 6)):
                final_list.append(data_final[:4])
                data_final = data_final[6:]
            print(final_list)
            for f in final_list:
                if "!" in f:   #jezeli otrzyma informacje o błędzie
                    exit_alert = True
                elif f[0] == "7": #serwer potwierdza koniec gry
                    return
                elif f[0] == "8":  # hangman
                    if f[2] == ".":
                        for i in range(len(player_hang_score_arr)):
                            if player_hang_score_arr[i][0] == f[1]:
                                player_hang_score_arr[i][1] = int(f[3])
                    else:
                        for i in range(len(player_hang_score_arr)):
                            if player_hang_score_arr[i][0] == f[1:2]:
                                player_hang_score_arr[i][1] = int(f[3])
                    if f[1:2] == my_id or f[1] == my_id:
                        hangman = int(f[3])
                elif f[0] == "9":  # literka
                    if f[2] == ".":
                        choosen.append(f[1])
                        for i in range(len(player_hang_score_arr)):
                            if player_hang_score_arr[i][0] == f[2]:
                                player_hang_score_arr[i][2] += word1.count(f[1])

                        word1 = word1.replace(f[1], "")
                        if f[2] == my_id:
                            score += word.count(f[1])

                    else:
                        choosen.append(f[1])
                        for i in range(len(player_hang_score_arr)):
                            if player_hang_score_arr[i][0] == f[2:3]:
                                player_hang_score_arr[i][2] += word1.count(f[1])

                        word1 = word1.replace(f[1], "")
                        if (f[2:3] == my_id):
                            score += word.count(f[1])

        else:
            print("errr")
            print(data_final)
            print("\n")



def receive_data():
    global s
    global connected
    global players
    global letter_realtime
    global player_hang_score_arr
    global my_id
    global exit_alert
    global get_another
    global game_started
    global current_screen
    go = True
    decoded = ""
    while current_screen:
        while connected:
            while decoded.find("$$") == -1:
                try:
                    dataFromServer = s.recv(5)
                    decoded = dataFromServer.decode()
                except:
                    sys.exit(0)
            decoded = decoded[:-2]
            if decoded == "200":
                print("nick zajety podaj inny")
                nick.rand_set()
                send_data(s, nick.nick)

            elif decoded == "000":
                if nick.nick == "":
                    nick.empty_nick()
                send_data(s, nick.nick)

            elif decoded == "100": # pytanie o pokoj
                send_data(s, str(booked_room[0]))
                print(booked_room[0])
            elif decoded == "400":  #pelny pokoj
                get_another = False
                while not get_another:
                    pass
                send_data(s, str(booked_room[0]))
                get_another = True
            elif decoded[0] == "3": # wejscie do pokoju
                print(booked_room[0])
                booked_room[1] = True
                if decoded[1] == "0":
                    my_id = decoded[2]
                else:
                    my_id = decoded[1:]
                hang_dic[my_id] = 0
            elif "50" in decoded: # glosowanie
                vote[2] = '2'
                vote[0] = False
                players = int(decoded[2])
                while not vote[0]:
                    pass
                send_data(s, vote[2])
            elif decoded == "777":
                go = False
            elif "!" in decoded:
                exit_alert = True
                go = False
            elif "*" in decoded: # czeka za dlugo
                exit_alert = True
                go = False
            elif decoded == "XXX":  # gracz czwarty konczy gre bo pozostala trojka juz zaczela
                game_started = True
                connected = False
                go = False

            elif "60" in decoded: # poczatek gry
                receive_word(s)
                for key, value in hang_dic.items():
                    temp = [key, value, 0]
                    player_hang_score_arr.append(temp)
                vote[1] = True
                t1.start()
                while True:
                    if letter_realtime[1]:  # jezeli jest zgloszona literka do wysylanie
                        send_data(s, letter_realtime[0])  # wysyla literke
                        letter_realtime[1] = False  # brak zgloszonej literki

class Button_server():
    def __init__(self, color, x, y, width, height, text, id):
        self.color = color
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.text = text
        self.id = id

        self.players = 0
        self.status = 0

    def draw(self, screen, outline=None):
        if outline:
            pg.draw.rect(screen, outline, (self.x - 2, self.y - 2, self.width + 4, self.height + 4), 0)

        pg.draw.rect(screen, self.color, (self.x, self.y, self.width, self.height), 0)

        if self.text != '':
            font = pg.font.SysFont('comicsans', 30)
            text = font.render(self.text, 1, (0, 0, 0))
            screen.blit(text, (self.x + (self.width / 2 - text.get_width() / 2), self.y + 50))

    def isOver(self, pos):
        if pos[0] > self.x and pos[0] < self.x + self.width:
            if pos[1] > self.y and pos[1] < self.y + self.height:
                return True

        return False


class Button():
    def __init__(self, color, x, y, width, height, text=''):
        self.color = color
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.text = text

    def draw(self, screen, outline=None):
        if outline:
            pg.draw.rect(screen, outline, (self.x - 2, self.y - 2, self.width + 4, self.height + 4), 0)

        pg.draw.rect(screen, self.color, (self.x, self.y, self.width, self.height), 0)

        if self.text != '':
            font = pg.font.SysFont('comicsans', 30)
            text = font.render(self.text, 1, (0, 0, 0))
            screen.blit(text, (self.x + (self.width / 2 - text.get_width() / 2), self.y + 15))

    def isOver(self, pos):
        if pos[0] > self.x and pos[0] < self.x + self.width:
            if pos[1] > self.y and pos[1] < self.y + self.height:
                return True

        return False


class InputBox:
    def __init__(self, x, y, w, h, text=''):
        self.rect = pg.Rect(x, y, w, h)
        self.color = pg.Color(colors["white"])
        self.text = text
        self.txt_surface = FONT.render(text, True, self.color)
        self.active = False
        self.nick = ''

    def handle_event(self, event):
        if event.type == pg.MOUSEBUTTONDOWN:
            if self.rect.collidepoint(event.pos):
                self.active = not self.active
            else:
                self.active = False
            self.color = pg.Color(colors["white"]) if self.active else pg.Color(colors["black"])
        if event.type == pg.KEYDOWN:
            if self.active:
                if event.key == pg.K_RETURN:
                    self.nick = self.text

                elif event.key == pg.K_BACKSPACE:
                    self.text = self.text[:-1]
                    self.nick = self.text
                else:
                    self.text += event.unicode
                    self.nick = self.text
                self.txt_surface = FONT.render(self.text, True, self.color)

    def update(self):
        width = max(200, self.txt_surface.get_width() + 10)
        self.rect.w = width

    def draw(self, screen):
        screen.blit(self.txt_surface, (self.rect.x + 5, self.rect.y + 5))
        pg.draw.rect(screen, self.color, self.rect, 2)

    def rand_set(self):
        anonim_num = random.randint(0,10)
        self.text = self.text + str(anonim_num)
        self.nick = self.text

    def empty_nick(self):
        self.text = "Anonim"
        self.nick = self.text


# obiekty GUI
serversButtons = []
serversButtons.append(Button_server(colors["gray"], 130, 150, 130, 130, "Room 1", 1))
serversButtons.append(Button_server(colors["gray"], 330, 150, 130, 130, "Room 2", 2))
serversButtons.append(Button_server(colors["gray"], 530, 150, 130, 130, "Room 3", 3))
exit_button = Button(colors["lightgreen"], 600, 500, 130, 60, "Exit")
vote_button = Button(colors["lightgreen"], 50, 180, 130, 60, "Vote")
wait_button = Button(colors["lightgreen"], 220, 180, 130, 60, "Wait")
exit3_button = Button(colors["lightgreen"], 185, 300, 130, 60, "Exit")
nick = InputBox(100, 500, 140, 32)

word = ""
choosen = []  # wybrane literki
images = []  # zdjecia hangmanow

for i in range(7):
    image = pg.image.load("images/images/hangman" + str(i) + ".png")
    images.append(image)

RADIUS = 15
GAP = 10
letters = []
startx = round((WIDTH - (RADIUS * 2 + GAP) * 6) / 2)
starty = 400
for i in range(26):
    x = startx + GAP * 2 + ((RADIUS * 2 + GAP) * (i % 6))
    y = starty + ((i // 6) * (GAP + RADIUS * 2))
    letters.append([x, y, chr(65 + i), True])

LETTER_FONT = pg.font.SysFont('comicsans', 25)
WORD_FONT = pg.font.SysFont('comicsans', 40)
RESULT_FONT = pg.font.SysFont('comicsans', 60)

t = threading.Thread(target=receive_data)
t.daemon = True

t1 = threading.Thread(target=receive_game)
t1.daemon = True


def menu():
    global screen
    global booked_room
    global get_another
    run = True
    while run:
        for e in pg.event.get():  # pętla po zdarzeniach - klkniecia, naciścnięcia
            if e.type == pg.QUIT:
                run = False
            if e.type == pg.MOUSEMOTION:  # najechanie na przycisk
                for room in serversButtons:
                    if room.isOver(pg.mouse.get_pos()):
                        room.color = colors["lightgray"]
                    else:
                        room.color = colors["gray"]
            if e.type == pg.MOUSEBUTTONDOWN:
                if e.button == 1:
                    for room in serversButtons:
                        if room.isOver(pg.mouse.get_pos()) and room.status == 0 and room.players < 5:
                            booked_room[0] = room.id
                            connect(ip_addr, port)
                            if not get_another:
                                get_another = True
                            pg.time.delay(500)
                            if booked_room[1]:
                                booked_room[1] = False
                                poczekalnia()  # oczekiwianie na pozostalych graczy
                                screen = pg.display.set_mode((800, 600))
                    if exit_button.isOver(pg.mouse.get_pos()):
                        s.close()   #gdy exit zamknij deskryptor
                        run = False
                        pg.quit()
                        quit()
            nick.handle_event(e)  # aktualizowanie wpisywania nicku
        nick.update()
        screen.blit(bg, (0, 0))
        nick.draw(screen)

        for servbuttons in serversButtons:
            servbuttons.draw(screen)

        exit_button.draw(screen)
        screen.blit(nick_init, (25, 505))

        if exit_alert:
            comments(2)
        pg.display.update()


def poczekalnia():
    global screen
    global players
    global vote
    global connected
    global game_started
    screen = pg.display.set_mode((400, 300))
    p_width = 400
    p_height = 300
    run = True
    while run:
        screen.fill(colors["white"])
        text = WORD_FONT.render("Waiting room", 1, colors["black"])
        screen.blit(text, (round(p_width / 2) - 90, 50))

        text = LETTER_FONT.render(str(players) + " / 5 PLAYERS", 1, colors["red"])
        screen.blit(text, (round(p_width / 2) - 50, 100))

        if players > 2 and players < 5 and not vote[0] and vote[2] == '2':
            vote_button.draw(screen)
            wait_button.draw(screen)


        for e in pg.event.get():
            if e.type == pg.QUIT:
                run = False
                pg.QUIT
                s.close()
                quit()
            if e.type == pg.MOUSEBUTTONDOWN:
                if e.button == 1:
                    if vote_button.isOver(pg.mouse.get_pos()) and players > 2 and players != 5 and vote[2] == '2':
                        # + 1 osoba do ankiety
                        vote[2] = '1'
                        vote[0] = True
                    if wait_button.isOver(pg.mouse.get_pos()) and players > 2 and players != 5 and vote[2] == '2':
                        vote[2] = '0'
                        vote[0] = True
        if vote[1]:
            game()
        if exit_alert:
            comments(2)
        if game_started:
            run = False
            s.close()
            game_started = False
            comments(3)
        pg.display.update()


def game():
    global screen
    global hangman
    global letter_realtime, end
    screen = pg.display.set_mode((800, 600))
    screen = pg.display.set_mode((screen.get_width(), screen.get_height()))  # , pg.FULLSCREEN)
    screen.fill(colors["white"])
    run = True
    while run:
        screen.fill(colors["white"])
        text = LETTER_FONT.render(nick.nick, 1, colors["black"])
        screen.blit(text, (round(WIDTH / 2) - 50, 350))
        screen.blit(images[hangman], (round(WIDTH / 2) - 50, 100))
        for letter in letters:
            x, y, ltr, vis = letter
            if vis and hangman < 6:
                pg.draw.circle(screen, colors["black"], (x, y), RADIUS, 3)
                text = LETTER_FONT.render(ltr, 1, colors["black"])
                screen.blit(text, (x - text.get_width() / 2, y - text.get_height() / 2))

        display_word = ""
        for letter in word:  # wyswietlanie hasla
            if letter in choosen:
                display_word += letter + " "
            else:
                display_word += "_ "
        text = WORD_FONT.render(display_word, 1, colors["black"])
        screen.blit(text, (round(WIDTH / 2) - 50, 50))

        # gracze wyswietlnie i ich bledy
        text = LETTER_FONT.render("Players fails:", 1, colors["black"])
        screen.blit(text, (50, 300))
        cord_y = 370
        for i in range(len(player_hang_score_arr)):
            text = LETTER_FONT.render(id_nick[str(player_hang_score_arr[i][0])] + ":   " + str(player_hang_score_arr[i][1]) + " / 6", 1,
                                      colors["black"])
            screen.blit(text, (50, cord_y))
            cord_y += 50
        for e in pg.event.get():
            if e.type == pg.QUIT:
                pg.QUIT
                send_data(s, "#")
                s.close()
                quit()
            if e.type == pg.MOUSEBUTTONDOWN:
                m_x, m_y = pg.mouse.get_pos()
                for letter in letters:
                    x, y, ltr, vis = letter
                    distance = math.sqrt((x - m_x) ** 2 + (y - m_y) ** 2)
                    if distance < RADIUS and letter[3] == True and hangman < 6:  # jeli literka jest widzialna i mozesz jeszcze grac
                        letter[3] = False  # literka jest niewidoczna
                        letter_realtime[0] = ltr  # moze wsylac literke
                        letter_realtime[1] = True
        counter = 0
        for player in player_hang_score_arr:
            id, hang, score = player
            if hang == 6:
                counter += 1
        if len(player_hang_score_arr) - counter < 2 or word1 == "":  # skonczyla cale slowo sie uzupelnilo
            end = True
        if end:
            reset_game()
            run = False
            send_data(s, "#")
            comments(1)  # pokazuje okienko z wynikiem
        if exit_alert:
            comments(2)

        pg.display.update()


def comments(option):
    global screen
    global players
    global player_hang_score_arr
    screen = pg.display.set_mode((500, 400))
    p_width = 400
    p_height = 300
    run = True
    if option == 1:
        screen.fill(colors["white"])
        text = WORD_FONT.render("GAME RESULTS:", 1, colors["black"])
        screen.blit(text, (round(p_width / 2) - 90, 40))
        player_hang_score_arr = sort_players(player_hang_score_arr)
        cord_y = 90
        for i in range(len(player_hang_score_arr)):
            if player_hang_score_arr[int(i)][1] == 6:
                text = LETTER_FONT.render(
                    str(id_nick[str(player_hang_score_arr[i][0])]) + ":   " + str(player_hang_score_arr[i][2]), 1,
                    colors["red"])
                screen.blit(text, (50, cord_y))
            else:
                text = LETTER_FONT.render(
                    str(id_nick[str(player_hang_score_arr[i][0])]) + ":   " + str(player_hang_score_arr[i][2]), 1,
                    colors["black"])
                screen.blit(text, (50, cord_y))

            cord_y += 40

        exit3_button.draw(screen)

        while run:
            for e in pg.event.get():
                if e.type == pg.QUIT:
                    s.close()
                    run = False
                    pg.QUIT
                    quit()
                if e.type == pg.MOUSEBUTTONDOWN:
                    if e.button == 1:
                        if exit3_button.isOver(pg.mouse.get_pos()):
                            s.close()
                            run = False
                            pg.QUIT
                            quit()
            pg.display.update()
    elif option == 2:
        while run:
            screen.fill(colors["white"])
            text = WORD_FONT.render("Disconnected from sever", 1, colors["black"])
            screen.blit(text, (round(p_width / 2) - 120, 40))
            text = WORD_FONT.render("Pardon", 1, colors["black"])
            screen.blit(text, (round(p_width / 2)-10, 100))

            exit3_button.draw(screen)

            for e in pg.event.get():
                if e.type == pg.QUIT:
                    s.close()
                    run = False
                    pg.QUIT
                    quit()
                if e.type == pg.MOUSEBUTTONDOWN:
                    if e.button == 1:
                        if exit3_button.isOver(pg.mouse.get_pos()):
                            s.close()
                            run = False
                            pg.QUIT
                            quit()
            pg.display.update()
    elif option == 3:
        while run:
            screen.fill(colors["white"])
            text = WORD_FONT.render("The game", 1, colors["black"])
            screen.blit(text, (round(p_width / 2) -30, 40))
            text = WORD_FONT.render("has already started", 1, colors["black"])
            screen.blit(text, (round(p_width / 2) - 90, 100))

            exit3_button.draw(screen)

            for e in pg.event.get():
                if e.type == pg.QUIT:
                    run = False
                if e.type == pg.MOUSEBUTTONDOWN:
                    if e.button == 1:
                        if exit3_button.isOver(pg.mouse.get_pos()):
                            run = False
            pg.display.update()




def sort_players(tab):
    tab_full_hang = []
    tab_winners = []
    for i in range(len(tab)):
        if tab[i][1]==6:
            tab_full_hang.append(tab[i])
        else:
            tab_winners.append(tab[i])
    tab_winners = sorted(tab_winners,key=lambda x: x[2], reverse=True)
    tab_full_hang = sorted(tab_full_hang,key=lambda x: x[2], reverse=True)

    return tab_winners + tab_full_hang

def reset_game():
    global hangman, choosen
    hangman, choosen = 0, []
    for let in letters:
        let[3] = True



try:
    ip_addr, port = init()
    menu()
except KeyboardInterrupt:  # obsluga ctr+C
    s.close()
    sys.exit(0)