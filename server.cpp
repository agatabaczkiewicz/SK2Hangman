#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <signal.h>
#include <thread>
#include <stdlib.h>
#include <mutex>
#include <condition_variable>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <queue>
#include <algorithm>
#include <future>
#include <chrono>


using namespace std;
using namespace std::chrono;
vector<string> nicks;   //vektor nickow i idikow graczy
mutex mutex_players;    //mutex zamykany gdy nowy gracz laczy sie z serwerem
mutex mutex_games[3];    //mutex dla kazdego pokoju
condition_variable wait_for_others[3];   //conditional variable dla kazdego pokoju
thread threads[15]; //watek dla kazdego gracza
int rooms[3][5];	//deskryptory wszystkich graczy
int ids[3][5];		//id_wszytskich graczy
int id = 0;
int notyou[3]{0, 0, 0};  //dla kazdego pokoju ma 1 gdy ktos sprzata na koncu rozgrywki
int ask[3]{0, 0, 0};     //przechowuje glosy graczy
int answer[3]{0, 0, 0};	 // przechowuje ile osob bierze udzial w ankiecie
int players[3]{0, 0, 0}; //liczba zalogowanych graczy w poszczegolnym pokoju
int game[3]{0, 0, 0};	 //czy gra zaczela sie
string words[3];             //przechowuje slowo dla danego pokoju
string done_letter[3]; //przechowuje wykorzytsane juz literki
int guess[3];            //przechowuje ile zostalo liter do ogdadniecia
int lost[3]{0, 0, 0};    //przechowuje ile graczy w danym pokoju ma juz pelnego wisielca
int wait[3]{0, 0, 0};    //ile graczy oddalo glos w ankiecie i czeka na decyzje
int x[3]{0, 0, 0};
int server_socket_descriptor;
int port;
string wordsfile;  
int number_of_words;
random_device rd;
mt19937_64 generator;
queue<int> id_queue;   //lista jednokierunkowa dostepnych idikow


struct thread_data_t
{
	int nr_deskryptora1; // deskryptor powiązany z danym wątkiem			 
	char data[3];		 // tablica do przesyładnia danych
	char data2[3];		 // tablica do przesyładnia danych
	int pokoj;			 // numer pokoju
	int numer;			 // identyfikator (ten, który jest przypisywany na samym początku)
	int point;			 // punkty zdobyte podczas gry
	int hangman;         //poziom wisielca

};

struct thread_data_t t_data[15];

bool check_nicks(string s)  //sprawdzam czy nick jest juz uzywany przez kogos innego
{
	for (string i : nicks)
	{
		if (i == s)
		{
			return false;
		}
	}
	nicks.push_back(s);
	return true;
}

void init() //serwer przygototwywuje sie
{
	for (int i = 0; i < 3; i++)
	{
		words[i] = "0";
		done_letter[i] = "3";
		for (int j = 0; j < 5; j++)
		{
			rooms[i][j] = 0;

			ids[i][j] = 99;
		}
	}
	for (int i = 0; i < 15; i++)
	{
		id_queue.push(i);
	}
	ifstream file("config.txt"); //odczyt konfiguracji z pliku port,nazwapliku ze slowami, ilosc slow)
	if (file.is_open())
	{
		string line, x;
		getline(file, line);
		x = line;
		port = stoi(x);
		getline(file, line);
		wordsfile = line;
		getline(file, line);
		x = line;
		number_of_words = stoi(x);
	}
	else
	{
		cout << "Brak pliku config";
		exit(1);
	}
	file.close();
}

