#include<iostream>
#include<fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<thread>
#include <unistd.h>
#include<bits/stdc++.h>
#include<stdio.h>
#include<ios> 
#include <mutex>
#include "sha1.h"

std::mutex mtx;
using namespace std;

char * client_port;
char *client_addr;
string  aip;
string  aport;

int maxChunkPeerPort;
int maxChunks = 0;
long long int currFileSize = 0;
int chunkSize = 524288;
int writeCount = 0;

bool fileUpdation = false;

string downloadedFiles = "";

unordered_map<string,vector<string>> group_files;
unordered_map<string,string> fileID_chunks;   //will store chunks in *_* format
unordered_map<string,string> fileID_fPath;
string destinationPath = "";
string group_ID = "";

vector<string> splitString(string str, string delim){
    vector<string> res;

    size_t pos = 0;
    while ((pos = str.find(delim)) != string::npos) {
        string t = str.substr(0, pos);
        res.push_back(t);
        str.erase(0, pos + delim.length());
    }
    res.push_back(str);

    return res;
}

long long int getFileSize(string fileName){
	 int sock;
    //Creation of Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout<<"Socket creation error"<<endl;
		exit(0);
	}

    struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(maxChunkPeerPort); 

    // Convert IPv4 and IPv6 addresses from text to binary
	// form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) 
    {
		cout<<"\nInvalid address/ Address not supported \n";
		exit(-1);
	}

    //Running it as a client
    int client_fd;

    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) 
    {
		cout<<"\nConnection Failed \n";
		exit(-1);
	}
	cout<<"Connection created with port "<<maxChunkPeerPort<<endl;

    fileName = "fileSize*_*" + group_ID + "*_*" + fileName;

    send(sock,&fileName[0],50,0);

	char fileSize[100] ={0};
	read(sock,fileSize,100);

	return stoll(string(fileSize));


}


void getAllChunksInfo(string fileID,int port,unordered_map<int,vector<int>> &piecesInfo){
     int sock;
    //Creation of Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout<<"Socket creation error"<<endl;
		exit(0);
	}

    struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); 

    // Convert IPv4 and IPv6 addresses from text to binary
	// form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) 
    {
		cout<<"\nInvalid address/ Address not supported \n";
		exit(-1);
	}

    //Running it as a client
    int client_fd;

    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) 
    {
		cout<<"\nConnection Failed \n";
		exit(-1);
	}

	cout<<"Connection created with port "<<port<<endl;

    fileID = "info*_*" + fileID;

    send(sock,&fileID[0],50,0);

    char chunks[1000000] = {0};
    recv(sock,chunks,1000000,0);
    cout<<"Chunks Received from "<<port<<" are --> "<<chunks<<endl;

    vector<string> allChunks = splitString(chunks,"*_*");
	cout<<allChunks.size()<<" "<<maxChunks<<" "<<maxChunkPeerPort<<endl;
	if(allChunks.size() > maxChunks){
		cout<<"Yes"<<endl;
		maxChunks = allChunks.size();
		maxChunkPeerPort = port;
	}
    
    
    //Store in piecesInfo map
    for(int i = 0 ; i < allChunks.size() ; i++){
       if(piecesInfo.find(stoi(allChunks[i])) == piecesInfo.end()){
         vector<int> peerPorts;
         peerPorts.push_back(port);
         int temp = stoi(allChunks[i]);
         piecesInfo.insert({stoi(allChunks[i]),peerPorts});
       }
       else{
        vector<int> peerPorts = piecesInfo[stoi(allChunks[i])];
        peerPorts.push_back(port);
        piecesInfo[stoi(allChunks[i])] = peerPorts;
       }
    }
    
    // close(client_fd);

}


