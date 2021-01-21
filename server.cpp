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
#define SERVER_PORT 1235

using namespace std;


vector<string> nicks;
mutex mutex_players;
mutex mutex_games[3];
condition_variable wait_for_others[3];
thread threads[15];//watek dla kazdego gracza
int rooms[3][5];    //deskryptory wszystkich graczy
int ids[3][5];		//id_wszytskich graczy
int points[3][5];                   //punkty wszystkich graczy
int id=0;
int ask[3]{0,0,0};                // pomocniczy licznik do ankiety
int players[3]{0,0,0};           //liczba zalogowanych graczy w poszczegolnym pokoju
int game[3]{0,0,0}; //czy gra zaczela sie
string words[3];
string done_letter[3];  //przechowuje wykorzytsane juz literki
int guess[3];
int lost[3]{0,0,0};
int wait[3]{0,0,0};
int x[3]{0,0,0};
int server_socket_descriptor;
int port;
string wordsfile;
int number_of_words;
random_device rd;
mt19937_64 generator;
queue < int > id_queue;


struct thread_data_t {
    int nr_deskryptora1;            // deskryptor powiązany z danym wątkiem
            // deskryptor przeciwnika z pary
    char data[3];                   // tablica do przesyładnia danych 
    char data2[3];                   // tablica do przesyładnia danych 
    int pokoj;                      // numer pokoju 
    int numer;				 // identyfikator (ten, który jest przypisywany na samym początku)
    int point;                      // punkty zdobyte podczas gry
	int hangman;
	int done;
	
  
};

struct thread_data_t t_data[15];



bool check_nicks(string s){
	for(string i : nicks){
		if (i==s){
			return false;
		}
	}
	nicks.push_back(s);
	return true;
}

void init(){
	for(int i=0;i<3;i++){
		words[i]="0";
		for(int j=0;j<5;j++){
			rooms[i][j]=0;
			points[i][j]=0;
			ids[i][j]=99;
		}
	}
	for(int i=0;i<15;i++){
		id_queue.push(i);


	}
	ifstream file("config.txt"); //odczyt konfiguracji z pliku port,nazwapliku ze slowami, ilosc slow)
	if(file.is_open()){
    		string line,x;
		getline(file, line);
		x=line;
       		port = stoi(x);
		getline(file, line);
		wordsfile=line;
		getline(file, line);
		x=line;
		number_of_words = stoi(x);
	}
	else{
		cout<<"Brak pliku config";
        exit(1);
	}
	file.close();

}

int check_in_word(char* letter, string word){
	
	string let(letter,3);
	let=let.substr(0,1);
	string w=word;
	int score=0;
	while(w.find(let)!=string::npos){
		
		w=w.substr(w.find(let));
		w=w.substr(1);
		score++;
		


	}
	return score;  //0 - nie bylo litery-0pkt, >0 punkt za kazda odkryta litere
}

string get_word(string filename,int num_lines){
	string word;
	generator = std::mt19937_64(rd());
	uniform_int_distribution<int> distribution(1,num_lines);
	int random_line = distribution(generator);
 	
	ifstream file(filename);
	if(file.is_open()){
    		string line;
    		for(int i = 0; i < random_line; i++){
       			getline(file, line);
    		}
   		word = line;
    	}
	else{
		cout<<"error";
		exit(0);
	}
	file.close();
	
	std::transform(word.begin(), word.end(),word.begin(), ::toupper);
	return word;
}

void count_letters(int pokoj, string word){ //
	string w=word;
	string s =" ";
	int x=0;
	while(w.find(s)!=string::npos){
		//cout<<w<<endl;
		w=w.substr(w.find(s));
		w=w.substr(1);
		x++;
	}
	guess[pokoj]=word.length()-x;
}

int result(int num){
//for po liscie struktur t_data szukajacy maksa
int max;
int win_id;
for(int i=0;i<15;i++){
	if(t_data[i].pokoj==num){
		if(t_data[i].point>max){
			max=t_data[i].point;
			win_id=t_data[i].numer; 
		}
	
}
}
return win_id;
}

