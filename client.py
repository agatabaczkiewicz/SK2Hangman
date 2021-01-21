import math

import socket
import threading
import sys
import time
import signal

import pygame as pg

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

roomStatus = ["Oczekuje", "Trwa gra"]

FONT = pg.font.Font(None, 32)

nick_init = FONT.render('Nick:', False, (0, 0, 0))

hangman = 0
players = 0
vote = [False, False]
hang_dic = {}
id_nick = {}
my_id = ""
letter_realtime = ['', False]
score = 0
word1 = ""
end  = False

player_hang_score_arr = []

def init():
    with open('configclient.txt', 'r') as reader:
        ip_add=reader.readline().rstrip()
        p=int(reader.readline().rstrip())

    return ip_add,p


def signal_handler(signal, frame):
    s.close()
    print("sss")
    sys.exit(0)


def connect(ip_addr,port):
    global connected
    global s
    if connected == False:
        try:
            s=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
            s.connect((ip_addr,port))
            print("connected")
            connected = True
            t.start()

        except:
            print("sth goes wrong")
            sys.exit(1)

def send_data(socket_des, message):
    try:
        message += "$$"
        socket_des.send(message.encode())
    except:
        print("send goes wrong")


def receive_word(socket_des):
    global word
    global word1
    global ids
    global hang_dic
    get = ''
    while len(get) < 138:
        dataFrom = socket_des.recv(138)
        get_part = dataFrom.decode()
        get += get_part
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
        elif turn == 1 and sign != ";" and sign != "$":  # teraz nick
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
    global s
    socket_des = s
    while True:
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
            print(my_id)
            for f in final_list:
                if f[0] == "8":  # hangman
                    if f[2] == ".":
                        player_hang_score_arr[int(f[1])][1] = int(f[3])
                    else:
                        player_hang_score_arr[int(f[1:2])][1] = int(f[3])
                    if f[1:2] == my_id or f[1] == my_id:
                        hangman = int(f[3])
                elif f[0] == "9": # literka
                    if f[2] == ".":
                        choosen.append(f[1])
                        player_hang_score_arr[int(f[1])][2] += word1.count(f[1])
                        word1 = word1.replace(f[1], "")
                        if (f[2] == my_id):
                            score += word.count(f[1])

                    else:
                        choosen.append(f[1])
                        player_hang_score_arr[int(f[2:3])][2] += word1.count(f[1])
                        word1 = word1.replace(f[1], "")
                        if (f[2:3] == my_id):
                            score += word.count(f[1])
                            # print('my score' {score}\n")

        else:
            print("errr")
            print(data_final)
            print("\n")
            # return [("er", "01")]  # bad receive code