int check_in_word(char *letter, string word, int room)  //sprwadzenie czy litera jest w slowie
{

	string let(letter, 3);
	let = let.substr(0, 1);
	string hash="#";
	string w = word;
	int score = 0;
	if(let.find(hash)!= string::npos){ //jezeli klient wysylal # tzn ze opuscil gre
		return -2;
	}
	while (w.find(let) != string::npos)
	{

		w = w.substr(w.find(let));
		w = w.substr(1);
		score++;
	}
	if (done_letter[room].find(let) != string::npos)
	{ //byla juz taka litera podana przez kogos innego
		return 0;
	}
	else if (score == 0) //brak litery w slowie 
	{
		return -1;
	}
	else
	{
		done_letter[room] += let;
		return score; // >0 czyli liczba wystapien podanej litery
	}
}

string get_word(string filename, int num_lines)  //losuj slowo
{
	string word;
	generator = std::mt19937_64(rd());
	uniform_int_distribution<int> distribution(1, num_lines);
	int random_line = distribution(generator);

	ifstream file(filename);
	if (file.is_open())
	{
		string line;
		for (int i = 0; i < random_line; i++)
		{
			getline(file, line);
		}
		word = line;
	}
	else
	{
		cout << "error";
		exit(0);
	}
	file.close();

	std::transform(word.begin(), word.end(), word.begin(), ::toupper);
	return word;
}

void count_letters(int pokoj, string word) //policz litery w slowie
{ 
	string w = word;
	string s = " ";
	int x = 0;
	while (w.find(s) != string::npos)
	{
		//cout<<w<<endl;
		w = w.substr(w.find(s));
		w = w.substr(1);
		x++;
	}
	guess[pokoj] = word.length() - x;
}

void clean_after_game(int nr_room)  //posprzataj po grze
{

	lost[nr_room] = 0;
	players[nr_room] = 0;
	ask[nr_room] = 0;
	answer[nr_room] = 0;
	game[nr_room] = 0;
	wait[nr_room] = 0;
	x[nr_room] = 0;
	guess[nr_room] = 0;
	done_letter[nr_room] = "0";
	words[nr_room] = "0";
	players[nr_room] = 0;
	for (int i = 0; i < 5; i++)
	{
		rooms[nr_room][i] = 0;

		if (ids[nr_room][i] != 99)
		{
			for (int nick = 1; nick < (int)nicks.size(); nick++)
			{
				if (nicks[nick] == to_string(ids[nr_room][i]))
				{
					nicks.erase(nicks.begin() + nick - 1, nicks.begin() + nick); //usuwa nick i idik z listy
				}
			}
			id_queue.push(ids[nr_room][i]); //idik powraca na liste dostepnych idikow
			ids[nr_room][i] = 99;
		}
		
		
	}
}

void handle_error_read(int deskryptor, int room, int id) //obsluz blad pojawiajacy sie przy read w trkacie rozgrywki
{
	cout << "blad";
	char buf[6] = {'!', '!', '!', '!', '!', '!'}; // problem serwera w odczytnaiu danych
	int n = 0;
	while (n < 6)
	{
		int send_int = write(deskryptor, &buf[n], strlen(buf) - n);
		n += send_int;
		if (send_int == -1)
		{
			write(deskryptor, "!", 1);
			printf("server has an unexpected problem\n");
			players[room] -= 1;
			close(deskryptor);
			
			return;
		}
	}
	players[room] -= 1;
	close(deskryptor);
	
}

