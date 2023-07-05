#include<iostream>
#include<fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<thread>
#include <unistd.h>
#include<bits/stdc++.h>
using namespace std;

/*
1. map<LoginID,password>

2. map<LoggedIDUSERS_loginID,Port_no> //This also indicates that this user is loggedIn..
                                      After logging out we will delete this entry from this map as next time user can have different Port Number

3. map<LoggedIDUSERS_loginID,filesTheyHave(as a Seeder or as a Leecher)> 

4. map<groupID,owner> //This will be used while creating a group

5. map<groupID,filesTheyHaveWithin>

6. map<groupID,members> //Peer should be part of group to access that file

// Unique Identifier for a file at Tracker side is FILE*_*GroupID as one file can be part of multiple group and we dont know which file is getting demanded by PEER
*/

//fileID means grpID + fileName

unordered_map<string,string> creds; //loginId --> password
map<string,vector<string>> fileID_peer; //fileID --> PeerAddress;
//unordered_map<string,string> fileID_path; //fileID --> filePath

unordered_map<string,string> loginId_peer; // loginId --> PeerAddress
unordered_map<string,string> peer_loginId;

unordered_map<string,string> grp_owner; // group --> owner(loginId)
unordered_map<string,vector<string>> grp_members; // group --> members(loginId)
unordered_map<string,vector<string>> grp_files; // group --> allFiles

unordered_map<string,vector<string>> grp_JoinRequests; //group --> allPendingRequests
vector<string> loggedIn;

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


