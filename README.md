# Peer-to-Peer Group Based File Sharing System

This repository contains code for the P2P /  Mini Bit Torrent which is written in Cpp.

## Lets understand the underlying concept

In short, it is group based file sharing system where users can share, download files from the group they belong to.
Lets understand with a real life scenario.Suppose group of users have a file and some other user needs that file then the reqeusting user can get file
from any of the users which has that file. Now what if file size is big then it will take lots of time a file to get transfered from one user to another user.
And, in real life there can be many users requesting many files from different groups of users which have those files.If we transfer serially then it would be super-slow to work.

So the solution of the problem lies in the fact that if a many users have same file then why not to get the files in separate chunks from many users.

For example.
If a file has 10 chunks, and 5 users have that file then we can get 2 chunks from each user parallely rather then getting whole file from a single user.

But there should be someone which will store information about what file what user has, which chunk to taken from whom. So here comes concept of TRACKER which can track and store the information.

Below is the image of the architecture followed

![image](https://github.com/hsahuja111/MiniBitTorrent-GroupP2P/assets/43888676/f770a49c-89ea-4358-a085-2cad5765fdf1)

## Pre-requisites:
Socket Programming, SHA1 hash, Multi-threading

### Chunk Size Used :

Divide the file into logical “pieces” called chunks wherein the size of each piece is 512KB.

## Data Validation:

There should be some mechanism which would verify that data is not altered over the network.
So we have used SHA1 hash for that. Suppose the file size is 1024KB, then divide it into two pieces of 512KB each and take SHA1 hash of each part, assume that the hashes are HASH1 & HASH2 then the corresponding hash string would be H1H2 , where H1 & H2 are starting 20
characters of HASH1 & HASH2 respectively and hence H1H2 is 40 characters.

SHA of each chunk is verified before and after sending each chunk.


## Architecture Overview:

The Following entities will be present in the network :
1. **Trackers**:
  Maintain information of clients with their files(shared by client) to assist the
  clients for the communication between peers.

2. **Clients**:
  a. User should create an account and register with tracker
  
  b. Login Using The User Credentials
  
  c. Create Group and hence will become owner of that group
  
  d. Fetch list of all Groups in server
  
  e. Request to Join Group
  
  f. Leave Group
  
  g. Accept Group join requests(if owner)
  
  h. Share file across group: Share the filename and SHA1 hash of the
  complete file as well as piecewise SHA1 with the tracker
  
  i. Fetch list of all sharable files in a Group
  
  j. Download file
  
  i. Retrieve peer information from tracker for the file
  
  ii. Core Part: Download file from multiple peers (different pieces of fil  from different peers - piece selection algorithm) simultaneously and all the files which client downloads will be shareable to other users in the same group. Ensure file integrity from SHA1 comparison
  
  k. Show downloads
  
  l. Stop sharing file
  
  m. Stop sharing all files(Logout)
  
  n. Whenever client logins, all previously shared files before logout should
  automatically be on sharing mode


## Working:

  1. Tracker will always be online.
  
  2. Client needs to create an account (userid and password) in order to be part of
  the network.
  
  3. Client can create any number of groups(groupid should be different) and hence
  will be owner of those groups
  
  4. Client needs to be part of the group from which it wants to download the file
  
  5. Client will send join request to join a group
  
  6. Owner Client Will Accept/Reject the request
  
  7. After joining group , client can see list of all the shareable files in the group
  
  8. Client can share file in any group (note: file will not get uploaded to tracker but
  only the <ip>:<port> of the client for that file)
  
  9. Client can send the download command to tracker with the group name and
  filename and tracker will send the details of the group members which are
  currently sharing that particular file
  
  10.After fetching the peer info from the tracker, client will communicate with peers
  about the portions of the file they contain and hence accordingly decide which
  part of data to take from which peer (You need to design your own Piece
  Selection Algorithm)
  
  11. As soon as a piece of file gets downloaded it should be available for sharing
  
  12.After logout, the client should temporarily stop sharing the currently shared files
  till the next login
  
  13. All trackers need to be in sync with each other


## Commands:

  1. **Tracker**
  
  a. Run Tracker: 
  
  To compile code for tracker run :  ```g++ tracker.cpp```
  
  To run code for the tracker run :  ```./a.out tracker.txt```
  
  tracker.txt - Contains ip, port details of the tracker
  
  b. Close Tracker:  ```quit```
    
  2. **Client**
  
  a. Run Client:
  
  To compile code for client run:  ```g++ client.cpp sha1.cpp``` (Compiling code for SHA also)
  
  To run code for client run: ```./client <IP>:<PORT> tracker.txt```
  
  tracker.txt - Contains ip, port details of all the trackers 
  
  b. Create User Account: ```create_user <user_id> <password>```
  
  c. Login: ```login <user_id> <password>```
  
  d. Create Group: ```create_group <group_id>```
  
  e. Join Group: ```join_group <group_id>```
  
  f. Leave Group: ```leave_group <group_id>```
  
  g. List Pending Join: ```list_requests<group_id>```
  
  h. Accept Group Joining Request:
    ```accept_request <group_id> <user_id>```
    
  i. List All Group In Network: ```list_groups```
  
  j. List All sharable Files In Group: ```list_files <group_id>```
  
  k. Upload File: ```upload_file <file_path> <group_id>```
  
  l. Download File:
  
  ```download_file <group_id> <file_name> <destination_path>```
  
  m. Logout: ```logout```
  
  n. Show_downloads: ```show_downloads```
  
  Output format:
  [D] [grp_id] filename
  [C] [grp_id] filename D(Downloading), C(Complete)
  
  o. Stop sharing: ```stop_share <group_id> <file_name>```