void ThreadBehavior(thread_data_t *t_data){
	struct thread_data_t *th_data = (struct thread_data_t*)t_data;
	bool wakeup=true;
	int read_int,check,send_int,j,n;
	bool h;
	int u=0;
	cout<<(*th_data).nr_deskryptora1;
		unique_lock<mutex> lck(mutex_games[(*th_data).pokoj],defer_lock);
		lck.lock();
	
	 while(wakeup){
        // czekamy na drugiego i trzeciego gracza- minimum aby rozpoczac rozgrywke
        if (rooms[(*th_data).pokoj][0] * rooms[(*th_data).pokoj][1]*rooms[(*th_data).pokoj][2] == 0){
		cout<<endl<<"przed wait"<<endl;
		
		wait_for_others[(*th_data).pokoj].wait(lck);
    
        }
        else{wakeup=false;}
    }

	//jest gracz trzeci
	if(rooms[(*th_data).pokoj][2] == (*th_data).nr_deskryptora1){

        wait_for_others[(*th_data).pokoj].notify_all();
		
    }

	lck.unlock();
	
	h=true;

	if(rooms[(*th_data).pokoj][4] != 0){ //jest 5 graczy nic nie trzeba robic GRAMY

		if(wait[(*th_data).pokoj]!=0){//jezeli  ktos czeka na ciebie
			x[(*th_data).pokoj]=1;
			wait_for_others[(*th_data).pokoj].notify_all(); //poinformuj ze jestes i mozemy grac
		}
		
		else if(game[(*th_data).pokoj] != 0){
			threads[(*th_data).numer].detach();
			return;
		
		}
		
		h=false;
		game[(*th_data).pokoj] = 1;
	}
		
////////////////////////////////////////////////////////////////////////////////////////////

	else if(rooms[(*th_data).pokoj][3]==(*th_data).nr_deskryptora1){ //4 gracz zeruje licznik, dla ponownego pytania czy czekamy za 5
			cout<<"czwarty jestem"<<endl;

			while(wait[(*th_data).pokoj]!=(players[(*th_data).pokoj]-1)){
				continue;
			}
			if(game[(*th_data).pokoj]==0){ 
			char buf[5];
			string mes="50"; //kod wiadomosci
			mes.append(to_string(players[(*th_data).pokoj])); //liczba graczy w pokoju
			mes.append("$$");
			strcpy(buf, mes.c_str()); 
			for(int i=0;i<4;i++){
				if(write(rooms[(*th_data).pokoj][i], &buf, 5)<0){//spytaj czy czekamy jeszcze za kims
					cout<<"write error"<<endl; 
				}
			}		
			cout<<endl<<(*th_data).nr_deskryptora1<<endl;
			x[(*th_data).pokoj]=2;		
			ask[(*th_data).pokoj]=0;
			wait[(*th_data).pokoj]=0;
			wait_for_others[(*th_data).pokoj].notify_all();
			}
			else{
			if(write((*th_data).nr_deskryptora1, "XXX$$", 5)<0){//sorki inni juz graja
					cout<<"write error"<<endl; 
				}
				threads[(*th_data).numer].detach();
				return;
			}
	}
	else { //tylko dla gracza 1,2,3

		char buf[5];
		string mes="50"; //kod wiadomosci
		mes.append(to_string(players[(*th_data).pokoj])); //liczba graczy w pokoju
		mes.append("$$");
		strcpy(buf, mes.c_str()); 
		if(write((*th_data).nr_deskryptora1, &buf, 5)<0){//spytaj czy czekamy jeszcze za kims
			cout<<"write error"<<endl; 
		}
		}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		
		while(h){
		if(x[(*th_data).pokoj]==0||x[(*th_data).pokoj]==2){
			
			cout<<endl<<"dupa "<<(*th_data).numer<<endl;
			read_int = read((*th_data).nr_deskryptora1, &(*th_data).data, 3*sizeof(char));
			if(read_int<1) break;
			string s((*th_data).data);
			s=s.substr(0,s.length()-2);
			
			cout<<endl<<"przeczytane: "<<s<<" "<<(*th_data).numer<<endl;
			int nume = stoi(s); //1- gramy ,0 czekamy
			ask[(*th_data).pokoj]+=nume;
			wait[(*th_data).pokoj]+=1;
		}
		else break;
		
			if(ask[(*th_data).pokoj] > (players[(*th_data).pokoj]/2)){  // wiekszosc graczy woli grac no to gramy bez czekania GRAMY
				game[(*th_data).pokoj] = 1;
				
				h=false;
				x[(*th_data).pokoj]=1;
				wait_for_others[(*th_data).pokoj].notify_all();
				cout<<"wychodze"<<endl;
				break;
		
			}
			else if(rooms[(*th_data).pokoj][0] * rooms[(*th_data).pokoj][1]*rooms[(*th_data).pokoj][2]*rooms[(*th_data).pokoj][3]*rooms[(*th_data).pokoj][4] != 0){ //pojawilo sie 5 graczy GRAMY
				game[(*th_data).pokoj] = 1;
				x[(*th_data).pokoj]=1;
				wait_for_others[(*th_data).pokoj].notify_all();
				
				h=false;
				break;
			}
			else if (ask[(*th_data).pokoj] <= (players[(*th_data).pokoj]/2)){ //czekamy
				cout<<endl<<"cccc "<<(*th_data).numer<<endl;
				unique_lock<mutex> lck3(mutex_games[(*th_data).pokoj]);
				wait_for_others[(*th_data).pokoj].wait(lck3);
				if (x[(*th_data).pokoj]==1){
					break;       
				}
			}


		}
	
	
	
	h=false;
	cout<<"wyszo"<<(*th_data).numer<<endl;
	//cout<<(players[(*th_data).pokoj]);
	/*while(ask[(*th_data).pokoj] == (players[(*th_data).pokoj])){
		cout<<"j";
	}*/

	while(wait[(*th_data).pokoj]<players[(*th_data).pokoj]){
		unique_lock<mutex> lck2(mutex_games[(*th_data).pokoj]);
		cout<<"czekam"<<endl;		
		//sleep(5);
		//lck.lock();
		wait_for_others[(*th_data).pokoj].wait(lck2);
		
	}



	if(wait[(*th_data).pokoj]==players[(*th_data).pokoj]){
		
		
		wait_for_others[(*th_data).pokoj].notify_all();
		cout<<"wszyscy odpowiedzieli"<<endl;
		

		wait[(*th_data).pokoj]=8;
	}
	
	
	//lck.unlock();
		
	cout<<"jestem"<<endl;
	//mutex_games[(*th_data).pokoj].lock();
	//komunikat o zaczeciu gry i slowo hasla wysyla pierwszy watek ktory dojdzie do tego momentu
	if(game[(*th_data).pokoj] == 1){

		game[(*th_data).pokoj] = 2;
		//ask[(*th_data).pokoj]=0;
	

		j=players[(*th_data).pokoj];
		for(int i=0;i<j;i++){
				if(write(rooms[(*th_data).pokoj][i], "600$$", 5)<0){//zaczynamy gre
					cout<<"write error"<<endl; 
				}
			}

		cout<<endl<<"taaak!!!"<<endl;

		//game[(*th_data).pokoj] = false;
		
	
	//wysylanie slowa
	
	if(words[(*th_data).pokoj]=="0"){
		words[(*th_data).pokoj]=get_word(wordsfile,number_of_words);
		count_letters((*th_data).pokoj,words[(*th_data).pokoj]); //liczy tylko litery bez spacji od razu zapisuje wynik do guess
		string s=words[(*th_data).pokoj];

		//words[(*th_data).pokoj].replace(words[(*th_data).pokoj].begin(),words[(*th_data).pokoj].end()," ",""); //odjecie spacji
		//guess[(*th_data).pokoj]=words[(*th_data).pokoj].length(); //trzeba by jakos odjac spacje

		s.append(";");
		for(int i=0;i<5;i++){
			if(ids[(*th_data).pokoj][i]!=99){
				s.append(to_string(ids[(*th_data).pokoj][i]));
				s.append(";");
				for(int nick=0;nick<nicks.size();nick++){
					if (nicks[nick]==to_string(ids[(*th_data).pokoj][i])){
						s.append(nicks[nick-1]);
						s.append(";");
					}
				}
				
			}
			else break;
		}
		int n=s.length();
		char cstr[138];

		if(n<138){
			string stuff(138-n, '.');
			s.append(stuff);
		}
		s.append("$$");
    		strcpy(cstr, s.c_str()); //wiadomosc jako char array
		//cout<<cstr<<endl;
		j=players[(*th_data).pokoj];
		for(int i=0;i<j;i++){
				if(write(rooms[(*th_data).pokoj][i], &cstr, 138)<0){//wysylamy liczbe znakow w slowie i id innych graczy
					cout<<"write error"<<endl; 
				}
			}
	game[(*th_data).pokoj]=2;
		
	}
	}
	//mutex_games[(*th_data).pokoj].unlock();
	/*if(write((*th_data).nr_deskryptora1, "777$$", 5)<0){//spytaj czy czekamy jeszcze za kims
			cout<<"write error"<<endl; 
		}*/

	cout<<"kurwa";
	//oczekiwanie na potwierdzenie - uzyc ask
	// glowna petla gry - wysylanie i odbieranie wiadomosci
	
    while(players[(*th_data).pokoj]>1){
	//cout<<"gra"<<endl;
	n = 0;
	
        read_int = read((*th_data).nr_deskryptora1, &(*th_data).data2, 3*sizeof(char));
	cout<<endl<<(*th_data).data2<<endl;
        n += read_int;
	while(n != 3){
	
	if(read_int == -1){
                char buf[6] = {'!', '!', '!', '!', '!', '!'};                  // problem serwera w odczytnaiu danych
                n=0;
                while(n < 6){        
                    send_int = write((*th_data).nr_deskryptora1, &buf[n], strlen(buf)-n);
                    n += send_int;
                    if(send_int == -1){
                        write((*th_data).nr_deskryptora1, "!", 1); 
                        printf("server has an unexpected problem\n"); 
			
                        threads[(*th_data).numer].detach();
			return;
                    }
                } 
		}     
		read_int = read((*th_data).nr_deskryptora1, &(*th_data).data2[n], 3*sizeof(char));
            	n += read_int;
	
	}
	mutex_games[(*th_data).pokoj].lock();
	cout<<"przed check"<<endl;
	check = check_in_word((*th_data).data2, words[(*th_data).pokoj]);
	cout<<"check: "<<check<<endl;
	guess[(*th_data).pokoj]-=check;
	mutex_games[(*th_data).pokoj].unlock();
	if(check==0){
		cout<<"hangman+1"<<endl;
		(*th_data).hangman+=1;	//masz kolejny poziom wisielca
		string resu="8"; //kod wiadomosci
		resu.append(to_string((*th_data).numer)); //nr id gracza
		if((*th_data).numer<10)resu.append("."); 
		resu.append(to_string((*th_data).hangman)); //poziom wisileca
		resu.append("$$");
		cout<<resu<<endl;
		char buff[6];
		strcpy(buff, resu.c_str());
		j=players[(*th_data).pokoj];
			for(int i=0;i<j;i++){
		 	n = 0;
            		while(n < 6){        
                		send_int = write(rooms[(*th_data).pokoj][i], &buff[n], strlen(buff)-n);
                		n += send_int;
                		if(send_int == -1){
                    			write(rooms[(*th_data).pokoj][i], "!", 1); 
                    			printf("server has an unexpected problem\n"); 
                    			//threads[(*th_data).numer].detach(); //to nie pasi
                		}                
            		}        
			}
		

		if((*th_data).hangman==6) lost[(*th_data).pokoj]++;//przegrales
	}
	
	else { //>0  // gra jeszcze nie skonczona wyslij sobie i innym gracza aktualizacje twojego wyniku
		string res((*th_data).data2,1); //litera
		res.insert(0,"9"); //kod wiadomosci
		res.append(to_string((*th_data).numer)); //nr id gracza
		if((*th_data).numer<10){
			res.append(".");
		}

		res.append("$$");
		char buff[6];
		strcpy(buff, res.c_str());
		j=players[(*th_data).pokoj];
			for(int i=0;i<j;i++){
		 	n = 0;
            		while(n < 6){        
                		send_int = write(rooms[(*th_data).pokoj][i], &buff[n], strlen(buff)-n);
                		n += send_int;
                		if(send_int == -1){
                    			write(rooms[(*th_data).pokoj][i], "!", 1); 
                    			printf("server has an unexpected problem\n"); 
                    			//threads[(*th_data).numer].detach();
                		}                
            		}        
			}
	}

	if(guess[(*th_data).pokoj]<=0 || lost[(*th_data).pokoj]==players[(*th_data).pokoj] ){ //koniec gry lub //liczba graczy =liczbie wisielcow
		int winner=result((*th_data).pokoj); //funkcja sprawdzajaca kto wygral
		cout<<winner;
		break;
	}
	
	
	
   

}
	threads[(*th_data).numer].detach();
}

