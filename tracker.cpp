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
#include<pthread.h>
#include <experimental/filesystem>
using namespace std;
#define BUFFER_SIZE 1024 

unordered_map<string, string> users;
unordered_map<string, set<string> > groups;
unordered_map<string, string> admin;
unordered_map<string, set<string> > groupReqs;
unordered_map<string, unordered_map<string, set<pair<string,string> > > > files;
unordered_map<string, string> namePathMap;
unordered_map<string, long long> nameSizeMap;
map<pair<string, string>, bool> onlineStatus;

string TRACKER_IP;
int TRACKER_PORT;

void error(string s){
    cout<<s<<endl<<endl;
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

string createUser(string userId, string password){

    if(users.find(userId) != users.end()){
        return ("User already exists.");
    }
    else{
        users[userId] = password;
        return ("Registered Successfully."); 
    }

}

string login(string userId, string password, string iport){
    if(users.find(userId) != users.end()){
        if(users[userId] == password){
            pair<string, string> p = make_pair(userId, iport);
            onlineStatus[p] = true;
            return ("Login Successfull.");
        }
        else{
            return ("Please enter correct password.");
        }
    }
    else return ("User Doesn't exist.");
    
}

string createGroup(string groupId, string userId){
    if(groups.find(groupId) != groups.end()){
        return("Group already exist.");
    }
    else{
        groups[groupId].insert(userId);
        admin[groupId] = userId;
        return("Group created sucessfully.");
    }
}

string joinGroup(string groupId, string userId){
    if(groups.find(groupId) != groups.end()){

        if(groups[groupId].find(userId) != groups[groupId].end())
            return ("You're already a member of this group.");
        else if(groupReqs[groupId].find(userId) != groupReqs[groupId].end())
            return ("Your joining request is pending.");
        else{
        groupReqs[groupId].insert(userId);
        return("Group joining request Sent.");
        }
    }
    else return("Group Doesn't exist.");
}

string leaveGroup(string groupId, string userId){
    if(groups.find(groupId) != groups.end()){

        if(groups[groupId].find(userId) != groups[groupId].end()){
            groups[groupId].erase(userId);
            return ("You've left the group.");
        }
        
        return("You're not present in this group.");
        
    }
    else return("Group Doesn't exist.");
}

string logout(string userId, string iport){
    if(users.find(userId) != users.end()){
        pair<string, string> p = make_pair(userId, iport); 
        onlineStatus[p] = false;
        return ("Logout Successfull.");
    }
    else return ("User Doesn't exist.");
    
}

string showRequests(string groupId, string userId){
    if(groups.find(groupId) != groups.end()){
        if(admin[groupId] != userId){
            return("Permission Denied! You're not an admin of this group.");
        }
        else{

            if(groupReqs[groupId].empty()) return("No pending requests.");

            string s = "";
            for (auto uid = groupReqs[groupId].begin(); uid != groupReqs[groupId].end(); ++uid) 
            {
                s += "<" + *uid + ">" + "|";
            }
            string resp = s.substr(0, s.length()-1);
            return resp; 
        }
    }
    else return("Group Doesn't exist.");
}

string acceptRequests(string groupId, string userId, string loggedIn){
    if(groups.find(groupId) != groups.end()){
        
        if(admin[groupId] != loggedIn){
            return("Permission Denied! You're not an admin of this group.");
            
        }
        else{
           
            if(groupReqs[groupId].find(userId) != groupReqs[groupId].end()){
                
                groups[groupId].insert(userId);
                groupReqs[groupId].erase(userId);
                return("Joining request accepted for " + userId);
            }
            else{
                
                return("No request found for " + userId);
            }
        }
    }
    else return("Group Doesn't exist.");
}

string listGroups(){
    
    if(groups.empty()) return("No groups present in this Network.");
    
    string s = "";
    for(auto grp : groups){
        string fs = grp.first.substr(0, grp.first.length()-1);
        s += "<" + fs + ">" + "|";
        
    }
    
    string resp = s.substr(0, s.length()-1);
    return resp; 
}

bool containsFile(string filePath, string groupId){
    if(files.find(groupId) != files.end()){
        if(files[groupId].find(filePath) != files[groupId].end()){
            return true;
        }
    }
    return false;
}

bool isMember(string groupId, string userId){
    if(groups[groupId].find(userId) != groups[groupId].end())
        return true;
    return false;
}

bool isFileExist(string path)
{
    std::ifstream infile(path.c_str());
    return infile.good();
}

string getFileName(string path){
    int i;
    for(i=path.length()-1; i>=0; i--){
        if(path[i] == '/') break;
    }

    if(i<path.length()-1){
        string fileName = path.substr(i+1, path.length());
        namePathMap[fileName] = path;
        return fileName;
    }

    else return "Invalid";
}

void saveFileSize(string path, string fname){
    ifstream f(path,ios::ate|ios::binary);
	long long size = f.tellg(); f.close();

    nameSizeMap[fname] = size;
}

string uploadFile(string filePath, string groupId, string userId, string iport){
    
    if(groups.find(groupId) != groups.end()){

       if(!isMember(groupId, userId)) return("You're not a member of this group.");

       if(!isFileExist(filePath)) return ("You've provided incorrect file path. Please provide correct path."); 

       string gid = groupId.substr(0, groupId.length()-1); 
       string fileName = getFileName(filePath);

       if(fileName == "Invalid") return ("Please provide absolute path.");

       saveFileSize(filePath, fileName);
       
       if(containsFile(fileName, groupId)){
           if(files[groupId][fileName].find(make_pair(userId, iport)) != files[groupId][fileName].end()){
               return ("You're already present in seeder list.");
           }
           else{
               files[groupId][fileName].insert(make_pair(userId, iport));
               string resp = "One more seeder added for file " + fileName + " in " + gid + ".";
               return(resp);
           }
       }
       else{
           files[groupId][fileName].insert(make_pair(userId, iport));
           string resp = "New file uploaded successfully in " + gid + ".";
           return(resp);
       }
        
    }
    else return("Group Doesn't exist.");
}

bool isSeederExist(string groupId, string fileName){
    for(auto seeder : files[groupId][fileName]){
        if(onlineStatus[make_pair(seeder.first, seeder.second)]==true) return true;
    }
    return false;
}

string listFiles(string groupId, string userId){
    if(groups.find(groupId) != groups.end()){

       if(!isMember(groupId, userId)) return("You're not a member of this group.");

       string gid = groupId.substr(0, groupId.length()-1);

       if(files.find(groupId) == files.end()) return ("No files present in " + gid);

        string s = "";
        for(auto file : files[groupId]){
            if(!isSeederExist(groupId, file.first)) continue;
            string fs = file.first;
            s += "<" + fs + ">" + "|";
        }
        if(s == "") return "Currently all seeders are offline.";
        string resp = s.substr(0, s.length()-1);
        return resp;
    }
    else return("Group Doesn't exist.");
}

string seederList(string userId, string groupId, string fileName){

    if(groups.find(groupId)==groups.end()) return "This group doesn't exist.";

    if(!isMember(groupId, userId)) return "You're not a member of this group.";

    if(files[groupId].find(fileName)==files[groupId].end()) return "This file doesn't exist.";

    string resp = "Seed count : " + to_string(files[groupId][fileName].size()) + " and File size : " + to_string(nameSizeMap[fileName]) +  "|";

    bool inside = false;
    for(auto seeder : files[groupId][fileName]){
        if(onlineStatus[make_pair(seeder.first, seeder.second)]==false) continue;
        inside = true;
        resp += seeder.second + "|";
    }
    if(inside==false) return("Sorry for inconvinience! Currently all seeders are offline.");
    resp += to_string(nameSizeMap[fileName]) + "|" + namePathMap[fileName];
    return resp;
}

string processCommand(string command){

  

    vector<string> tokens;
	stringstream ss(command);
    while (ss.good()) {
        string sb; getline(ss, sb, ' ');
        tokens.push_back(sb);
    }

    string cmd = tokens[0];

    if(cmd == "create_user"){
        if(tokens.size()!=3) return("Please enter valid command!");
        string uid =  tokens[1];
        string pass = tokens[2];
        return createUser(uid, pass);
    }

    if(cmd == "login"){
        if(tokens.size()!=4) return("Please enter valid command!");
        string uid =  tokens[1];
        string pass = tokens[2];
        string iport = tokens[3];
        
        return login(uid, pass, iport);
    }

    if(cmd == "create_group"){
        if(tokens.size()!=3) return("Please enter valid command!");
        string gid = tokens[1];
        string uid = tokens[2];
        return createGroup(gid, uid);
    }

    if(cmd == "join_group"){
        if(tokens.size()!=3) return("Please enter valid command!");
        string gid = tokens[1];
        string uid = tokens[2];
        return joinGroup(gid, uid);
    }

    if(cmd == "leave_group"){
        if(tokens.size()!=3) return("Please enter valid command!");
        string gid = tokens[1];
        string uid = tokens[2];
        return leaveGroup(gid, uid);
    }

    if(cmd == "requests"){
        if(tokens.size()!=4) return("Please enter valid command!");
        string gid = tokens[2];
        string uid = tokens[3];
        return showRequests(gid, uid);
    }

    if(cmd == "accept_request"){
        if(tokens.size()!=4) return("Please enter valid command!");
        string gid = tokens[1] + "\n";
        string ud = tokens[2];
        string loggedIn = tokens[3];
        string uid = ud.substr(0, ud.length()-1);
      
        return acceptRequests(gid, uid, loggedIn);
    }

    if(cmd == "list_groups"){
        if(tokens.size()!=2) return("Please enter valid command!");
        string uid = tokens[1];
        return listGroups();
    }

    if(cmd == "list_files"){
        if(tokens.size()!=3) return("Please enter valid command!");
        string gid = tokens[1];
        string uid = tokens[2];
        return listFiles(gid, uid);
    }

    if(cmd == "upload_file"){
        if(tokens.size()!=5) return("Please enter valid command!");
        string path = tokens[1];
        string gid = tokens[2];
        string uid = tokens[3];
        string iport = tokens[4];
        return uploadFile(path, gid, uid, iport);
    }

    if(cmd == "logout"){
        if(tokens.size()!=3) return("Please enter valid command!");
        string uid = tokens[1];
        string iport = tokens[2];
        return logout(uid, iport);
    }

    if(cmd == "seeder"){
        string gid = tokens[1];
        gid += "\n";
        string fileName = tokens[2];
        string uid = tokens[3];
        return seederList(uid, gid, fileName);
    }

    return ("Invalid command!");
}

void* manage_connections(void* p_newsockfd){
    int newsockfd = * (int*)p_newsockfd;
    free(p_newsockfd);
    int n;
    char buffer[BUFFER_SIZE];     
    while(true){
        bzero(buffer, BUFFER_SIZE);
        n = read(newsockfd, buffer, BUFFER_SIZE);
        if(n<0) error("Error on reading!");

        int i = strncmp("closed", buffer, 6);
        if(i == 0){
            cout<<endl<<endl<<"** CONNECTION CLOSED **"<<endl;
            break;
        }

        string command = buffer;
        string resp = processCommand(command);

        
        
        
        bzero(buffer, BUFFER_SIZE);
        
        strcpy(buffer, resp.c_str());
        
        n = write(newsockfd, buffer, strlen(buffer));
        if(n<0) error("Error on writing!");

        i = strncmp("Bye", buffer, 3);
        if(i == 0) break;
        cout<<endl;
    }
    return NULL;
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

int main(int argc, char* argv[]) 
{   
    if(argc != 2) error("Port no. not provided! Program terminated");
    
    int sockfd, newsockfd, portno; 
    struct sockaddr_in server_addr, client_addr; 
    socklen_t client_len;

    string tracker_info_file = argv[1];
    save_tracker_details(tracker_info_file);

    portno = TRACKER_PORT;

    sockfd = socket(AF_INET, SOCK_STREAM, 0) ;
    if(sockfd < 0) error("Error Opening socket!");

    bzero((char *) &server_addr, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
   
    server_addr.sin_port = htons(portno);
    inet_pton(AF_INET, TRACKER_IP.c_str(), &server_addr.sin_addr); 

    if(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) <0 ) error("Binding failed!");

    listen(sockfd, 10);

    while(true){
        client_len = sizeof(client_addr);
        newsockfd = accept(sockfd, (struct sockaddr * ) &client_addr, &client_len);
        if(newsockfd < 0)
        error("Error on Accept");
        
        pthread_t peer;
        int *pclient = (int*)malloc(sizeof(int));
        *pclient = newsockfd;
        pthread_create(&peer, NULL, manage_connections, pclient);
    }

   
    return 0; 
} 