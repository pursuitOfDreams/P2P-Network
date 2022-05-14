// Client side C/C++ program to demonstrate Socket programming
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <filesystem>
#include <thread>
#include <mutex>

using namespace std;
using std::filesystem::directory_iterator;
#define TRUE 1
#define SA struct sockaddr

// Semaphore variables
std::mutex mu;
int readercount = 0;

// step 1 : server will send the information about itself
// step 2 : client will receive the information sent by the server
// step 3 : client will check the information sent
// step 4 : if the file

void server2(vector<int> &client_sockets, string msg)
{

	for (auto i : client_sockets)
	{
		int n = msg.length();
		int sentval = send(i, msg.c_str(), n + 1, 0);
	}
}

void server(int num_of_neighbours, int PORT, string msg)
{

	vector<int> client_sockets;
	int i = 0;
	int master_socket, client_socket;
	int opt = 1;
	struct sockaddr_in address;
	struct sockaddr_in remote_address;

	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
				   sizeof(opt)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(PORT);

	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(master_socket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	while (i < num_of_neighbours)
	{
		// number of connections that will be established will be equal to number of neighbours
		socklen_t addrlen = sizeof(remote_address);
		client_socket = accept(master_socket, (SA *)&remote_address, &addrlen);
	
		// assumption is that accept function is just going to block the control flow of the thread and
		// not the entire program
		if (client_socket != -1)
		{
			client_sockets.push_back(client_socket);
			i++;
		}
	}

	thread communication_thread = thread(server2, std::ref(client_sockets), msg);

	if(communication_thread.joinable()){
		communication_thread.join();
	}
}

void client(vector<int> neighbouring_ports, vector<int> &sockfd)
{
	int number_of_neighbours = neighbouring_ports.size();
	bool sent[number_of_neighbours];

	for (int i = 0; i < number_of_neighbours; i++)
	{

		sent[i] = false;
	}
	struct sockaddr_in servaddress;
	servaddress.sin_family = AF_INET;
	servaddress.sin_addr.s_addr = htonl(INADDR_ANY);

	bool exit = false;
	while (!exit && number_of_neighbours!=0)
	{
		for (int i = 0; i < neighbouring_ports.size(); i++)
		{
			if (!sent[i])
			{
				servaddress.sin_port = htons(neighbouring_ports[i]);
				if (connect(sockfd[i], (SA *)&servaddress, sizeof(servaddress)) != 0)
				{
				}
				else
				{
					sent[i] = true;
				}
			}
		}

		for (int i = 0; i < number_of_neighbours; i++)
		{
			if (sent[i])
			{
				exit = true;
			}
			else
			{
				exit = false;
				break;
			}
		}
	}

	for (int i = 0; i < number_of_neighbours; i++)
	{
		char buffer[10000];
		int readval = recv(sockfd[i], &buffer, 10000, 0);
		cout << buffer << endl;
	}
}
int main(int argc, char const *argv[])
{

	cin.sync();
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 500000;

	fstream file;
	string word, t, q, filename;
	int number_of_neighbours;

	// filename of the file
	filename = argv[1];

	// opening file
	file.open(filename);

	// extracting words from the file
	int client_id;
	file >> client_id;

	int PORT;
	file >> PORT;

	int unique_id;
	file >> unique_id;

	file >> number_of_neighbours;

	vector<int> neighbour_ids(number_of_neighbours);
	vector<int> neighbour_ports(number_of_neighbours);

	for (int i = 0; i < number_of_neighbours; i++)
	{
		file >> neighbour_ids[i];
		file >> neighbour_ports[i];
	}
	int number_of_files;
	file >> number_of_files;

	string s;
	vector<string> files;
	vector<string> my_files;
	vector<pair<string, int>> owner;
	while (file >> s)
	{
		files.push_back(s);
		owner.push_back(make_pair(s, 0));
	}

	for (const auto &file : directory_iterator(argv[2]))
	{
		string s = file.path().filename();
		s.erase(
			remove(s.begin(), s.end(), '\"'),
			s.end());

		my_files.push_back(s);
	}

	for (auto a : my_files)
	{
		cout << a << endl;
	}

	string msg = "Connected to " + to_string(client_id) + " with unique-ID " + to_string(unique_id) + " on port " + to_string(PORT);
	// ===================== file reading done =======================

	struct sockaddr_in servaddress;
	struct sockaddr_in remote_address;
	servaddress.sin_family = AF_INET;
	servaddress.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddress.sin_port = htons(PORT);

	vector<int> sockfd(number_of_neighbours);

	for (int i = 0; i < number_of_neighbours; i++)
	{
		sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);
	}

	// will create two threads : server and client

	thread server_thread = thread(server, number_of_neighbours, PORT, msg);
	thread client_thread = thread(client, neighbour_ports, std::ref(sockfd));

	if (client_thread.joinable())
	{
		client_thread.join();
	}
	if (server_thread.joinable())
		server_thread.join();
}