void* getChunkFromPeer(vector<int> chunkNumbers,int port,string fileID,int trackerSock){
	for(int c = 0 ; c < chunkNumbers.size(); c++){
	//cout<<"Reached in getChunkFromPeer "<<chunkNumber<<" "<<port<<" "<<fileID<<" "<<endl;
	int sock = 0, valread, client_fd;
	struct sockaddr_in serv_addr;
	
	
	char buffer[chunkSize] = { 0 };

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); //Port of the Peer

	// Convert IPv4 and IPv6 addresses from text to binary
	// form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
		<= 0) {
		printf(
			"\nInvalid address/ Address not supported \n");
	}

	if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr,
				sizeof(serv_addr)))
		< 0) {
		printf("\nConnection Failed \n");
		exit(-1);
		}
	
	int chunkNumber = chunkNumbers[c];
	bool shaMtached = true;
	string request = "chunk*_*";
	request = request + fileID;
	request = request + "*_*";
	request = request + to_string(chunkNumber);

    //cout<<"Requested -->  "<<request<<endl;
	
    send(sock , &request[0] , 50, 0);
	//cout<<"Request Sent for Chunk to the port "<<port<<endl;
    //cout<<"Waiting for reply from Peer"<<endl;

	
	ofstream outfile (destinationPath.c_str() , std::fstream::in | std::fstream::out | std::fstream::binary);
    outfile.seekp((chunkNumber-1)*chunkSize, ios::beg);
	int d = 32;

		int toBeWritten;
		if(chunkNumber == maxChunks){

			long long int x = currFileSize - (currFileSize/524288) * 524288;
			d = ceil(float(x)/float(16384));
			cout<<"Max Small Chunks are "<<d<<endl;
			toBeWritten = x - (x/16384)*16384;
			cout<<"Last Small Chunk "<<toBeWritten<<endl;
		}

		bool shaMatched = true;
		int i = 1;
		while(i <= d){
			
			char sha[41]={0};
			read(sock,sha,41);
			//cout<<"SHA received is"<<sha<<endl;
			send(sock,"SHA",20,0);

			int n = read(sock, buffer, 16384);
			
			if (n <= 0){
				cout << "EXITING" << endl;
				break;
			}

			//cout<<"New SHA is "<<sha1(buffer);
			
			if(sha == sha1(buffer))
			{
				cout<<"SHA Matched for "<<chunkNumber<<endl;
			}
			else{
				shaMtached = false;
			}
			//cout<<"Chunk Read "<<i<<" of "<<chunkNumber<<" and ";
			
			mtx.lock();
			if(i == d && maxChunks == chunkNumber){outfile.write(buffer,toBeWritten); writeCount++;}

			else{
				outfile.write(buffer,n);
			}
			mtx.unlock();
			//cout<<"Chunk Written "<<i<<" of "<<chunkNumber<<endl;
			bzero(buffer, 16384);
			i++;
		}

		
		cout<<"Done Writing Chunk Number "<<chunkNumber<<"and sha matched "<<shaMatched<<endl;
		vector<string> desTokens = splitString(destinationPath,"/");
		string destFileName = desTokens[desTokens.size() - 1];
		string dfileID = group_ID + "*_*" + destFileName;
		cout<<"Destination file Id"<<dfileID<<endl;
		//need to update dest############################################################################3
		if(fileID_chunks.find(dfileID) == fileID_chunks.end()){
			string chunks = to_string(chunkNumber);
			fileID_chunks.insert({dfileID,chunks});
		}
		else{
			string chunks = fileID_chunks[dfileID];
			chunks = chunks + "*_*" + to_string(chunkNumber);
			fileID_chunks[dfileID] = chunks;
		}
		
		mtx.lock();
		if(!fileUpdation){
			fileUpdation = true;
			//upload_file <file_path> <group_id>
			fileID_fPath.insert({dfileID,destinationPath});
			string reqToTracker = "upload_file " + destinationPath + " " + group_ID + " " + dfileID;
			send(trackerSock , &reqToTracker[0], reqToTracker.size(), 0);
			cout<<"Request sent to tracker"<<endl;

			// char reply[50];
			// read(sock,reply,50);
			// if(!strcmp(reply,"Success")){
			// 	cout<<"File uploaded successfully"<<endl;
			// }
			// else{
			// 	cout<<"File not uploaded as "<<reply<<endl;
			// }

		}

		mtx.unlock();

		for(auto itr : fileID_chunks){
			cout<<itr.first<<" --> "<<itr.second<<endl;
		}

		for(auto itr : fileID_fPath){
			cout<<itr.first<<" --> "<<itr.second<<endl;
		}


		outfile.close();
	}
		
	
}

