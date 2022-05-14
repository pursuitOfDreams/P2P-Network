# P2P-Network
This project was done as a part of the course CS-252 Computer Networks Lab done under the supervision of Prof. Kameshwari Chebrolu, Computer Science Department, IIT Bombay. This was a group project done by [Nikhil Manjrekar]() and [Pinkesh Raghuvanshi]().

# Overview
In this project, we have implemented a Peer to Peer Network for searching and downloading the required files. The overview is as follows:
> There is a network of clients which are interconnected with each other based on a specified topology and each client is provided with information about its directory path and path to configuration file which contains information about its client ID, unique ID, file it requires, Port number of neighbors and their client IDs.

# Table of Content
This repo contains five phases of implementation of the code., with files are named are named in the format `client-phasex.cpp` where `x` is 1,2,3,4 and 5. Description of each phase is as follows: 

### Phase 1
In this phase, the client should just process the input arguments, establish connections with its immediate neighbors and output the following.( Code in `client-phase1.cpp`)


### Phase 2
In this phase, we implemented file searching to a depth of just 1, i.e. check if file is present among immediate neighbors. Each client, for each file it is supposed to search for, it asks its immediate neighbors and find out if the neighbor is the owner. In case there are multiple owners, the one with the smallest unique ID is chosen. ( code in  `client-phase2.cpp`)

### Phase 3
Very similar to Phase2, except that in this phase, we actually transfer the file if the file is found, else nothing happens. The file transfer will happen over the connection that has already been set up in phase 1. The file which is received should show up in the “Downloaded” directory of the receiver. (code in `client-phase3.cpp`)

### Phase 4
This phase is similar to Phase2 except we have increased the search depth to 2. File transfer is not done in this phase. The depth is reported accordingly.File is searched at depth 2 only if the file is not found at depth 1. For a given depth, the tie-breaking rule is the same as phase 2. (code in `client-phase4.cpp`)

### Phase 5
In this phase, we transfer the file at depth 2 also much like Phase3. If the file is not at the immediate neighbor, we use a separate connection to get the file from the node.( code in `client-phase5.cpp`)