void ask_nick(int connection_socket_descriptor){
  bool go = true;
			//wybieranie nicku
	while(go){
		char data[20]{};
          	int len = read( connection_socket_descriptor, data, sizeof(data)-1);
        	if(len<1) break;
           	printf(" Received %2d bytes: |%s|\n",len, data);
		string s(data);
		s=s.substr(0,s.length()-2);
		//mutex_players.lock();
		if(check_nicks(s)){
			if(write(connection_socket_descriptor, "100$$", 5)<0){ // dodanie nicku powiodlo sie, prosba o nowy pokoj
				cout<<"write error"<<endl;
			}
			//mutex_players.unlock();
			go=false;
		}
		else if (!check_nicks(s)){
			if(write(connection_socket_descriptor, "200$$", 5)<0){ //nick zajety, podaj nowy
				cout<<"write error"<<endl;
			}

		}
		//mutex_players.unlock();
	}
}



void handleConnection(int connection_socket_descriptor, int id, int gdzie) {
    int identyfikator = id;
    int room = gdzie;
 
	mutex_players.unlock();
	//ask_nick(connection_socket_descriptor);


    //mutex_players.unlock();
    t_data[identyfikator].nr_deskryptora1 = connection_socket_descriptor;
    t_data[identyfikator].pokoj = room;
    t_data[identyfikator].numer = identyfikator;
	 t_data[identyfikator].hangman = 0;
	 t_data[identyfikator].done=0;
	

	//cout<<"jestem w handle"<<endl;
    threads[identyfikator] = thread(ThreadBehavior,&t_data[identyfikator]);
  
}
void signal_handler(int sig){
	close(server_socket_descriptor);
	exit(0);
}


