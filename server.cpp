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

#define SERVER_PORT 1235

using namespace std;


vector<string> nicks;
mutex mutex_players;
mutex mutex_games[3];
condition_variable wait_for_others[3];
thread threads[15];//watek dla kazdego gracza
int rooms[3][5];                   //dla kazdego pokoju tablica deskryptorow graczy
int id=0;
int ask[3]{0,0,0};                // pomocniczy licznik do ankiety
int players[3]{0,0,0};           //liczba zalogowanych graczy w poszczegolnym pokoju
bool game[3]{false,false,false}; //czy gra zaczela sie
int server_socket_descriptor;

struct thread_data_t {
    int nr_deskryptora1;            // deskryptor powiązany z danym wątkiem
            // deskryptor przeciwnika z pary
    char data[3];                   // tablica do przesyładnia danych
    int pokoj;                      // numer pokoju 
    int numer;
    int done;                      // identyfikator (ten, który jest przypisywany na samym początku)
  
};

struct thread_data_t t_data[15];

string code_message(string message){
	 return message.append("$$");
}

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
		for(int j=0;j<5;j++){
			rooms[i][j]=0;
		}
	}
}

void ThreadBehavior(thread_data_t *t_data){
	struct thread_data_t *th_data = (struct thread_data_t*)t_data;
	bool wakeup=true;
	bool wakeup2=true;
	char data2[3]{};
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

	
	if(rooms[(*th_data).pokoj][4] != 0){ //jest 5 graczy nic nie trzeba robic GRAMY
		if(write((*th_data).nr_deskryptora1, "6$$", 3)<0){//wyslij ze zaczynamy gre
			cout<<"write error"<<endl; 
		}
		wakeup2=false;
		game[(*th_data).pokoj] = true;
	}
	else { //tylko dla gracza 1,2,3,4
		if(write((*th_data).nr_deskryptora1, "5$$", 3)<0){//spytaj czy czekamy jeszcze za kims
			cout<<"write error"<<endl; 
		}


		if(rooms[(*th_data).pokoj][3]==(*th_data).nr_deskryptora1){ //4 gracz zeruje licznik, dla ponownego pytania czy czekamy za 5
			for(int i=0;i<3;i++){
				if(write(rooms[(*th_data).pokoj][i], "5$$", 3)<0){//spytaj czy czekamy jeszcze za kims
					cout<<"write error"<<endl; 
				}
			}		
			cout<<endl<<(*th_data).nr_deskryptora1<<endl;		
			ask[(*th_data).pokoj]=0;
		}
		while(wakeup2){
			int read_int = read((*th_data).nr_deskryptora1, &(*th_data).data, 3*sizeof(char));
			if(read_int<1) break;
			string s((*th_data).data);
			s=s.substr(0,s.length()-2);
			cout<<endl<<"przeczytane: "<<s<<endl;
			int nume = stoi(s); //1- gramy ,0 czekamy
			ask[(*th_data).pokoj]+=nume;
			if(ask[(*th_data).pokoj] > (players[(*th_data).pokoj]/2)){  // wiekszosc graczy woli grac no to gramy bez czekania GRAMY
				game[(*th_data).pokoj] = true;
				break;
		
			}
			else if(rooms[(*th_data).pokoj][0] * rooms[(*th_data).pokoj][1]*rooms[(*th_data).pokoj][2]*rooms[(*th_data).pokoj][3]*rooms[(*th_data).pokoj][4] != 0){ //pojawilo sie 5 graczy GRAMY
				game[(*th_data).pokoj] = true;
				break;
			}
			else if (ask[(*th_data).pokoj] < (players[(*th_data).pokoj]/2)){ //czekamy
				continue;
			}


		}
	
	}
	


	if(game[(*th_data).pokoj] == true){

		ask[(*th_data).pokoj]=0;
		cout<<endl<<"taaak!!!"<<endl;
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
		mutex_players.lock();
		if(check_nicks(s)){
			if(write(connection_socket_descriptor, "0$$", 3)<0){
				cout<<"write error"<<endl;
			}
			//mutex_players.unlock();
			go=false;
		}
		else if (!check_nicks(s)){
			if(write(connection_socket_descriptor, "2$$", 3)<0){
				cout<<"write error"<<endl;
			}

		}
		mutex_players.unlock();
	}
}



void handleConnection(int connection_socket_descriptor, int id, int gdzie) {
    int identyfikator = id;
    int room = gdzie;
   // pthread_mutex_unlock(&players);
	mutex_players.unlock();
	ask_nick(connection_socket_descriptor);


    //mutex_players.unlock();
    t_data[identyfikator].nr_deskryptora1 = connection_socket_descriptor;
    t_data[identyfikator].pokoj = room;
    t_data[identyfikator].numer = identyfikator;
    t_data[identyfikator].done=0;
	cout<<"jestem w handle"<<endl;
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
    signal(SIGINT, signal_handler);
    init();
    //inicjalizacja gniazda serwera
    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);

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
	if(write(connection_socket_descriptor, "1$$", 3)<0){
		cout<<"write error"<<endl; 
}
 
		bool room =true;
		//wybieranie pokoju
		while(room){
			char data[20]{};
            		int len = read( connection_socket_descriptor, data, sizeof(data)-1);
            		if(len<1) break;
           			printf(" Received %2d bytes: |%s|\n",len, data);
			string s(data);
			s=s.substr(0,s.length()-2);
			
			mutex_players.lock();
			int num = stoi(s);
			num--;
			cout<<num<<endl;
		if(rooms[num][0]*rooms[num][1]*rooms[num][2]*rooms[num][3]*rooms[num][4]*rooms[num][5]==0){
			
				for(int i=0;i<5;i++){
				//mutex
				
				if(rooms[num][i]==0){
					
					
					rooms[num][i]=connection_socket_descriptor;
					id++;
					if(write(connection_socket_descriptor, "3$$", 3)<0){
						cout<<"write error"<<endl;
					}
					players[num]+=1; //zwiekszenie luczby licznika graczy w pokoju					
					room=false;
					handleConnection(connection_socket_descriptor, id-1, num);
		
					//x=false;
					break;
				}
			}
		}
		else{
			if(write(connection_socket_descriptor, "4$$", 3)<0){
						cout<<"write error"<<endl;
					}
			
		}

		} 

		

	
}
  close(server_socket_descriptor);
printf("Connection closed\n");
	
}