void preProcessingDemand(int sock){
    cout<<"Reached in Preprocessing Step"<<" "<<endl;
    string demand = "";
   
    while(true){

		//cout<<"Count is "<<writeCount<<endl;
        cout<<"Enter command: ";
        getline(cin,demand);
		//cout<<"Command Entered is "<<demand<<endl;
        vector <string> dmdTokens = splitString(demand," ");
        string dmd = dmdTokens[0];

	if(dmd == "logout"){
		send(sock , &demand[0], demand.size(), 0);
		char rep[50] ;
		read(sock,rep,50);
		if(!strcmp(rep,"Success")){
			cout<<rep<<endl;
			continue;
		}
		else{
			continue;
		}
	}

	if(dmd == "stop_share"){
		send(sock , &demand[0], demand.size(), 0);
		
	}

	// list_files <group_id>
	if(dmd == "list_files"){
		send(sock , &demand[0], demand.size(), 0);

		char rep[500] ;
		read(sock,rep,500);

		vector<string> allFiles = splitString(string(rep),"*_*");

		for(int i = 0 ; i < allFiles.size() ; i++){
			cout<<allFiles[i]<<endl;
		}
	}

	if(dmd == "create_user"){
		send(sock , &demand[0], demand.size(), 0);
		//cout<<"Request sent ";
		char reply[50]={0};
		read(sock,reply,50);
		if(!strcmp(reply, "User is created successfully")){
			cout<<"User Created successfully"<<endl;
		}
		else{
			cout<<"User is not created "<<reply<<" "<<endl;
		}
	}

	if(dmd == "login") {
		send(sock , &demand[0], demand.size(), 0);
		//cout<<"Request sent "<<endl;
		char reply[50]={0};
		read(sock,reply,50);
		//cout<<"reply from Tracker is "<<reply<<endl;
		if(!strcmp(reply, "Logged in succesfully")){
			cout<<"Logged in Successfully"<<endl;
			send(sock,client_port,4,0); // We have to replace 9000 with the port number
			//cout<<"Peer Address sent successfully "<<endl;
		}
		else{
			cout<<"Not logged in --"<<reply<<" "<<endl;
		}

	}

	if(dmd == "create_group"){
		// cout<<"Reached in Creating group"<<endl;
		send(sock , &demand[0], demand.size(), 0);
		char reply[50]={0};
		read(sock,reply,50);
		
		if(!strcmp(reply, "Group created successfully")){
			cout<<"Group created successfully"<<endl;
		}
		else{
			cout<<"Group not created  --"<<reply<<endl;
		}
	}

	if(dmd == "join_group"){
		//cout<<"Reached in joining group"<<endl;
		send(sock , &demand[0], demand.size(), 0);

		char reply[100]={0};
		read(sock,reply,100);

		if(!strcmp(reply, "Request sent successfully")){
			cout<<"Request sent successfully"<<endl;
		}
		else{
			cout<<"Request not accepted as  --"<<reply<<endl;
		}

	}

	if(dmd == "list_requests")
	{
		//cout<<"Reached in List Requests"<<endl;

		send(sock , &demand[0], demand.size(), 0);

		char requests[300];

		read(sock,requests,300);

		vector<string> req = splitString(requests,"*_*");

		for(int i = 0 ; i < req.size(); i++){
			cout<<req[i]<<endl;
		}

	}

	if(dmd == "accept_request"){

		//cout<<"Reached in Accept Requests"<<endl;
		send(sock , &demand[0], demand.size(), 0);
		char reply[100];
		read(sock,reply,100);
		if(!strcmp(reply,"Request accepted successfully")){
			cout<<"Request accepted successfully"<<endl;
		}

		else{
			cout<<reply<<endl;
		}



	}
	

	if(dmd == "upload_file"){ //Upload File: upload_file <file_path> <group_id>
		//cout<<"Reached in Uploading file"<<endl;

		send(sock , &demand[0], demand.size(), 0);
		//cout<<"Request sent to tracker"<<endl;
		char reply[50];
		read(sock,reply,50);
		if(!strcmp(reply,"Success")){
			cout<<"File uploaded successfully"<<endl;
		}
		else{
			cout<<"File not uploaded as "<<reply<<endl;
			continue;
		}


		string filePath = dmdTokens[1];
		string grp_ID = dmdTokens[2];
		vector<string> fileTokens = splitString(filePath,"/");
		string fileName = fileTokens[fileTokens.size() - 1];
		//cout<<"File Name is "<<fileName<<endl;

		string fileID = grp_ID + "*_*" + fileName;
		fileID_fPath.insert({fileID,filePath});
		//fileID_chunks
		//cout<<"File size Request received for "<<filePath<<endl;
		ifstream fin(filePath, std::ifstream::binary);
		fin.seekg(0, ios::end);
		long long int fileKASize = fin.tellg();
		long long int totalChunks = ceil(float(fileKASize)/float(512*1024));

		string str = "";
		for(int i = 1 ; i <= totalChunks ; i++){
			str += to_string(i);
			if( i != totalChunks){
				str += "*_*";
			}
		}

		cout<<str<<" "<<str.length()<<endl;
		fileID_chunks[fileID] = str;
		fin.close();

		
	}

	if(dmd == "leave_group"){
		string grp_ID = dmdTokens[1];
		send(sock , &demand[0], demand.size(), 0);
		char reply[50];
		read(sock,reply,50);
		cout<<reply<<endl;

		auto it = fileID_chunks.begin();
			vector<string> rem;

			while(it != fileID_chunks.end())
			{
				//cout<<"Entered in fileId_peer loop"<<endl;
					vector<string> splitted = splitString(it->first,"*_*");
					//cout<<splitted[0]<<endl;
					if(splitted[0] == grp_ID)
					{
						//cout<<"Yes group matched"<<endl;
							//cout<<it->first<<endl;
							string r = it->first;
							++it;
							fileID_chunks.erase(r);
							//cout<<"Erased"<<endl;
					} 
					else{
						++it;
					}		
			}

		auto itr = fileID_fPath.begin();
			

			while(itr != fileID_fPath.end())
			{
				//cout<<"Entered in fileId_peer loop"<<endl;
					vector<string> splitted = splitString(itr->first,"*_*");
					//cout<<splitted[0]<<endl;
					if(splitted[0] == grp_ID)
					{
						//cout<<"Yes group matched"<<endl;
							//cout<<itr->first<<endl;
							string r = itr->first;
							++itr;
							fileID_fPath.erase(r);
							cout<<"Erased"<<endl;
					} 
					else{
						++itr;
					}		
			}

			//cout<<"Printing fileID_chunks  map"<<endl;

			// for( auto it : fileID_chunks){
			// 	cout<<it.first<<" --> ";
			// 	cout<<it.second;
			// 	cout<<endl;
			// }

			// cout<<"Printing fileID_fPath  map"<<endl;

			// for( auto it : fileID_fPath){
			// 	cout<<it.first<<" --> ";
			// 	cout<<it.second;
			// 	cout<<endl;
			// }

			
	}

	if(dmd == "list_groups"){
		send(sock , &demand[0], demand.size(), 0);

		char reply[100];
		read(sock,reply,100);
		//cout<<reply<<endl;

		vector<string> groups = splitString(reply,"*_*");

		for(auto itr : groups){
			cout<<itr<<endl;
		}
	}

	if(dmd == "show_downloads"){

		vector<string> fils = splitString(downloadedFiles,"*_*");

		for(int i = 0 ; i < fils.size() ; i++){
			cout<<fils[i]<<endl;
		}

	}

	if(dmd == "download_file"){
		maxChunks = 0;
        //download_file <group_id> <file_name> <destination_path>
        //cout<<"Reached in Downloading and requested for file "<<endl;
		fileUpdation = false;

		send(sock , &demand[0], demand.size(), 0);

        char reply[100] = {0};

        recv(sock,reply,100,0);

		if(!strcmp(reply,"Please log in first")){
			cout<<reply<<endl;
			continue;
		}
		if(!strcmp(reply,"Group does not exists")){
			cout<<reply<<endl;
			continue;
		}
		if(!strcmp(reply,"File does not exists in this group")){
			cout<<reply<<endl;
			continue;
		}

        cout<<"Reply got from Tracker for the ports to be contacted -- > "<<reply<<endl;
		
		destinationPath = dmdTokens[3];
		group_ID = dmdTokens[1];

		ofstream MyFile(destinationPath.c_str());
		MyFile.close();

		//cout<<"Destination File created successfully "<<endl;


        vector<string> allPorts = splitString(reply,"*_*");

        unordered_map<int,vector<int>> piecesInfo;
        
		string fileID = group_ID + "*_*" + dmdTokens[2];

		//cout<<"FileID for which chunks info to be requested is "<<fileID<<endl;

        for(int i = 0 ; i < allPorts.size() ; i++){
            getAllChunksInfo(fileID,stoi(allPorts[i]),piecesInfo);
        }

        cout<<"Received all info about pieces and Seeder is "<<maxChunkPeerPort<<endl;

		long long int fileSize = getFileSize(dmdTokens[2]);
		//cout<<fileSize<<endl;
		currFileSize = fileSize;

        for (auto it : piecesInfo)
        {
            cout << it.first << "   ";
            for(int i = 0 ; i < it.second.size() ; i++){
                cout<<it.second[i]<<" ";
            }
            cout<<endl;
        }

		cout<<piecesInfo.size()<<endl;

		//###########    Piece Selection Algorithm       ##################
		unordered_map<int,int> chunk_port;
		

		for(auto it : piecesInfo){
			vector<int> ports = it.second;
			int range = ports.size();
            int num = rand() % range;
			//cout<<num;
			chunk_port.insert({it.first,ports[num]});
		}

		// for(auto it : chunk_port){
		// 	cout<<it.first<<" "<<it.second<<endl;
		// }

		unordered_map<int,vector<int>> port_chunks;

		for (auto it : chunk_port)
        {
			if(port_chunks.find(it.second) == port_chunks.end())
			{
				vector<int> chunks;
				chunks.push_back(it.first);
				port_chunks.insert({it.second,chunks});
			}
			else{
				vector<int> chunks = port_chunks[it.second];
				chunks.push_back(it.first);
				port_chunks[it.second] = chunks;
			}
            	
        }


		// for (auto it : port_chunks)
        // {
        //     cout << it.first << "   ";
        //     for(int i = 0 ; i < it.second.size() ; i++){
        //         cout<<it.second[i]<<" ";
        //     }
        //     cout<<endl;
        // }

        vector<thread> threads;
    
        for (auto it : port_chunks)
        {
           // threads.push_back(thread(getChunkFromPeer,it.first,it.second[0],fileID));

			threads.push_back(thread(getChunkFromPeer,it.second,it.first,fileID,sock));
			// thread t1(getChunkFromPeer,it.first,it.second[0],fileID);
			// thread t2(getChunkFromPeer,it.first,it.second[0],fileID);
			// thread t3(getChunkFromPeer,it.first,it.second[0],fileID);
			// thread t (getChunkFromPeer,it.first,it.second[0],fileID);
			// t.join();
			// getChunkFromPeer(it.first,it.second[0],fileID);
        }

        for(int i = 0 ; i < threads.size();i++){
            threads[i].join();
        }

		downloadedFiles = downloadedFiles + "*_*" + dmdTokens[2];

    
	}
    
    }

}


