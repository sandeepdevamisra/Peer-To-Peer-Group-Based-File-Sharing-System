#include<iostream>
#include<sstream>
#include<fstream>
#include<vector>
#include<unordered_map>
#include<map>
#include<set>
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <fstream>
#include <thread>
#include <netdb.h>
#include <pthread.h> 
#include <semaphore.h>
#define BUFFER_SIZE 524288
using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

string TRACKER_IP;
int TRACKER_PORT;
string PEER_IP;
int PEER_PORT;
string IPORT;

string userID;
string passWORD;
bool loginStatus = false;

vector<thread> thV;
vector<pair<string,string> > downloadHistory;

void error(string s){
    cout<<s;
    exit(1);
}

vector<string> split_args(string str){
	vector<string> v;
	stringstream ss(str);
    while (ss.good()) {
        string sb;
        getline(ss, sb, ':');
        v.push_back(sb);
    }
	return v;
}

void save_tracker_details(string path){
	string line;
	ifstream readFile(path);
	while (getline (readFile, line));
	readFile.close();
	vector<string> tdv = split_args(line);
	TRACKER_IP = tdv[0];
	TRACKER_PORT = stoi(tdv[1]);
} 

void process_request(int newsockfd, int port){
	string ip = TRACKER_IP;

	while(true){
		char buffer[BUFFER_SIZE];
		bzero(buffer, BUFFER_SIZE);
		int n = read(newsockfd, buffer, BUFFER_SIZE);
		if(n<0) error("Error on reading!");
		string command = buffer;
		vector<string> tokens;
		stringstream ss(command);
    	while(ss.good()){
     	    string sb;
			getline(ss, sb, '|');
        	tokens.push_back(sb);}
        
		string cmd = tokens[0];

		if(cmd == "get_chunk"){
			string path = tokens[1];
			long long fileSize = stoll(tokens[2]);
			
		FILE* sourceFile;
      
        sourceFile = fopen(path.c_str(), "r");
        long long numb;
        if(sourceFile == NULL){
            cout << "This file can't be opened\n"; return;
        }
         
        fseek(sourceFile, 0, SEEK_END);
		numb = ftell(sourceFile);
		fseek(sourceFile, 0, SEEK_SET);
        unsigned char buffer[numb];
         
        fread(buffer, sizeof(char), numb, sourceFile);

		char buff[numb];
        write(newsockfd, buffer, numb);
        bzero(buff, fileSize);    
        fclose(sourceFile);

		break;
		}
		if(cmd == "send_packet"){

			break;
		}				
	}

	return;
}

string transmit_data_to_tracker(int sockfd, char buffer[]){
	int i = strncmp("Bye", buffer, 3);
	if(i == 0){
		string con_closed = "closed";
		strcpy(buffer, con_closed.c_str());
		int n = send(sockfd, buffer, strlen(buffer), 0);
		if(n<0) error("Error on writing!");
		return "SUCCESS";
	}
	int n = send(sockfd, buffer, strlen(buffer), 0);
	if(n<0) error("Error on writing!");

	bzero(buffer, BUFFER_SIZE);
	n = recv(sockfd, buffer, BUFFER_SIZE, 0);
	if(n<0) error("Error on reading!");

	string r = buffer;
	cout<<"Response: "<<r;

	cout<<endl<<endl;
	return r;
}

string connect_with_tracker(char buffer[]){

		int sockfd, n; 
		struct sockaddr_in server_addr;
		struct hostent *server; 
		
		sockfd = socket(AF_INET, SOCK_STREAM, 0);	
		if(sockfd < 0) error("Error Opening socket!");

		server = gethostbyname(TRACKER_IP.c_str());
		if(server == NULL) error("No such host..");
	
		bzero((char *) &server_addr, sizeof(server_addr));

		server_addr.sin_family = AF_INET;
		bcopy((char *) server->h_addr, (char *) &server_addr.sin_addr.s_addr, server->h_length);
		server_addr.sin_port = htons(TRACKER_PORT);

		if(connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) error("Connection failed !");

		return transmit_data_to_tracker(sockfd, buffer);
}

void downloadChunk(string IP, int port, string path, string destination, long long fileSize){

	int sockfd, n; 
	struct sockaddr_in server_addr;
	struct hostent *server;

	string cmd = "get_chunk|" + path + "|" + to_string(fileSize) ;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);	
	if(sockfd < 0) error("Error Opening socket!");

	server = gethostbyname(IP.c_str());
	if(server == NULL) error("No such host..");
	
	bzero((char *) &server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &server_addr.sin_addr.s_addr, server->h_length);
	server_addr.sin_port = htons(port);

	if(connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) error("Connection failed !");	

	n = send(sockfd, cmd.c_str(), strlen(cmd.c_str()), 0);
	if(n<0) error("Error on writing!");

	char buff[BUFFER_SIZE];
	bzero(buff, BUFFER_SIZE);
	n = recv(sockfd, buff, BUFFER_SIZE, 0);
	if(n<0) error("Error on reading!");

	ofstream f; f.open(destination);
    f << buff; f.close();
}