def receive_data():
    global s
    global connected
    global players
    global letter_realtime
    global player_hang_score_arr
    global my_id
    go = True
    decoded = ""
    while connected:
        while decoded.find("$$") == -1:
            dataFromServer = s.recv(5);
            decoded = dataFromServer.decode()
        decoded = decoded[:-2]
        if decoded == "200":
            print("nick zajety podaj inny\n")
            pg.time.delay(5000)
            send_data(s, nick.nick)

        elif decoded == "000":
            send_data(s, nick.nick)

        elif decoded == "100":
            print("podaj numer pokoju do ktorego chcesz sie przylaczyc 1-3")
            send_data(s, str(booked_room[0]))
            print(booked_room[0])
            booked_room[1] = True
        elif decoded == "400":
            print("Pokoj juz pelny")
        elif decoded[0] == "3":
            print(f"witaj w pok\n")
            print(decoded)
            if decoded[1] == "0":
                my_id = decoded[2]
            else:
                my_id = decoded[1:]
            print(my_id)
            hang_dic[my_id] = 0
        elif "50" in decoded:
            players = int(decoded[2])
            print(f"liczba graczy {players}")
            print("chcesz czekac za kolejnym graczem - 0 , gramy - 1\n")
            while not vote[0]:
                pass
            send_data(s, '1')
            # receive_ids(socket_des)
        elif decoded == "777":
            go = False
        elif decoded == "XXX":
            go = False

        elif "60" in decoded:
            # nplayers=decoded[1] # przesylamy ogolna liczbe graczy
            print("zaczynamy gre")
            receive_word(s)
            for key, value in hang_dic.items():
                temp = [key, value, 0]
                player_hang_score_arr.append(temp)
            print(player_hang_score_arr[0][1])
            vote[1] = True
            t1.start()
            while True:
                if letter_realtime[1]:
                    send_data(s, letter_realtime[0])
                    print("wyslalo literke")
                    letter_realtime[1] = False
            # print("Wchodzi")
            # receive_game(s)
            # print("WESZLO do receve")
            # while True:
            #	receive_game(socket_des);
            # go = False


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
        # Call this method to draw the button on the screen
        if outline:
            pg.draw.rect(screen, outline, (self.x - 2, self.y - 2, self.width + 4, self.height + 4), 0)

        pg.draw.rect(screen, self.color, (self.x, self.y, self.width, self.height), 0)

        if self.text != '':
            font = pg.font.SysFont('comicsans', 30)
            font2 = pg.font.SysFont('comicsans', 20)
            text = font.render(self.text, 1, (0, 0, 0))
            screen.blit(text, (self.x + (self.width / 2 - text.get_width() / 2), self.y + 15))


            text = font2.render(roomStatus[self.status], 1, (0, 0, 0))
            screen.blit(text, (self.x + (self.width / 2 - text.get_width() / 2), self.y + 80))

    def isOver(self, pos):
        # Pos is the mouse position or a tuple of (x,y) coordinates
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
        # Call this method to draw the button on the screen
        if outline:
            pg.draw.rect(screen, outline, (self.x - 2, self.y - 2, self.width + 4, self.height + 4), 0)

        pg.draw.rect(screen, self.color, (self.x, self.y, self.width, self.height), 0)

        if self.text != '':
            font = pg.font.SysFont('comicsans', 30)
            text = font.render(self.text, 1, (0, 0, 0))
            screen.blit(text, (self.x + (self.width / 2 - text.get_width() / 2), self.y + 15))

    def isOver(self, pos):
        # Pos is the mouse position or a tuple of (x,y) coordinates
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
            # If the user clicked on the input_box rect.
            if self.rect.collidepoint(event.pos):
                # Toggle the active variable.
                self.active = not self.active
            else:
                self.active = False
            # Change the current color of the input box.
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
                # Re-render the text.
                self.txt_surface = FONT.render(self.text, True, self.color)

    def update(self):
        # Resize the box if the text is too long.
        width = max(200, self.txt_surface.get_width() + 10)
        self.rect.w = width

    def draw(self, screen):
        # Blit the text.
        screen.blit(self.txt_surface, (self.rect.x + 5, self.rect.y + 5))
        # Blit the rect.
        pg.draw.rect(screen, self.color, self.rect, 2)


serversButtons = []
serversButtons.append(Button_server(colors["gray"], 130, 150, 130, 130, "Room 1", 1))
serversButtons.append(Button_server(colors["gray"], 330, 150, 130, 130, "Room 2", 2))
serversButtons.append(Button_server(colors["gray"], 530, 150, 130, 130, "Room 3", 3))
exit_button = Button(colors["lightgreen"], 600, 500, 130, 60, "Exit")
vote_button = Button(colors["lightgreen"], 50, 180, 130, 60, "Vote")
exit2_button = Button(colors["lightgreen"], 220, 180, 130, 60, "Exit")
exit3_button = Button(colors["lightgreen"], 300, 300, 130, 60, "Exit")
poczekalnia_button = Button(colors["lightgreen"], 70, 300, 130, 60, "Play again")
nick = InputBox(100, 500, 140, 32)

word = ""
choosen = []
images = []

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
    run = True
    while run:
        for e in pg.event.get():
            if e.type == pg.QUIT:
                run = False
                # pg.quit()
                # quit()
            if e.type == pg.MOUSEMOTION:
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
                            connect(ip_addr,port)
                            pg.time.delay(500);
                            if booked_room[1] == True:
                                poczekalnia()

                            screen = pg.display.set_mode((800, 600))
                    if exit_button.isOver(pg.mouse.get_pos()):
                        run = False
                        # pg.quit()
                        # quit()
            nick.handle_event(e)
        nick.update()
        screen.blit(bg, (0, 0))
        nick.draw(screen)

        for servbuttons in serversButtons:
            servbuttons.draw(screen)

        exit_button.draw(screen)
        screen.blit(nick_init, (25, 505))

        # pg.display.flip()
        pg.display.update()