void* runPeerAsClient(){
	
	//**************************Connection Creation with TRACKER**************************************
    int sock;
    //Creation of Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout<<endl<<"Socket creation error"<<endl;
		exit(0);
	}

    struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;

	serv_addr.sin_port = htons(stoi(aport)); // ***************9011 is Tracker Port Number

    // Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) 
    {
		cout<<"\nInvalid address/ Address not supported \n";
		exit(-1);
	}

    //Running it as a client
    int client_fd;

    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) 
    {
		cout<<"\nConnection Failed \n";
		exit(-1);
	}
	cout<<"Connection with Tracker Created"<<endl;
    string demand="";

    preProcessingDemand(sock);

    close(client_fd);

}



void sendChunk(int new_socket,string requestFromClient){

	//cout<<"Request received from Peer "<<requestFromClient<<endl;

	vector<string> reqTokens = splitString(string(requestFromClient), "*_*");

	string fileID = reqTokens[1] + "*_*" + reqTokens[2];
	string filePath = fileID_fPath[fileID];
	//cout<<"File Path is"<<filePath<<endl;
    // fileId will be used to get the path

    string fileName = reqTokens[2];
    //cout<<"Chunk Requested for the file "<<fileName<<endl;
	int chunkNumber = stoi(reqTokens[3]);
    //cout<<"Chunk Number Requested "<<chunkNumber<<endl;

	
	
	ifstream fin(filePath, std::ifstream::binary);
	fin.seekg((chunkNumber-1)*chunkSize, ios::beg);
	//cout<<"File Seeked at Server"<<endl;
	// fin.seekg(0, ios::end);
	// cout<<"**********************************SIZE OF FILE IS "<<fin.tellg()<<endl;

	int i = 1;
	while(i <= 32) {
        if(fin.eof()){break;}
		char data[16384];
		fin.read(data,16384);
		string sha = sha1(data);
		//cout<<"SHA sent is "<<sha<<endl;
		send(new_socket,&sha[0],41,0);
		//cout<<"Data Read -> "<<sizeof(data)<<endl;
		char res[20] = {};
		read(new_socket, res, 20);

		if(!strcmp(res,"SHA")){
			send(new_socket,data,16384,0);
		}
		//cout<<"Data sent as well"<<endl;
		i++;
	}



}