void download(vector<string> tkns){
	string groupId = tkns[1];
	string fileName = tkns[2];
	string destination = tkns[3];

	int sockfd, n; 
	struct sockaddr_in server_addr;
	struct hostent *server;

	string cmd = "seeder " + groupId + " " + fileName + " " + userID;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);	
	if(sockfd < 0) error("Error Opening socket!");

	server = gethostbyname(TRACKER_IP.c_str());
	if(server == NULL) error("No such host..");
	
	bzero((char *) &server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &server_addr.sin_addr.s_addr, server->h_length);
	server_addr.sin_port = htons(TRACKER_PORT);

	if(connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) error("Connection failed !");	

	n = send(sockfd, cmd.c_str(), strlen(cmd.c_str()), 0);
	if(n<0) error("Error on writing!");

	char buff[BUFFER_SIZE];
	bzero(buff, BUFFER_SIZE);
	n = recv(sockfd, buff, BUFFER_SIZE, 0);
	if(n<0) error("Error on reading!");

	string r = buff;
	vector<string> seedList;
	stringstream ss(r);
    while(ss.good()){
     	string sb;
		getline(ss, sb, '|');
       	seedList.push_back(sb);
	}

	vector<string> seedIp;
	vector<int> seedPort;
	string msg = seedList[0];
	long long FILE_SIZE = stoll(seedList[seedList.size()-2]);
	string filePath = seedList[seedList.size()-1];

	cout<<"Response: "<<msg<<endl;
	for(int i=1; i<seedList.size()-2; i++){

		string seedAddr = seedList[i];
		vector<string> sda;
		stringstream ss1(seedAddr);
    	while(ss1.good()){
     		string sb1;
			getline(ss1, sb1, ':');
       		sda.push_back(sb1);
		}

		seedIp.push_back(sda[0]);
		seedPort.push_back(stoi(sda[1]));
	}
	for(int i=0; i<seedIp.size(); i++){
		cout<<"          "<<i+1<<". IP : "<<seedIp[i]<<" | Port : "<<seedPort[i]<<endl;
	}
	cout<<endl;

	long long NO_OF_CHUNKS = (FILE_SIZE)/1024;
	cout<<"          "<<"No. of chunks created : "<<NO_OF_CHUNKS<<endl;	

	downloadChunk(seedIp[0], seedPort[0], filePath, destination, FILE_SIZE);
	downloadHistory.push_back(make_pair(groupId, fileName));
	cout<<endl<<"          "<<"File Downloaded Successfully."<<endl;
	cout<<endl;
}

bool isLogout(char buffer[]){
	int i = strncmp("logout", buffer, 6);
	if(i == 0) return true;
	else return false;
}

bool listAllGroups(char buffer[]){
	int i = strncmp("list_groups", buffer, 11);
	if(i == 0) return true;
	else return false;
}

bool isShowDownloads(char buffer[]){
	int i = strncmp("show_downloads", buffer, 14);
	if(i == 0) return true;
	else return false;
}

