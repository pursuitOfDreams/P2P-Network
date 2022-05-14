# P2P-Network
This project was done as a part of the course CS-252 Computer Networks Lab done under the supervision of Prof. Kameshwari Chebrolu, Computer Science Department, IIT Bombay. This was a group project done by [Nikhil Manjrekar]() and [Pinkesh Raghuvanshi]().

# Overview
In this project, we have implemented a Peer to Peer Network for searching and downloading the required files. The overview is as follows:
> There is a network of clients which are interconnected with each other based on a specified topology and each client is provided with information about its directory path and path to configuration file which contains information about its client ID, unique ID, file it requires, Port number of neighbors and their client IDs.

# Table of Content
This repo contains five phases of implementation of the code., with files are named are named in the format `client-phasex.cpp` where `x` is 1,2,3,4 and 5. Description of each phase is as follows: 

### Phase 1
In this phase, the client should just process the input arguments, establish connections with its immediate neighbors and output the following.( Code in `client-phase1.cpp`)