void* serverHelper(int new_socket){

    char req[50] = {0};
    int valRead = read(new_socket , req , 50 );
    cout<<"Request received from the Client is -->"<<req<<endl;
    
    
    vector<string> reqTokens = splitString(req,"*_*");
   

    //*******************Sending Info**************************
    if(reqTokens[0] == "info"){
        string fileIDRequest = reqTokens[1] + "*_*" + reqTokens[2];
        //cout<<"Request received for "<<fileIDRequest<<endl;
		string resp = fileID_chunks[fileIDRequest];
        send(new_socket , &resp[0] , resp.size() , 0);
        //cout<<"Chunks info sent for "<<fileIDRequest<<endl;
    }

	if(reqTokens[0] == "fileSize"){ // ************Path to be specified later on.................................
		string fileRequest = reqTokens[1];
		string fileID = reqTokens[1] + "*_*" + reqTokens[2];
		//cout<<"File id is  "<<fileID<<endl;
        //cout<<"File size Request received for "<<fileRequest<<endl;
		string filePath = fileID_fPath[fileID];
		ifstream fin(filePath, std::ifstream::binary);
		fin.seekg(0, ios::end);
		cout<<"**********************************SIZE OF FILE IS "<<fin.tellg()<<endl;
		string resp = to_string(fin.tellg());
        send(new_socket , &resp[0] , 20 , 0);
        //cout<<"File size sent for "<<fileRequest<<endl;
	}
		
	//****************Sending Chunk Data************************
    if(reqTokens[0] == "chunk"){
        sendChunk(new_socket,req);	
    }
}