void* peerRequestHandler(int new_socket){

	 string curr_loginID = "";

	while(true) {
		
		char requestFromClient[524288] = {0};
		cout<<"Waiting for read "<<endl;
		int valread = read(new_socket, requestFromClient ,524288);
		if(valread <= 0){break;}
		//cout<<"Total Request from Client is -->"<<requestFromClient<<endl;
		
		vector<string> reqTokens = splitString(string(requestFromClient) , " ");

		string request = reqTokens[0];

		//cout<<"Request from Client is "<<request<<endl;
		
		if(request == "create_user") {
			cout<<"Reached in create user"<<endl;
			if(reqTokens.size() != 3){
				if (send(new_socket, "Enter Correct Command" , 50 , 0) == -1) {
				perror("[-]Error in sending status.");
				continue;
				}
				continue;
			}

			if(creds.find(reqTokens[1]) != creds.end()){
				send(new_socket, "Login Id already exists" , 50 , 0);
				continue;
			}

			creds.insert({reqTokens[1],reqTokens[2]});

			if (send(new_socket, "User is created successfully" , 50 , 0) == -1) {
				perror("[-]Error in sending status.");
				continue;
			}

			cout<<creds[reqTokens[1]]<<endl;
			cout<<"************************************************"<<endl;

		}

		if(request == "login") {              //login <user_id> <password>
			cout<<"Reached in login"<<endl;
			if(reqTokens.size() != 3){
				if (send(new_socket, "Enter Correct Command" , 50 , 0) == -1) {
				perror("[-]Error in sending status.");
				continue;
				}
				continue;
			}

			string id = reqTokens[1];

			if(creds.find(id) == creds.end()){
				if (send(new_socket, "User does not exist" , 50 , 0) == -1) {
					perror("[-]Error in sending status.");
					continue;
			    }
				continue;
			}

			else{
				string clientPassword = reqTokens[2];
				if(creds[id] != clientPassword){
					if (send(new_socket, "Wrong Password" , 50 , 0) == -1) {
					perror("[-]Error in sending status.");
				    }
					continue;
				}

				else{
					cout<<"Password matched"<<endl;
					if(curr_loginID.size() != 0 ){
						send(new_socket, "You are already logged in" , 50 , 0);
						continue;
					}

					// if(find(loggedIn.begin(),loggedIn.end(),id) != loggedIn.end()){
					// 	send(new_socket, "You are already logged in" , 50 , 0);
					// 	continue;
					// }


					curr_loginID = id;
					loggedIn.push_back(curr_loginID);
					
					if (send(new_socket, "Logged in succesfully" , 50 , 0) == -1) {
						perror("[-]Error in sending status.");
						continue;
				    }
					

					cout<<"Logged In at Tracker side"<<endl;

					char peerAddress[20];
				    int valread = read(new_socket, peerAddress , 20);
					cout<<peerAddress<<endl;
				    loginId_peer.insert({curr_loginID,peerAddress});
					peer_loginId.insert({peerAddress,curr_loginID});
				    cout<<loginId_peer[curr_loginID]<<endl;
				
			    }
				
			}

			cout<<"************************************************"<<endl;
			
		}
			
		if(request == "upload_file"){ // upload_file <file_path> <group_id>
		 //cout<<"Reached in uploading file"<<endl;

		 if(curr_loginID.size() == 0){
			send(new_socket, "Please log in first" , 50 , 0);
			continue;
		 }


		 //Need to check whether the group in which we are trying to upload exists or not

			string filePath = reqTokens[1];
			string grp_ID =   reqTokens[2];

		 if(grp_owner.find(grp_ID) == grp_owner.end()){
			send(new_socket, "Group Does not exists" , 50 , 0);
			continue;
		 }

		 vector<string> members = grp_members[grp_ID];

		 if(find(members.begin(),members.end(),curr_loginID) == members.end()){
			send(new_socket, "You are not part of this group" , 50 , 0);
			continue;
		 }

			vector<string> fileTokens = splitString(filePath,"/");
			string fileName = fileTokens[fileTokens.size() - 1];
			//cout<<"File Name is "<<fileName<<endl;
			string fileID = grp_ID + "*_*" + fileName;
			//cout<<"FileID is "<<fileID<<endl;
			
			
			if(grp_files.find(grp_ID) == grp_files.end()){
				vector<string> files;
				files.push_back(fileName);
				grp_files.insert({grp_ID,files});
			}
			else{
				vector<string> files = grp_files[grp_ID];
				files.push_back(fileName);
				grp_files[grp_ID] = files;
			}



			if(fileID_peer.find(fileID) == fileID_peer.end()){
				string peer;
				peer = loginId_peer[curr_loginID];
				//cout<<peer<<" "<<endl;
				//cout<<fileName<<endl;
				vector<string> peers;
				peers.push_back(peer);
				fileID_peer.insert({fileID,peers});
				//cout<<fileID_peer[fileID];
			}

			else{
				vector<string> peers = fileID_peer[fileID];
				if(find(peers.begin(),peers.end(),loginId_peer[curr_loginID]) != peers.end()){
					send(new_socket, "File already present" , 50 , 0);
			        continue;
				}

				peers.push_back(loginId_peer[curr_loginID]);
				fileID_peer[fileID] = peers;
			}

			send(new_socket, "Success" , 50 , 0);

			//cout<<"Printing group files vector "<<endl;
			vector<string> filesUnderGrp = grp_files[grp_ID];
			// for(int i = 0 ; i < filesUnderGrp.size();i++){
			// 	cout<<filesUnderGrp[i]<<" ";
			// }
			// cout<<endl;

			vector<string> peersUnderFile = fileID_peer[fileID];
			// cout<<"Printing fileID_ peer vector "<<endl;
			// for(int i = 0 ; i < peersUnderFile.size();i++){
			// 	cout<<peersUnderFile[i]<<" ";
			// }
			// cout<<endl;

			cout<<"************************************************"<<endl;

		}

		if(request == "download_file"){ //download_file <group_id> <file_name> <destination_path>
			//Various checks to take care(groups) ..........to be done..........
			//cout<<"Reached in downloading at Tracker side"<<endl;

			if(curr_loginID.size() == 0 ){
				send(new_socket, "Please log in first" , 50 , 0);
				continue;
		    }

			string grp_ID = reqTokens[1];
			string fileName = reqTokens[2];
			string fileId = grp_ID + "*_*" +  fileName;

			if(grp_owner.find(grp_ID) == grp_owner.end()){
				send(new_socket, "Group does not exists" , 50 , 0);
				continue;
			}
			
			
			if(fileID_peer.find(fileId) == fileID_peer.end()){
				send(new_socket,"File does not exists in this group",50,0);
				cout<<"Response that file does not exits sent to Peer"<<endl;
				continue;
			}

			
			cout<<fileId<<endl;
			vector<string> peers = fileID_peer[fileId];

			string resp = "";
			for(int i = 0 ; i < peers.size() ; i++){
				string peerlogin = peer_loginId[peers[i]];
				cout<<peerlogin<<endl;
			 if(find(loggedIn.begin(),loggedIn.end(),peerlogin) != loggedIn.end())
			 {
				resp = resp + peers[i];
				if(i != peers.size() - 1){
					resp = resp + "*_*";
				}
			 }
			}

			cout<<resp<<endl;
			send(new_socket,&resp[0],100,0);
			cout<<"Response sent to peer --> "<<resp<<endl;
			
			char upload[100];
			read(new_socket, upload , 100);
			//cout<<upload<<endl;

			vector<string> tokens = splitString((string)upload," ");
			string dfileID =  tokens[3];
			//cout<<"file id is "<<dfileID<<endl;

			if(fileID_peer.find(dfileID) == fileID_peer.end()){
				string peer;
				peer = loginId_peer[curr_loginID];
				//cout<<peer<<" "<<endl;
				vector<string> peers;
				peers.push_back(peer);
				fileID_peer.insert({dfileID,peers});
				//cout<<fileID_peer[fileID];
			}

			else{
				vector<string> peers = fileID_peer[dfileID];
				peers.push_back(loginId_peer[curr_loginID]);
				fileID_peer[dfileID] = peers;
			}
			//cout<<"Uploaded"<<endl;
			// send(new_socket,"Success",100,0);

			vector<string> peersUnderFile = fileID_peer[dfileID];
			// cout<<"Printing fileID_ peer vector "<<endl;
			// for(int i = 0 ; i < peersUnderFile.size();i++){
			// 	cout<<peersUnderFile[i]<<" ";
			// }
			// cout<<endl;


		}

		if(request == "create_group"){ // create_group <group_id>
				 if(curr_loginID.size() == 0){
					send(new_socket, "Please log in first" , 50 , 0);
					continue;
		 		 }

				string grp_ID = reqTokens[1];

				if(grp_owner.find(grp_ID) != grp_owner.end()){
					send(new_socket,"Group Already exists ",50,0);
					continue;
				}

				grp_owner.insert({grp_ID , curr_loginID});
				vector<string> members;
				members.push_back(curr_loginID);
			    grp_members.insert({grp_ID , members});
				cout<<"Group Created "<<grp_ID<<" ->";
				send(new_socket,"Group created successfully",50,0);
				// for(int i = 0 ; i < members.size(); i++)
				// {
				// 	cout<<members[i]<<" "<<endl;
				// }
		}

		if(request == "join_group"){ // join_group <group_id>

		 if(curr_loginID.size() == 0){
			send(new_socket, "Please log in first" , 50 , 0);
			continue;
		 }
			 string grp_ID = reqTokens[1];
			 //cout<<"Group Id is "<<grp_ID<<endl;

			 if(grp_owner.find(grp_ID) == grp_owner.end()){
				send(new_socket,"Group does not exist",50,0);
				continue;
			 }
			
			//  cout<<"Current logged in user is "<<curr_loginID<<endl;
			//  cout<<"Group owner of this group is "<<grp_owner[grp_ID]<<endl;

			//  if(grp_owner[grp_ID] == curr_loginID){
			// 	send(new_socket,"You are already owner of this group",50,0);
			// 	continue;
			//  }

			vector<string> members = grp_members[grp_ID];
			if( find(members.begin(), members.end(), curr_loginID) != members.end() ){
				send(new_socket,"You are already part of this group",50,0);
				continue;
			}
			

			 vector<string> requests; 
			 if(grp_JoinRequests.find(grp_ID) == grp_JoinRequests.end()){
				requests.push_back(curr_loginID);
				grp_JoinRequests.insert({grp_ID,requests});
			 }
			 else{
				requests = grp_JoinRequests[grp_ID];
				if(find(requests.begin(), requests.end(), curr_loginID) != requests.end()){
					send(new_socket,"You have already requested to be part of this group",100,0);
					continue;
				}
				requests.push_back(curr_loginID);
				grp_JoinRequests[grp_ID] = requests;
			 }

			 send(new_socket,"Request sent successfully",50,0);
			
			 requests = grp_JoinRequests[grp_ID];
			 for(int i = 0 ; i < requests.size() ; i++){
				cout<<requests[i]<<" ";
			 }
			 cout<<endl;
		}

		if(request == "leave_group"){ // leave_group <group_id>
		//cout<<"Reached in leave group"<<endl;

		 if(curr_loginID.size() == 0){
			send(new_socket, "Please log in first" , 50 , 0);
			continue;
		 }

		

			string grp_ID = reqTokens[1];

			if(grp_owner.find(grp_ID) == grp_owner.end()){
				send(new_socket," This Group does not exist",50,0);
				continue;
			}

			vector<string> members = grp_members[grp_ID];

			if( find(members.begin(), members.end(), curr_loginID ) == members.end() ){
				send(new_socket,"You are not part of this group",50,0);
				continue;
			}

			

			if(members.size() == 1){
				grp_members.erase(grp_ID);
				grp_owner.erase(grp_ID);
			}

			else{
				auto itr = find(members.begin(), members.end(), curr_loginID );
			    members.erase(itr);
				grp_members.erase(grp_ID);
				grp_members.insert({grp_ID,members});
				// who is the next owner neeed to be handled      ##############*************************#########
			}

			string peerPort = loginId_peer[curr_loginID];

			//cout<<"Port of Current logged in is "<<peerPort<<endl;

			// for( auto it : fileID_peer){
			// 	cout<<it.first<<" --> ";
			// 	vector<string> peers = it.second;
			// 	for(int i = 0 ; i < peers.size(); i++){
			// 		cout<<peers[i]<<" ";
			// 	}
			// }

			auto it = fileID_peer.begin();
			

			while(it != fileID_peer.end()){
				// cout<<"Entered in fileId_peer loop"<<endl;
				vector<string> peers = it->second;
				// cout<<"r"<<endl;
				if(find(peers.begin(),peers.end(),peerPort) != peers.end()){
					// cout<<"Yes peer found"<<endl;
					vector<string> splitted = splitString(it->first,"*_*");
					//cout<<splitted[0]<<endl;
					if(splitted[0] == grp_ID)
					{
						// cout<<"Yes group matched"<<endl;
						if(peers.size() == 1){
							// cout<<"Yes size is 1"<<endl;
							//cout<<it->first<<endl;
							string r = it->first;
							++it;
							fileID_peer.erase(r);
							// cout<<"Erased"<<endl;
						} 
						else{
							auto loc = find(peers.begin(),peers.end(),peerPort);
							peers.erase(loc);
							fileID_peer[it->first] = peers;
							++it;
						}

					}
					else{
						++it;
					}
					
					
				}
				else{
					++it;
				}

				


			}

			send(new_socket,"Success",50,0);

			/*cout<<"Printing fileID peers map"<<endl;

			for( auto it : fileID_peer){
				cout<<it.first<<" --> ";
				vector<string> peers = it.second;
				for(int i = 0 ; i < peers.size(); i++){
					cout<<peers[i]<<" ";
				}
				cout<<endl;
			}

			cout<<"Printing group_members map"<<endl;

			for(auto it: grp_members){
				cout<<it.first<<" --> ";
				vector<string> members = it.second;
				for(int i = 0 ; i < members.size(); i++){
					cout<<members[i]<<" ";
				}
				cout<<endl;
			}

			cout<<"Printing group_owners map"<<endl;

			for(auto it: grp_owner){
				cout<<it.first<<" --> ";
				cout<<it.second<<endl;
				
			}

			
*/


		}

		if(request == "list_files"){

			set<string> files;
			string grp = reqTokens[1];

			for(auto r : fileID_peer){
				vector<string> spl = splitString(r.first,"*_*");
				if(spl[0] == grp){
					files.insert(spl[1]);
				}
			}

			string resp = "";

			set<int>::iterator itr;
   
			for (auto itr : files)
			{
				resp = resp + itr + "*_*";
			}

			resp = resp.substr(0,resp.size() - 3);
			cout<<resp<<endl;

			send(new_socket,&resp[0],100,0);
			 


		}

		if(request == "list_requests"){  //list_requests <group_id>


			if(curr_loginID.size() == 0){
				send(new_socket, "Please log in first" , 50 , 0);
				continue;
			}

			string grp_ID = reqTokens[1];
			vector<string> requests = grp_JoinRequests[grp_ID];
			string responseString = "";
			for(int i = 0 ; i < requests.size() ; i++){
				responseString = responseString + requests[i];
				if(i != requests.size() - 1){
					responseString = responseString + "*_*";
				}
			}
			//cout<<responseString<<endl;

			send(new_socket,&responseString[0],300,0);
			cout<<endl;
		}

		if(request == "accept_request"){// accept_request <group_id> <user_id>

			if(curr_loginID.size() == 0){
				send(new_socket, "Please log in first" , 50 , 0);
				continue;
		    }

			
			string grp_ID = reqTokens[1];
			string usr_ID = reqTokens[2];

			if(grp_owner[grp_ID] != curr_loginID){
				send(new_socket,"You are not permitted to accept requests in this group",100,0);
				continue;
			}

			vector<string> members = grp_members[grp_ID];
			members.push_back(usr_ID);
			grp_members[grp_ID] = members;

			vector<string> requests = grp_JoinRequests[grp_ID];
			//cout<<"Number of requests are "<<requests.size()<<endl;
			auto itr = find(requests.begin(), requests.end(), usr_ID );
			//cout<<"Iterator "<<*itr<<endl;
			requests.erase(itr);
			//cout<<"Erased"<<endl;
			//cout<<"Number of requests now are "<<requests.size()<<endl;
			grp_JoinRequests[grp_ID] = requests;
			

			send(new_socket,"Request accepted successfully",100,0);

			

		}

		if(request == "list_groups"){ // list_groups
		//cout<<"Reached in list_groups"<<endl;

			if(curr_loginID.size() == 0){
				send(new_socket, "Please log in first" , 50 , 0);
				continue;
			}

			string resp = "";

			for(auto itr : grp_owner){
				resp = resp + itr.first + "*_*";
			}

			resp = resp.substr(0,resp.size() - 3);
			cout<<resp<<endl;

			send(new_socket,&resp[0],100,0);
			


		}

		if(request == "logout"){ // logout
			//cout<<"Reached in logout"<<endl;

			if(curr_loginID.size() == 0){
				send(new_socket, "Please log in first" , 50 , 0);
				continue;
			}

			//cout<<"Current logged in is "<<curr_loginID<<endl;
			string rem = curr_loginID;
			auto r = find(loggedIn.begin(),loggedIn.end(),rem);
			cout<<*r<<endl;
			if(loggedIn.size())
			loggedIn.erase(r);

			curr_loginID = "";

			//cout<<"Left are"<<endl;
			//cout<<loggedIn.size()<<endl;
			for(int i = 0 ; i < loggedIn.size() ; i++){
				cout<<loggedIn[i]<<" ";
			}

			send(new_socket, "Success" , 50 , 0);
			continue;

		}

		// if(request == "show_downloads") { // show_downloads
		// 	if(curr_loginID.size() == 0){
		// 		send(new_socket, "Please log in first" , 50 , 0);
		// 		continue;
		// 	}

		// }

		if(request == "stop_share") { // stop_share <group_id> <file_name>

			if(curr_loginID.size() == 0){
				send(new_socket, "Please log in first" , 50 , 0);
				continue;
		    }

			string fileid = reqTokens[1] + "*_*" + reqTokens[2];
			string currPeer = loginId_peer[curr_loginID];

			auto it = fileID_peer.begin();
			
			while(it != fileID_peer.end()){
				
				vector<string> peers = it->second;

				if(it->first == fileid && find(peers.begin(),peers.end(),currPeer) != peers.end()){
					//cout<<"yes all conditions matched"<<endl;

					if(peers.size() == 1){
						//cout<<it->first<<endl;
						string r = it->first;
						++it;
						fileID_peer.erase(r);
					}

					else{
						auto loc = find(peers.begin(),peers.end(),currPeer);
						peers.erase(loc);
						fileID_peer[it->first] = peers;
						++it;
					}
				}

				else{
					++it;
				}

			}

			// for(auto m : fileID_peer){
			// 	//cout<<m.first<<" --> ";
			// 	vector<string> peers = m.second;
			// 	for(int i = 0 ; i < peers.size() ; i++){
			// 		cout<<peers[i]<<" ";
			// 	}
			// 	cout<<endl;
			// }

		}

		else{
			continue;
		}

		


	}

}

	