void ThreadBehavior(thread_data_t *t_data)
{
	struct thread_data_t *th_data = (struct thread_data_t *)t_data;
	bool wakeup = true;
	int read_int, check, send_int, n;
	bool h;
	cout << (*th_data).nr_deskryptora1;
	unique_lock<mutex> lck(mutex_games[(*th_data).pokoj], defer_lock);
	notyou[(*th_data).pokoj]=0;
	lck.lock();

//	POCZEKALNIA
	while (wakeup)
	{
		// czekamy na drugiego i trzeciego gracza- minimum aby rozpoczac rozgrywke
		if (rooms[(*th_data).pokoj][0] * rooms[(*th_data).pokoj][1] * rooms[(*th_data).pokoj][2] == 0)
		{
			
			cv_status statusF2; 
			statusF2 = wait_for_others[(*th_data).pokoj].wait_until(lck, system_clock::now() + seconds(100)); //jezeli przez 100 sekund nie jest spelniony warunek trzech graczy 
			if (statusF2 == cv_status::timeout)                                                                  //to gracze opuszczaja pokoj a pokoj jest czyszczony
			{
				write((*th_data).nr_deskryptora1, "*$$", 3);
				cout << "za dlugo czekam" << endl;
				cout << "zamykam" << (*th_data).numer << endl;
				clean_after_game((*th_data).pokoj);
				close((*th_data).nr_deskryptora1);
				lck.unlock();
				threads[(*th_data).numer].detach();
				return;
			}

			
		}
		else
		{
			wakeup = false;
		}
	}

	//jest gracz trzeci
	if (rooms[(*th_data).pokoj][2] == (*th_data).nr_deskryptora1)
	{

		wait_for_others[(*th_data).pokoj].notify_all(); //powiadom pozostalych dwoch ze mozecie gracz
	}

	lck.unlock();

	h = true;

	if (players[(*th_data).pokoj] == 5)  
	{ //jest 5 graczy 

		while (wait[(*th_data).pokoj] != answer[(*th_data).pokoj])//czeka dopoki wszyscy nie odpowiedza na wczesnijesze pytanie
		{
			continue;
		}
		if (game[(*th_data).pokoj] == 0)
		{ //jezeli  ktos czeka na ciebie
		
			x[(*th_data).pokoj] = 1;
			wait_for_others[(*th_data).pokoj].notify_all(); //poinformuj ze jestes i mozemy grac
		}

		else if (game[(*th_data).pokoj] != 0) //pozostali gracze wola grac bez 5
		{
			cout<<"5 bye bye"<<endl;
			if (write((*th_data).nr_deskryptora1, "XXX$$", 5) < 0)
			{ //sorki inni juz graja
				cout << "write error" << endl;
				write((*th_data).nr_deskryptora1, "!$$", 3);
			}
			players[(*th_data).pokoj] -= 1;
			h=false;
			
		
			wait_for_others[(*th_data).pokoj].notify_all(); //powiadom tych, ktorzy czekaja az sobie pojedziesz, ze opuszczasz rozgrywke
			close((*th_data).nr_deskryptora1);
			threads[(*th_data).numer].detach();
			return;
		}
		

		h = false;
		game[(*th_data).pokoj] = 1;
	}

	

	else if (rooms[(*th_data).pokoj][3] == (*th_data).nr_deskryptora1) //gdy pojawi sie czwarty gracz
	{ 
		

		while (wait[(*th_data).pokoj] != answer[(*th_data).pokoj]) //czeka dopoki wszyscy nie odpowiedza na wczesnijesze pytanie
		{
			continue;
		}
		
		if(players[(*th_data).pokoj]==5 && game[(*th_data).pokoj] == 0){ //oprocz czwartego jest tez gracz piaty, a gracze nie zaczeli jeszcze gry
			x[(*th_data).pokoj] = 1;
			wait[(*th_data).pokoj]++;
			answer[(*th_data).pokoj]++;
			h=false;
		}
		else if (game[(*th_data).pokoj] == 0) //pozostala trojka graczy nie zaczela jeszcze gry i czekaja na czwartego, wiec czwarty pyta ich czy 
		{                                     //chca czekac za piatym, czy grac
			char buf[5];
			string mes = "50";								  //kod wiadomosci
			mes.append(to_string(players[(*th_data).pokoj])); //liczba graczy w pokoju
			mes.append("$$");
			strcpy(buf, mes.c_str());
			for (int i = 0; i < 4; i++)
			{
				if (rooms[(*th_data).pokoj][i] != 0)
				{
					if (write(rooms[(*th_data).pokoj][i], &buf, 5) < 0)
					{ //spytaj czy czekamy jeszcze za kims
						rooms[(*th_data).pokoj][i]=0;
						cout << "write error" << endl;
						write(rooms[(*th_data).pokoj][i], "!$$", 3);
					}
				}
			}
			// przygotowanie obslugi glosowania
			x[(*th_data).pokoj] = 2;
			ask[(*th_data).pokoj] = 0;
			answer[(*th_data).pokoj] =0;
			wait[(*th_data).pokoj] = 0;
			wait_for_others[(*th_data).pokoj].notify_all();
		}
		else if(game[(*th_data).pokoj]>0) //pozostala trojka graczy postanowila gracz gracz czwarty konczy swoja gre w tym pokoju
		{
			cout<<"4 bye bye"<<endl;
			players[(*th_data).pokoj] -= 1;
			if (write((*th_data).nr_deskryptora1, "XXX$$", 5) < 0)
			{ //sorki inni juz graja
				cout << "write error" << endl;
				write((*th_data).nr_deskryptora1, "!$$", 3);
			}
		
			close((*th_data).nr_deskryptora1);
			wait_for_others[(*th_data).pokoj].notify_all();  //powiadom tych, ktorzy czekaja az sobie pojedziesz, ze opuszczasz rozgrywke
			threads[(*th_data).numer].detach();
			return;
		}
	}
	else
	{ //tylko dla gracza 1,2,3

		char buf[5];
		string mes = "50";								  //kod wiadomosci
		mes.append(to_string(players[(*th_data).pokoj])); //liczba graczy w pokoju
		mes.append("$$");
		strcpy(buf, mes.c_str());
		if (write((*th_data).nr_deskryptora1, &buf, 5) < 0)
		{ //spytaj czy czekamy jeszcze za kims
			cout << "write error" << endl;
			write((*th_data).nr_deskryptora1, "!$$", 3);
		}
	}

	

	while (h)
	{
		if (x[(*th_data).pokoj] == 0 || x[(*th_data).pokoj] == 2)
		{

			
			answer[(*th_data).pokoj]+=1; //zwieksz licznik graczy ktorzy glosuja
			read_int = read((*th_data).nr_deskryptora1, &(*th_data).data, 3 * sizeof(char));
			if (read_int < 1){  //obsluz blad
				cout<<"read_error"<<endl;
				players[(*th_data).pokoj]-=1;
				write((*th_data).nr_deskryptora1, "!$$", 3);
				if(players[(*th_data).pokoj]==0)  //jezeli liczba graczy bedzie rowna 0 wyczysc pokoj
					clean_after_game((*th_data).pokoj);
				
				close((*th_data).nr_deskryptora1);
				threads[(*th_data).numer].detach();
				return;
			}
			string s((*th_data).data);
			s = s.substr(0, s.length() - 2);
			int nume = stoi(s); // glos 1- gramy ,0 czekamy
			ask[(*th_data).pokoj] += nume;  //dodaj glos  
			wait[(*th_data).pokoj] += 1;   //zwieksz licznik odpowiedzi
		}
		else
			break;

		if (ask[(*th_data).pokoj] > (answer[(*th_data).pokoj] / 2))
		{ // wiekszosc graczy woli grac no to gramy bez czekania 
			game[(*th_data).pokoj] = 1;

			h = false;
			x[(*th_data).pokoj] = 1;
			wait_for_others[(*th_data).pokoj].notify_all();  //powiadom graczy ktorzy czekaja
			break;
		}
	
		else if (ask[(*th_data).pokoj] <= (answer[(*th_data).pokoj] / 2)) //gracze czekaja za nowym gracze lub czekaja na wynik glosowania
		{ //czekamy
			
			unique_lock<mutex> lck3(mutex_games[(*th_data).pokoj]);
			cv_status statusF2;
			statusF2 = wait_for_others[(*th_data).pokoj].wait_until(lck3, system_clock::now() + seconds(100));
			if (statusF2 == cv_status::timeout)   //jezeli oczekuja juz 100 sekund to zaczynaja gre pomimo tego ze nowy gracz jeszcze nie doszedl
			{
				game[(*th_data).pokoj] = 1;
				x[(*th_data).pokoj] = 1;
			}

			if (x[(*th_data).pokoj] == 1)
			{
				break;
			}
		}
	}

	h = false;
	
	if(rooms[(*th_data).pokoj][4] == (*th_data).nr_deskryptora1 ){  //jezeli jestes piatym graczem podbij liczniki
		wait[(*th_data).pokoj]++;
		answer[(*th_data).pokoj]++;
	}
	int c=0;
	while (wait[(*th_data).pokoj] < answer[(*th_data).pokoj] ) // jezeli liczba graczy ktorzy oddali glos jest mniejsza niz liczba tych ktorzy mieli to zrobic
	{
		unique_lock<mutex> lck2(mutex_games[(*th_data).pokoj]);
		//cout << "czekam" << endl;
		c=1;
		wait_for_others[(*th_data).pokoj].wait(lck2);   //czekaj

	}

	if (wait[(*th_data).pokoj] == answer[(*th_data).pokoj] &&c==0 )   // jezeli liczba graczy ktorzy oddali glos jest taka sama jak liczba tych ktorzy mieli to zrobic
	{
		c=1;
		wait_for_others[(*th_data).pokoj].notify_all();    // powiadom wszytskie oczekujace watki
		cout << "wszyscy odpowiedzieli w ankiecie " << endl;

		
	}

	while (answer[(*th_data).pokoj] < players[(*th_data).pokoj] )  //oczekiwanie to zachodzi w przypadku, gdy podczas glosowania gracze 1-3 uznali ze nie chca czekac, ale
	{                                                              //ale w tym czasie jeszcze ktos sie polaczyl, wiec gracze 1-3 musza czekac dopoki oni opuszcza pokoj
		unique_lock<mutex> lck5(mutex_games[(*th_data).pokoj]);
		cout << "czekamy aby gracz 4 lub 5 sobie poszli" << endl;
	
		wait_for_others[(*th_data).pokoj].wait(lck5);
	}


	//komunikat o zaczeciu gry i slowo hasla wysyla pierwszy watek ktory dojdzie do tego momentu
	if (game[(*th_data).pokoj] == 1)  
	{

		game[(*th_data).pokoj] = 2;
		//WYSLANIE KOMUNIKATU O ZACZECIU GRY
		for (int i = 0; i < 5; i++)
		{
			if (rooms[(*th_data).pokoj][i] != 0)
			{
				if (write(rooms[(*th_data).pokoj][i], "600$$", 5) < 0)
				{ //zaczynamy gre
					rooms[(*th_data).pokoj][i] = 0;
					ids[(*th_data).pokoj][i]=99;
					write(rooms[(*th_data).pokoj][i], "!$$", 3);
					cout << "write error" << endl;
				}
			}
		}


		

		//WYSYLANIE SLOWA, IDkow I NICKow

		if (words[(*th_data).pokoj] == "0")
		{
			words[(*th_data).pokoj] = get_word(wordsfile, number_of_words); //losuje slowo
			cout << words[(*th_data).pokoj] << endl;
			count_letters((*th_data).pokoj, words[(*th_data).pokoj]); //liczy tylko litery bez spacji od razu zapisuje wynik do guess
			string s = words[(*th_data).pokoj];
			//tworzenie wiadomosci
			s.append(";");
			for (int i = 0; i < 5; i++)
			{
				if (ids[(*th_data).pokoj][i] != 99)
				{
					s.append(to_string(ids[(*th_data).pokoj][i]));
					s.append(";");
					for (int nick = 0; nick < (int)nicks.size(); nick++)
					{
						if (nicks[nick] == to_string(ids[(*th_data).pokoj][i]))
						{
							s.append(nicks[nick - 1]);
							s.append(";");
						}
					}
				}
				else
					break;
			}
			int n = s.length();
			char cstr[138];

			if (n < 138)
			{
				string stuff(138 - n, '.');
				s.append(stuff);
			}
			s.append("$$");
			strcpy(cstr, s.c_str()); 
			
			
			for (int i = 0; i < 5; i++)
			{
				if (rooms[(*th_data).pokoj][i] != 0)   //kazdy klient, ktory jest aktywny
				{
					if (write(rooms[(*th_data).pokoj][i], &cstr, 138) < 0)  //wysylamy klientom slowo, id oraz nick wszystkich klientow
					{ 
						cout << "write error" << endl;  // wyszedl blad podczas wyslanie, czyli dany klient nie jest juz aktywny
						rooms[(*th_data).pokoj][i]=0;
						write(rooms[(*th_data).pokoj][i], "!$$", 3);
					}
				}
			}
			game[(*th_data).pokoj] = 2;
		}
	}

	
	//ROZGRYWKA
	while (players[(*th_data).pokoj] > 1)  //dopoki liczba graczy jest wieksza od 1
	{
		
		n = 0;

		read_int = read((*th_data).nr_deskryptora1, &(*th_data).data2, 3 * sizeof(char));
		if (read_int == -1) //obsluz blad podczas odczytu danych
		{
			handle_error_read((*th_data).nr_deskryptora1, (*th_data).pokoj, (*th_data).numer);
			break;
			
		}
		else
			n += read_int;

		while (n != 3)
		{

			if (read_int == -1)
			{
				handle_error_read((*th_data).nr_deskryptora1, (*th_data).pokoj, (*th_data).numer);
				break;
			
			}
			else
			{
				read_int = read((*th_data).nr_deskryptora1, &(*th_data).data2[n], 3 * sizeof(char));
				n += read_int;
			}
		}

		cout << "przed check" << endl;
		if (guess[(*th_data).pokoj] > 0 && lost[(*th_data).pokoj] != players[(*th_data).pokoj] && read_int != -1)  //slowo jest caly czas nieodgadniete, i jeszcze wszyscy klienci nie sa wisielcami
		{ //czy jeszcze warto sprawdzac
			mutex_games[(*th_data).pokoj].lock();
			check = check_in_word((*th_data).data2, words[(*th_data).pokoj], (*th_data).pokoj);
			mutex_games[(*th_data).pokoj].unlock();

			cout << "check: " << check << endl;
			if(check == -2){                   //klinet wysylal wiadomosc z #, czyli informuje serwer ze sie rozlaczyl
				players[(*th_data).pokoj] -= 1;  //ogolna liczba graczy zostaje zmniejszona
				break;
			}
			else if (check == -1)  //litery wyslanej przez klienta nie ma w hasle, poziom wisielca danego klienta zostaje zwiekszony. wszyscy klienci zostaja o tym poinformowani
			{
				cout << "hangman+1" << endl;
				(*th_data).hangman += 1;				  //masz kolejny poziom wisielca
				string resu = "8";						  //kod wiadomosci
				resu.append(to_string((*th_data).numer)); //nr id gracza
				if ((*th_data).numer < 10)
					resu.append(".");
				resu.append(to_string((*th_data).hangman)); //poziom wisileca
				resu.append("$$");
				cout << resu << endl;
				char buff[6];
				strcpy(buff, resu.c_str());

				for (int i = 0; i < 5; i++)
				{
					n = 0;
					if (rooms[(*th_data).pokoj][i] != 0 && guess[(*th_data).pokoj] > 0)
					{
						while (n < 6)
						{
							send_int = write(rooms[(*th_data).pokoj][i], &buff[n], strlen(buff) - n);
							if (send_int == -1)
							{
								rooms[(*th_data).pokoj][i] = 0;
								write(rooms[(*th_data).pokoj][i], "!$$", 3);
								break;
							}
							else
								n += send_int;
						}
					}
				}

				if ((*th_data).hangman == 6 && guess[(*th_data).pokoj] > 0)
					lost[(*th_data).pokoj]++; //przegrales, liczba wisielcow w pokoju zwiekszona
			}

			else
			{ //>0  // gra jeszcze nie skonczona wyslij sobie i innym gracza aktualizacje twojego wyniku
				guess[(*th_data).pokoj] -= check;
				string res((*th_data).data2, 1);		 //litera
				res.insert(0, "9");						 //kod wiadomosci
				res.append(to_string((*th_data).numer)); //nr id gracza
				if ((*th_data).numer < 10)
				{
					res.append(".");
				}

				res.append("$$");
				char buff[6];
				strcpy(buff, res.c_str());

				for (int i = 0; i < 5; i++)
				{
					n = 0;
					if (rooms[(*th_data).pokoj][i] != 0)
					{
						while (n < 6)
						{
							send_int = write(rooms[(*th_data).pokoj][i], &buff[n], strlen(buff) - n);
							if (send_int == -1)
							{
								rooms[(*th_data).pokoj][i] = 0;
								write(rooms[(*th_data).pokoj][i], "!$$", 3);
						
								break;
							}
							else
								n += send_int;
						}
					}
				}
			}
		}
		if (guess[(*th_data).pokoj] <= 0 || lost[(*th_data).pokoj] == players[(*th_data).pokoj])
		{ //koniec gry - haslo cale odgadniete lub liczba graczy =liczbie wisielcow

			n = 0;
			while (n < 6)
			{
				send_int = write((*th_data).nr_deskryptora1, "7777$$", 6); //komunikat o zamknieciu gry przez serwer
				
				if (send_int == -1)
				{
					write((*th_data).nr_deskryptora1, "!$$", 1);
					printf("server has an unexpected problem\n");
					close((*th_data).nr_deskryptora1);
					threads[(*th_data).numer].detach();
					return;
					
				}
				n += send_int;
			}

		
			if (notyou[(*th_data).pokoj] == 0)   //pierwszy watek jaki sie tu dostanie czysci pokoj i przygotowywyje go na nowa rozgrywke
			{
				notyou[(*th_data).pokoj] = 1;
				sleep(3);
				mutex_games[(*th_data).pokoj].lock();
				clean_after_game((int)(*th_data).pokoj); //przygotowuje pokoj na nowa rozgrywke
				mutex_games[(*th_data).pokoj].unlock();
				
				break;
			}
			else
			{
				
				break;
			}
		}
	}
	if(players[(*th_data).pokoj]==1){             //pozostal tylko 1 gracz, serwer konczy gre
		cout<<"zostal tylko jeden gracz"<<endl;
		send_int = write((*th_data).nr_deskryptora1, "7777$$", 6); //komunikat o zamknieciu gry przez serwer
				if (send_int == -1)
				{	
					clean_after_game((int)(*th_data).pokoj); 
					write((*th_data).nr_deskryptora1, "!", 1);
					printf("server has an unexpected problem\n");
					close((*th_data).nr_deskryptora1);
					threads[(*th_data).numer].detach();
					return;
					
				}
				else{
					clean_after_game((int)(*th_data).pokoj); 
				}
	}

	cout << endl<<"zamykam pokoj " << (*th_data).numer << endl;
	close((*th_data).nr_deskryptora1);
	threads[(*th_data).numer].detach();
}