int main(int argc, char* argv[]){
	//*******Hardcoding what files I have********
	// file_fPath.insert({"hsahuja.txt","./hsahuja.txt"});
	// file_chunks.insert({"hsahuja.txt","1**2"});
	client_addr = argv[1];
	client_port = argv[2];

	std:: ifstream infile(argv[3]);
	std:: getline(infile,aip);
	std:: getline(infile,aport);

    cout<<"Reached in Server "<<endl;
    int server_fd, valread;
	int new_socket = 0;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8081
	if (setsockopt(server_fd, SOL_SOCKET,
				SO_REUSEADDR | SO_REUSEPORT, &opt,
				sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(atoi(client_port));

	// Forcefully attaching socket to the port 8081
	if (bind(server_fd, (struct sockaddr*)&address,
			sizeof(address))
		< 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

    cout<<"LISTENING....at "<<client_port<<endl;

    thread t(runPeerAsClient);
    

	vector<thread> threads;

	while(true){

		if ((new_socket
			= accept(server_fd, (struct sockaddr*)&address,
					(socklen_t*)&addrlen))
			< 0) {
			perror("accept");
		}

	    cout<<"Connection Accepted"<<endl;

		threads.push_back(thread(serverHelper,new_socket));

	}

	for(int i = 0 ; i < threads.size();i++){
		threads[i].join();
	}

	close(new_socket);
	//closing the listening socket
	shutdown(server_fd, SHUT_RDWR);
	cout<<"Connection Closed Successfully"<<endl;
	t.join();
}