def poczekalnia():
    global screen
    global players
    global vote
    screen = pg.display.set_mode((400, 300))
    p_width = 400
    p_height = 300
    run = True
    while run:
        screen.fill(colors["white"])
        text = WORD_FONT.render("POCZEKALNIA", 1, colors["black"])
        screen.blit(text, (round(p_width / 2) - 90, 50))

        text = LETTER_FONT.render(str(players) + " / 5 PLAYERS", 1, colors["red"])
        screen.blit(text, (round(p_width / 2) - 50, 100))
        exit2_button.draw(screen)
        if players > 2 and players < 5:
            vote_button.draw(screen)

        for e in pg.event.get():
            if e.type == pg.QUIT:
                run = False
                pg.QUIT
                quit()
            if e.type == pg.MOUSEBUTTONDOWN:
                if e.button == 1:
                    if vote_button.isOver(pg.mouse.get_pos()) and players > 2 and players != 5:
                        # + 1 osoba do ankiety
                        vote[0] = True
                        while not vote[1]:
                            pass
                        game()
                        #run = False
                    if exit2_button.isOver(pg.mouse.get_pos()):
                        run = False

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
        for letter in word:
            if letter in choosen:
                display_word += letter + " "
            else:
                display_word += "_ "
        text = WORD_FONT.render(display_word, 1, colors["black"])
        screen.blit(text, (round(WIDTH / 2) - 50, 50))

        # gracze
        text = LETTER_FONT.render("Players fails:", 1, colors["black"])
        screen.blit(text, (50, 300))
        cord_y=370
        for i in id_nick:
            text = LETTER_FONT.render(str(id_nick[i]) + ":   " + str(player_hang_score_arr[int(i)][1]) + " / 6", 1, colors["black"])
            screen.blit(text, (50, cord_y))
            cord_y += 50
        for e in pg.event.get():
            if e.type == pg.QUIT:
                pg.QUIT
                quit()
            if e.type == pg.MOUSEBUTTONDOWN:
                m_x, m_y = pg.mouse.get_pos()
                for letter in letters:
                    x, y, ltr, vis = letter
                    distance = math.sqrt((x - m_x) ** 2 + (y - m_y) ** 2)
                    if distance < RADIUS and letter[3] == True and hangman < 6:
                        letter[3] = False
                        letter_realtime[0] = ltr
                        letter_realtime[1] = True
        counter = 0
        for player in player_hang_score_arr:
            id, hang, score = player
            if hang == 6:
                counter += 1
        #print("Wyswietlone slowo  " + display_word)
        #print("Znikaace slowo  " + word1)
        if len(player_hang_score_arr) - counter < 2 or word1 == "":
            end = True

        if end:
            reset_game()
            run = False
            results()

        pg.display.update()




def results():
    global screen
    global players
    global player_hang_score_arr
    screen = pg.display.set_mode((500, 400))
    p_width = 400
    p_height = 300
    run = True
    while run:
        screen.fill(colors["white"])
        text = WORD_FONT.render("GAME RESULTS:", 1, colors["black"])
        screen.blit(text, (round(p_width / 2) - 90, 40))
        cord_y = 90
        for i in id_nick:
            text = LETTER_FONT.render(str(id_nick[i]) +":   " + str(player_hang_score_arr[int(i)][2]), 1, colors["black"])
            screen.blit(text, (50, cord_y))
            cord_y += 40
        exit3_button.draw(screen)

        poczekalnia_button.draw(screen)

        for e in pg.event.get():
            if e.type == pg.QUIT:
                run = False
                pg.QUIT
                quit()
            if e.type == pg.MOUSEBUTTONDOWN:
                if e.button == 1:
                    if poczekalnia_button.isOver(pg.mouse.get_pos()):
                        poczekalnia()
                        run = False
                    if exit3_button.isOver(pg.mouse.get_pos()):
                        run = False

        pg.display.update()


def reset_game():
    global hangman, choosen
    hangman, choosen = 0, []
    for let in letters:
        let[3] = True


try:
    ip_addr, port = init()
    menu()
    # game()
    # results()
except KeyboardInterrupt:  # obsluga ctr+C
    s.close()
    sys.exit(0)




