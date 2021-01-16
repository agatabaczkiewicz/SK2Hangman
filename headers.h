using namespace std;

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
		}
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
		cout<<"error";
		exit(0);
	}
	file.close();

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


	return word;
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
			if(write(connection_socket_descriptor, "1$$", 3)<0){ // dodanie nicku powiodlo sie, prosba o nowy pokoj
				cout<<"write error"<<endl;
			}
			//mutex_players.unlock();
			go=false;
		}
		else if (!check_nicks(s)){
			if(write(connection_socket_descriptor, "2$$", 3)<0){ //nick zajety, podaj nowy
				cout<<"write error"<<endl;
			}

		}
		mutex_players.unlock();
	}
}

