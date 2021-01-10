#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <poll.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

#define SERVER_PORT 1234

using namespace std;


vector<string> nicks;
mutex mutex_players;
vector<mutex> mutex_games;
vector<condition_variable> wait_for_others;
thread threads[15];//dla kazdego gracza
int rooms[3][6];
int id=0;

struct thread_data_t {
    int nr_deskryptora1;            // deskryptor powiązany z danym wątkiem
    int nr_deskryptora2; 
    int nr_deskryptora3;
    int nr_deskryptora4;
    int nr_deskryptora5;           // deskryptor przeciwnika z pary
    char data[6];                   // tablica do przesyładnia danych
    int pokoj;                      // numer pokoju 
    int numer;                      // identyfikator (ten, który jest przypisywany na samym początku)
  
};


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
		for(int j=0;j<6;j++){
			rooms[i][j]=0;
		}
	}
}

void *ThreadBehavior(void *t_data){

}

/*void handleConnection(int connection_socket_descriptor, int id, int gdzie) {
    int identyfikator = id;
    int room = gdzie;
    pthread_mutex_unlock(&players);
    mutex_players.unlock();
    t_data[identyfikator].nr_deskryptora1 = connection_socket_descriptor;
    t_data[identyfikator].pokoj = room;
    t_data[identyfikator].numer = identyfikator;

    pthread_create(&thread[identyfikator], NULL, ThreadBehavior, (void *)&t_data[identyfikator]);
}

*/

int main(int argc, char* argv[]) {
    int server_socket_descriptor;
    int connection_socket_descriptor;
    int bind_result;
    int listen_result;
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;
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
	//cout<<connection_socket_descriptor<<endl;
	 if (connection_socket_descriptor < 0)
        {
            fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
            exit(1);
        }
	printf("Accepted a new connection\n");
	if(write(connection_socket_descriptor, "0", 1)<0){
		cout<<"write error"<<endl; 
}
 	//bool x=true;
	//while(x){
		bool go = true;
			//wybieranie nicku
	   		while(go){
				char data[20]{};
            			int len = read( connection_socket_descriptor, data, sizeof(data)-1);
            			if(len<1) break;
           			 printf(" Received %2d bytes: |%s|\n",len, data);
				string s(data);
				if(check_nicks(s)){
					if(write(connection_socket_descriptor, "1", 1)<0){
						cout<<"write error"<<endl;
					}
					go=false;
				}
				else if (!check_nicks(s)){
					if(write(connection_socket_descriptor, "2", 1)<0){
						cout<<"write error"<<endl;
					}

				}
			}
		bool room =true;
		//wybieranie pokoju
		while(room){
			char data[20]{};
            		int len = read( connection_socket_descriptor, data, sizeof(data)-1);
            		if(len<1) break;
           			printf(" Received %2d bytes: |%s|\n",len, data);
			string s(data);
			int num = stoi(s);
			cout<<num<<endl;
			num--;
		if(rooms[num][0]*rooms[num][1]*rooms[num][2]*rooms[num][3]*rooms[num][4]*rooms[num][5]==0){
			for(int i=0;i<5;i++){
				//mutex
				//mutex_players.lock();
				if(rooms[num][i]==0){
					id++;
					rooms[num][i]=id;
					if(write(connection_socket_descriptor, "3", 1)<0){
						cout<<"write error"<<endl;
					}					
					room=false;
					//x=false;
					break;
				}
			}
		}
		else{
			if(write(connection_socket_descriptor, "4", 1)<0){
						cout<<"write error"<<endl;
					}
			
		}

		} //koniec while(room)

		//handleConnection(connection_socket_descriptor, identy, i);
		

 		
       // }
	//sleep(1);
	
}
  close(server_socket_descriptor);
printf("Connection closed\n");
	
}