void execute_command(char buffer[]){

	if(isShowDownloads(buffer)){
		if(loginStatus == false){
			cout<<"Response: You're not logged in."<<endl<<endl;
			return;
		}

		if(downloadHistory.size()==0){
			cout<<"Response: Download history is empty."<<endl<<endl;
			return;
		}

		for(auto dh : downloadHistory){
			cout<<"          "<<"[C] "<< dh.first << " "<< dh.second<<endl;
		}

	}

	if(isLogout(buffer)){

		if(loginStatus == false){
			cout<<"Response: You're not logged in."<<endl<<endl;
			return;
		}

		string data = "logout " + userID + " " + IPORT;
		char buff[BUFFER_SIZE];
		strcpy(buff, data.c_str());

		string resp = connect_with_tracker(buff);

		if(resp == "Logout Successfull."){

	

			loginStatus = false;
			userID = "";
			passWORD = "";
		}
		return;
	}

	if(listAllGroups(buffer)){

		if(loginStatus == false){
			cout<<"Response: You're not logged in. Please login to view all groups."<<endl<<endl;
			return;
		}

		string data = "list_groups " + userID;
		char buff[BUFFER_SIZE];
		strcpy(buff, data.c_str());

		string resp = connect_with_tracker(buff);

		return;
	}

	string cmd = buffer;
	vector<string> tokens;
	stringstream ss(cmd);
    while(ss.good()){
     	string sb;
		getline(ss, sb, ' ');
       	tokens.push_back(sb);
	}
	string keyword = tokens[0];

	if(keyword == "create_group" && loginStatus == false){
		cout<<"Response: Please login your account to create a group. "<<endl<<endl;
		return;
	}

	if(keyword == "login" && loginStatus == true){
		cout<<"Response: Already logged in. Please Logout current session! "<<endl<<endl;
		return;
	}

	if(keyword == "join_group" && loginStatus == false){
		cout<<"Response: Please login your account to join a group. "<<endl<<endl;
		return;
	}

	if(keyword == "leave_group" && loginStatus == false){
		cout<<"Response: Please login your account to leave a group. "<<endl<<endl;
		return;
	}

	if(keyword == "requests" && loginStatus == false){
		cout<<"Response: Please login your account to view joining requests. "<<endl<<endl;
		return;
	}

	if(keyword == "accept_request" && loginStatus == false){
		cout<<"Response: Please login your account to accept joining requests. "<<endl<<endl;
		return;
	}

	if(keyword == "upload_file" && loginStatus == false){
		cout<<"Response: Please login your account to uplaod a file. "<<endl<<endl;
		return;
	}

	if(keyword == "list_files" && loginStatus == false){
		cout<<"Response: Please login your account to view all files in a group. "<<endl<<endl;
		return;
	}

	if(keyword == "download_file"){
		if(loginStatus == false){
			cout<<"Response: Please login your account to download files. "<<endl<<endl;
			return;
		}
		else{
			vector<string> tkns = tokens;
			if(tkns.size()!=4){ 	
				cout<<"Response: Please enter valid command. "<<endl<<endl;
				return;
			}
			download(tkns);
			return;
		}
	}

	if(keyword == "join_group" || keyword == "create_group" || keyword == "leave_group" || keyword == "requests" || keyword == "accept_request" || keyword == "list_files"){
		string b = buffer;
		b = b + " " + userID;
		strcpy(buffer, b.c_str());
	}

	if(keyword == "login"){
		string b = buffer;
		b = b + " " + IPORT;
		strcpy(buffer, b.c_str());
	}

	if(keyword == "upload_file"){
		string b = buffer;
		b = b + " " + userID + " " +  IPORT;
		strcpy(buffer, b.c_str());
	}

	string resp = connect_with_tracker(buffer);

	if(keyword == "login"){
		if(resp == "Login Successfull."){
			loginStatus = true;
			userID = tokens[1];
			passWORD = tokens[2];
	
		}
	}
}

void init_server(){
	int sockfd, newsockfd, portno; 
    struct sockaddr_in server_addr, client_addr; 
    socklen_t client_len;

	sockfd = socket(AF_INET, SOCK_STREAM, 0) ;
    if(sockfd < 0) error("Error Opening socket!");

    bzero((char *) &server_addr, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PEER_PORT);
    inet_pton(AF_INET, PEER_IP.c_str(), &server_addr.sin_addr); 

    if(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) <0 ) error("Binding failed!");
    
    
	
    listen(sockfd, 10);
	client_len = sizeof(client_addr);

	while(true)
  	{
		newsockfd = accept(sockfd,(struct sockaddr *) &client_addr, &client_len);
		if(newsockfd<0) break;
		int port = (ntohs(client_addr.sin_port));
		string ip = inet_ntoa(client_addr.sin_addr);
		process_request(newsockfd, port);
	}

 	for(auto itr = thV.begin(); itr != thV.end(); itr++) if(itr->joinable()) itr->join();
   	
}

int main(int argc, char *argv[]) 
{ 

	if(argc < 3) error("Please write valid command");


	IPORT = argv[1];
	vector<string> arguments = split_args(IPORT);
	string ip = arguments[0];
	string port = arguments[1];
	int portno = atoi(port.c_str());
	string tracker_info_file = argv[2];
	save_tracker_details(tracker_info_file);

	PEER_IP = ip;
	PEER_PORT = portno;
	cout<<"IP Address : "<<PEER_IP<<endl;
	cout<<"Port : "<<PEER_PORT<<endl<<endl;
	thread peer_server_thread(init_server);
	peer_server_thread.detach();

	while(true){

		char buffer[BUFFER_SIZE];
		cout<<"Command : ";
		bzero(buffer, BUFFER_SIZE);
		fgets(buffer, BUFFER_SIZE, stdin);
		execute_command(buffer);
	}
	
	return 0; 
} 