void ask_nick(int connection_socket_descriptor)
{
	bool go = true;
	//wybieranie nicku
	while (go)
	{
		char data[20]{};
		int len = read(connection_socket_descriptor, data, sizeof(data) - 1);
		if (len < 1)
			break;
		
		string s(data);
		s = s.substr(0, s.length() - 2);
		
		if (check_nicks(s))      //nick wolny
		{
			if (write(connection_socket_descriptor, "100$$", 5) < 0)
			{ // dodanie nicku powiodlo sie, prosba o nowy pokoj
				cout << "write error" << endl;
			}

			go = false;
		}
		else if (!check_nicks(s))
		{
			if (write(connection_socket_descriptor, "200$$", 5) < 0)
			{ //nick zajety, podaj nowy
				cout << "write error" << endl;
			}
		}
	}
}

void handleConnection(int connection_socket_descriptor, int id, int gdzie)
{
	int identyfikator = id;
	int room = gdzie;

	mutex_players.unlock();

	t_data[identyfikator].nr_deskryptora1 = connection_socket_descriptor;
	t_data[identyfikator].pokoj = room;
	t_data[identyfikator].numer = identyfikator;
	t_data[identyfikator].hangman = 0;


	//cout<<"jestem w handle"<<endl;
	threads[identyfikator] = thread(ThreadBehavior, &t_data[identyfikator]);
}
void signal_handler(int sig)
{
	close(server_socket_descriptor);
	exit(0);
}

