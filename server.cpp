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
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#define SERVER_PORT 1234

using namespace std;


vector<string> nicks;
int rooms[3][6];
int id=0;

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
 
	while(true){
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
				if(rooms[num][i]==0){
					id++;
					rooms[num][i]=id;
					if(write(connection_socket_descriptor, "3", 1)<0){
						cout<<"write error"<<endl;
					}
					room=false;
					break;
				}
			}
		}
		else{
			if(write(connection_socket_descriptor, "4", 1)<0){
						cout<<"write error"<<endl;
					}
			
		}

		}

        }
	sleep(1);
	  
}
 close(server_socket_descriptor);
	printf("Connection closed\n");
}