int main(int argc, char* argv[]){
	// creds.insert({"hsahuja","hsahuja"});
	// creds.insert({"jsahuja","jsahuja"});
	const char *ip;
	std:: ifstream infile(argv[1]);
	string aip;
	string aport;
	std:: getline(infile,aip);

	std:: getline(infile,aport);
	const char* port;
	ip = aip.c_str();
	port = aport.c_str();

	// loginId_peer.insert({"hsahuja","9000"});
	// loginId_peer.insert({"jsahuja","9001"});
    int server_fd, valread;
	int new_socket = 0;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024] = { 0 };
	

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		
	}

	// Forcefully attaching socket to the port 8081
	if (setsockopt(server_fd, SOL_SOCKET,
				SO_REUSEADDR | SO_REUSEPORT, &opt,
				sizeof(opt))) {
		perror("setsockopt");
		
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(atoi(port));

	// Forcefully attaching socket to the port 8081
	if (bind(server_fd, (struct sockaddr*)&address,
			sizeof(address))
		< 0) {
		perror("bind failed");
		
	}

	if (listen(server_fd, 3) < 0) {
		perror("listen");
		
	}

	cout<<"TRACKER Listening at 9011........"<<endl;

	vector<thread> threads;

	int i = 0;

	while(true){

		if ((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))< 0) {
			perror("accept");
			exit(-1);
	    }

		i++;
		cout<<"Request accepted for  "<<i<<endl;
		
		threads.push_back(thread(peerRequestHandler,new_socket));
	
	}

	for(int i = 0; i < threads.size();i++){
		threads[i].join();
	}
	
	close(new_socket);
	shutdown(server_fd, SHUT_RDWR);
	return 0;

}