int main(int argc, char *argv[])
{

	int connection_socket_descriptor;
	int bind_result;
	int listen_result;
	char reuse_addr_val = 1;
	struct sockaddr_in server_address;
	//signal(SIGINT, signal_handler);
	init();
	signal(SIGPIPE, SIG_IGN);

	//inicjalizacja gniazda serwera
	memset(&server_address, 0, sizeof(struct sockaddr));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);

	server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket_descriptor < 0)
	{
		fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
		exit(1);
	}
	setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_addr_val, sizeof(reuse_addr_val));

	bind_result = bind(server_socket_descriptor, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
	if (bind_result < 0)
	{
		fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
		exit(1);
	}

	listen_result = listen(server_socket_descriptor, 1);
	if (listen_result < 0)
	{
		fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
		exit(1);
	}
	while (1)
	{
		connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
		cout << "socket: " << connection_socket_descriptor << endl;
		if (connection_socket_descriptor < 0)
		{
			fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
			exit(1);
		}
		printf("Accepted a new connection\n");
		if (write(connection_socket_descriptor, "000$$", 5) < 0)
		{ //akceptacja nowego polaczenia prosba o dodanie nicku
			cout << "write error" << endl;
		}
		mutex_players.lock();
		ask_nick(connection_socket_descriptor);
		bool room = true;
		//wybieranie pokoju
		while (room)
		{
			char data[20]{};
			int len = read(connection_socket_descriptor, data, sizeof(data) - 1);
			if (len < 1)
				break;
			string s(data);
			s = s.substr(0, s.length() - 2);

			//mutex_players.lock();
			int num = stoi(s);
			num--;
			cout << num << endl;
			if (rooms[num][0] * rooms[num][1] * rooms[num][2] * rooms[num][3] * rooms[num][4] * rooms[num][5] == 0 and game[num] == 0)
			{

				for (int i = 0; i < 5; i++)
				{
					//mutex

					if (rooms[num][i] == 0)
					{

						rooms[num][i] = connection_socket_descriptor;
						notyou[num] = 0;
						id = id_queue.front();
						id_queue.pop();
						nicks.push_back(to_string(id));
						ids[num][i] = id;
						char cs[5];
						string si = "3";
						if (id < 10)
						{
							si.append("0");
							si.append(to_string(id));
						}
						else
							si.append(to_string(id));
						si.append("$$");
						strcpy(cs, si.c_str());
						if (write(connection_socket_descriptor, &cs, 5) < 0)
						{ //jestes dopisany do pokoju
							cout << "write error" << endl;
						}
						players[num] += 1; //zwiekszenie luczby licznika graczy w pokoju
						room = false;
						handleConnection(connection_socket_descriptor, id, num);

						//x=false;
						break;
					}
				}
			}
			else
			{
				if (write(connection_socket_descriptor, "400$$", 5) < 0)
				{
					cout << "write error" << endl;
				}
			}
		}
	}
	close(server_socket_descriptor);
	printf("Connection closed\n");
}