int main(int argc, char* argv[]) {
   
    int connection_socket_descriptor;
    int bind_result;
    int listen_result;
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;
    //signal(SIGINT, signal_handler);
    init();
  
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
    setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

    bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if (bind_result < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
        exit(1);
    }

    listen_result = listen(server_socket_descriptor, 1);
    if (listen_result < 0) {
        fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
        exit(1);
    }
while(1){
        connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
	cout<<"socket: "<< connection_socket_descriptor<<endl;
	 if (connection_socket_descriptor < 0)
        {
            fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
            exit(1);
        }
	printf("Accepted a new connection\n");
	if(write(connection_socket_descriptor, "000$$", 5)<0){ //akceptacja nowego polaczenia prosba o dodanie nicku
		cout<<"write error"<<endl; 
}
 		mutex_players.lock();
		ask_nick(connection_socket_descriptor);
		bool room =true;
		//wybieranie pokoju
		while(room){
			char data[20]{};
            		int len = read( connection_socket_descriptor, data, sizeof(data)-1);
            		if(len<1) break;
           			printf(" Received %2d bytes: |%s|\n",len, data);
			string s(data);
			s=s.substr(0,s.length()-2);
			
			//mutex_players.lock();
			int num = stoi(s);
			num--;
			cout<<num<<endl;
		if(rooms[num][0]*rooms[num][1]*rooms[num][2]*rooms[num][3]*rooms[num][4]*rooms[num][5]==0 and game[num] == 0){
			
				for(int i=0;i<5;i++){
				//mutex
				
				if(rooms[num][i]==0){
					
					
					rooms[num][i]=connection_socket_descriptor;
					id=id_queue.front();
					id_queue.pop();
					nicks.push_back(to_string(id));
					ids[num][i]=id;
					char cs[5];
					string si ="3";			
					if(id<10){
						si.append("0");
						si.append(to_string(id));
					}
					else si.append(to_string(id));
					si.append("$$");
					strcpy(cs, si.c_str());
					if(write(connection_socket_descriptor, &cs, 5)<0){ //jestes dopisany do pokoju
						cout<<"write error"<<endl;
					}
					players[num]+=1; //zwiekszenie luczby licznika graczy w pokoju					
					room=false;
					handleConnection(connection_socket_descriptor, id, num);
		
					//x=false;
					break;
				}
			}
		}
		else{
			if(write(connection_socket_descriptor, "400$$", 5)<0){
						cout<<"write error"<<endl;
					}
			
		}

		} 

		

	
}
  close(server_socket_descriptor);
printf("Connection closed\n");
	
}