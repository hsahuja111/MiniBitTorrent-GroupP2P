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

#### Chunk Size Used
