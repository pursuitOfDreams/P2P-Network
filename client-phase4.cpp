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

unordered_map<int, int> sockfd_to_uid;
unordered_map<int, int> clientsocket_to_uid;
unordered_map<string, int> owner_list;
unordered_map<string, int> neighbour_files;
// Semaphore variables
std::mutex mu;
int readercount = 0;

// step 1 : server will send the information about itself
// step 2 : client will receive the information sent by the server
// step 3 : client will check the information sent
// step 4 : if the file
string get_word(string s, int index, string delimiter)
{
	size_t pos = 0;
	string token;
	string ans;
	int i = 0;
	vector<string> words;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		token = s.substr(0, pos);
		words.push_back(token);

		s.erase(0, pos + delimiter.length());
	}
	ans = words[index];
	return ans;
}

vector<string> get_file_list(string s, string delimiter_char)
{
	vector<string> files_available;
	size_t pos = 0;
	string token;
	while ((pos = s.find(delimiter_char)) != std::string::npos)
	{
		token = s.substr(0, pos);
		files_available.push_back(token);
		s.erase(0, pos + delimiter_char.length());
	}

	return files_available;
}

void server2(vector<int> &client_sockets, string msg, vector<string> required_files, vector<string> my_files, int unique_id)
{
	char buffer[10000];
	int readval;
	int sentval;
	int n;

	////////////// 1 //////////
	for (auto i : client_sockets)
	{
		n = msg.length();
		sentval = send(i, msg.c_str(), n + 1, 0);
	}

	////////////// 2 //////////
	vector<string> messages;
	for (auto i : client_sockets)
	{
		memset(buffer, 0, 10000);
		readval = recv(i, &buffer, 10000, 0);
		vector<string> asked_files = get_file_list(string(buffer), " ");
		msg = "";
		for (auto i : asked_files)
		{
			if (std::find(my_files.begin(), my_files.end(), i) != my_files.end())
			{
				msg = msg + to_string(unique_id) + " ";
			}
			else
			{
				msg = msg + "0 ";
			}
		}

		messages.push_back(msg);
	}

	////////////// 3 //////////
	for (int i = 0; i < client_sockets.size(); i++)
	{
		msg = messages[i];
		n = msg.length();

		sentval = send(client_sockets[i], msg.c_str(), n + 1, 0);
	}

	////////////// 4 //////////
	for (auto i : client_sockets)
	{
		memset(buffer, 0, 10000);
		readval = recv(i, &buffer, 10000, 0);
		vector<string> received_files = get_file_list(string(buffer), " ");
		int id = stoi(received_files[0]);
		if (received_files.size() > 1)
		{
			for (int j = 1; j < received_files.size(); j++)
			{
				if (neighbour_files.find(received_files[j]) == neighbour_files.end())
				{
					if (id != 0)
					{
						neighbour_files[received_files[j]] = id;
					}
				}
				else
				{
					if (neighbour_files[received_files[j]] > id)
					{
						neighbour_files[received_files[j]] = id;
					}
				}
			}
		}
	}

	////////////// 5 //////////
	for (int i = 0; i < client_sockets.size(); i++)
	{
		msg = "";
		n = msg.length();
		sentval = send(client_sockets[i], msg.c_str(), n + 1, 0);
	}

	//////////////  6 //////////
	messages.clear();
	for (auto i : client_sockets)
	{
		memset(buffer, 0, 10000);
		readval = recv(i, &buffer, 10000, 0);
		vector<string> asked_files = get_file_list(string(buffer), " ");
		msg = "";
		for (auto i : asked_files)
		{
			if (neighbour_files.find(i)!=neighbour_files.end())
			{
				msg = msg + to_string(neighbour_files[i]) + " ";
			}
			else
			{
				msg = msg + "0 ";
			}
		}
		messages.push_back(msg);
	}

	//////////////// 7 ///////////
	for (int i = 0; i < client_sockets.size(); i++)
	{
		msg = messages[i];
		n = msg.length();
		sentval = send(client_sockets[i], msg.c_str(), n + 1, 0);
	}

}

void server(int num_of_neighbours, int PORT, string msg, vector<string> files, vector<string> my_files, int id)
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

	thread communication_thread = thread(server2, std::ref(client_sockets), msg, files, my_files, id);

	if (communication_thread.joinable())
	{
		communication_thread.join();
	}
}

void client(vector<int> neighbouring_ports, vector<int> &sockfd, vector<string> required_files, vector<string> my_files, int unique_id)
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

	int sentval;
	string msg;
	int n;
	char buffer[10000];
	int readval;

	////////////// 1 //////////
	for (int i = 0; i < number_of_neighbours; i++)
	{
		readval = recv(sockfd[i], &buffer, 10000, 0);
		sockfd_to_uid[sockfd[i]] = stoi(get_word(string(buffer), 5, " "));
		cout << buffer << endl;
	}

	msg = "";
	for (auto file : required_files)
	{
		msg = msg + file + " ";
	}

	vector<string> list_of_files = get_file_list(msg, " ");

	////////////// 2 //////////
	n = msg.length();
	for (int i = 0; i < number_of_neighbours; i++)
	{
		sentval = send(sockfd[i], msg.c_str(), n + 1, 0);
	}

	////////////// 3 //////////
	for (int i = 0; i < number_of_neighbours; i++)
	{
		memset(buffer, 0, 10000);
		readval = recv(sockfd[i], &buffer, 10000, 0);
		vector<string> uids_string = get_file_list(string(buffer), " ");
		vector<int> uids;
		for (auto a : uids_string)
		{
			uids.push_back(stoi(a));
		}
		for (int i = 0; i < list_of_files.size(); i++)
		{
			if (owner_list.find(list_of_files[i]) == owner_list.end())
			{
				if (uids[i] != 0)
				{
					owner_list[list_of_files[i]] = uids[i];
				}
			}
			else
			{
				if (owner_list[list_of_files[i]] > uids[i] && uids[i] != 0)
				{
					owner_list[list_of_files[i]] = uids[i];
				}
			}
		}
	}

	/////

	// for (auto file : required_files)
	// {
	// 	if (owner_list.find(file) == owner_list.end())
	// 	{
	// 		cout << "Found " << file << " at 0 with MD5 0 at depth 0" << endl;
	// 	}
	// 	else
	// 	{
	// 		cout << "Found " << file << " at " << owner_list[file] << " with MD5 0 at depth 1" << endl;
	// 	}
	// }

	msg = to_string(unique_id) + " ";
	for (auto file : my_files)
	{
		msg = msg + file + " ";
	}

	////////////// 4 //////////
	n = msg.length();
	for (int i = 0; i < number_of_neighbours; i++)
	{
		sentval = send(sockfd[i], msg.c_str(), n + 1, 0);
	}

	////////////// 5 //////////
	for (int i = 0; i < number_of_neighbours; i++)
	{
		readval = recv(sockfd[i], &buffer, 10000, 0);
	}

	msg = "";

	for(auto file:required_files){
		if(owner_list.find(file)==owner_list.end()){
			msg = msg + file + " ";
		}
	}

	///////////// 6 ///////////
	n = msg.length();
	for (int i = 0; i < number_of_neighbours; i++)
	{
		sentval = send(sockfd[i], msg.c_str(), n + 1, 0);
	}

	unordered_map<string, int> two_hop_away;
	////////////// 7 /////////////
	list_of_files = get_file_list(msg, " ");
	for (int i = 0; i < number_of_neighbours; i++)
	{
		memset(buffer, 0, 10000);
		readval = recv(sockfd[i], &buffer, 10000, 0);
		vector<string> uids_string = get_file_list(string(buffer), " ");
		vector<int> uids;
		for (auto a : uids_string)
		{
			uids.push_back(stoi(a));
		}
		for (int i = 0; i < list_of_files.size(); i++)
		{
			if (owner_list.find(list_of_files[i]) == owner_list.end())
			{
				if (uids[i] != 0)
				{
					if(two_hop_away.find(list_of_files[i]) == two_hop_away.end()){
						two_hop_away[list_of_files[i]] = uids[i];
					}
					else{
						if(two_hop_away[list_of_files[i]] > uids[i]){
							two_hop_away[list_of_files[i]] = uids[i];
						}
					}
				}
			}
		}
	}
	
	sort(required_files.begin(),required_files.end());

	for(auto file:required_files){
		if(owner_list.find(file)!=owner_list.end()){
			cout << "Found " << file << " at " << owner_list[file] << " with MD5 0 at depth 1" << endl;
		}
		else if(two_hop_away.find(file)!=two_hop_away.end()){
			cout << "Found " << file << " at " << two_hop_away[file] << " with MD5 0 at depth 2" << endl;
		}
		else{
			cout << "Found " << file << " at 0 with MD5 0 at depth 0" << endl;
		}
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

	thread server_thread = thread(server, number_of_neighbours, PORT, msg, files, my_files, unique_id);
	thread client_thread = thread(client, neighbour_ports, std::ref(sockfd), files, my_files, unique_id);

	if (client_thread.joinable())
	{
		client_thread.join();
	}
	if (server_thread.joinable())
		server_thread.join();


}
