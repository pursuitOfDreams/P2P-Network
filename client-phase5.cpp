// Client side C/C++ program to demonstrate Socket programming
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <filesystem>
#include <thread>
#include <mutex>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>

using namespace std;
using std::filesystem::directory_iterator;
#define TRUE 1
#define SA struct sockaddr
vector<string> found;
unordered_map<int, int> sockfd_to_uid;
unordered_map<int, int> uid_to_sockfd;
unordered_map<int, int> clientsocket_to_uid;
unordered_map<string, int> owner_list;
unordered_map<int, int> one_hop_away;
unordered_map<int, int> uid_to_port;
unordered_map<string, int> neighbour_files;
int accept_requests = 0;
int connection_to_be_sent = 0;
bool move_ahead = false;

// Semaphore variables
std::mutex mu;
int readercount = 0;
unsigned char result[MD5_DIGEST_LENGTH];

// step 1 : server will send the information about itself
// step 2 : client will receive the information sent by the server
// step 3 : client will check the information sent
// step 4 : if the file

// Get the size of the file by its file descriptor
unsigned int get_size_by_fd(int fd)
{
	struct stat statbuf;
	if (fstat(fd, &statbuf) < 0)
		exit(-1);
	return statbuf.st_size;
}

unsigned char *get_MD5(string file_path)
{
	int file_descript;
	unsigned int file_size;
	char *file_buffer;
	memset(result, 0, MD5_DIGEST_LENGTH);

	file_descript = open(file_path.c_str(), O_RDONLY);
	if (file_descript < 0)
		exit(-1);

	file_size = get_size_by_fd(file_descript);
	// printf("file size:\t%lu\n", file_size);

	file_buffer = (char *)mmap(0, file_size, PROT_READ, MAP_SHARED, file_descript, 0);
	MD5((unsigned char *)file_buffer, file_size, result);
	munmap(file_buffer, file_size);

	return result;

	// print_md5_sum(result);
	// printf("  %s\n", argv[1]);
}

string get_md5_sum(string file_path)
{
	int i;
	unsigned char *md = get_MD5(file_path);
	string result;
	char buf[32];
	for (i = 0; i < MD5_DIGEST_LENGTH; i++)
	{
		sprintf(buf, "%02x", md[i]);
		result.append(buf);
	}
	return result;
}

int send_server_image(int sock, string path)
{
	int n = 0;
	int siz = 0;
	FILE *picture;
	char buf[50];
	picture = fopen(path.c_str(), "rb");
	fseek(picture, 0, SEEK_END);
	siz = ftell(picture);

	sprintf(buf, "%d", siz);
	if ((n = send(sock, buf, sizeof(buf), 0)) < 0)
	{
		perror("send_size()");
		exit(errno);
	}

	char Sbuf[siz];
	fseek(picture, 0, SEEK_END);
	siz = ftell(picture);
	fseek(picture, 0, SEEK_SET); // Going to the beginning of the file

	while (!feof(picture))
	{
		n = fread(Sbuf, sizeof(char), siz, picture);
		if (n > 0)
		{											/* only send what has been read */
			if ((n = send(sock, Sbuf, siz, 0)) < 0) /* or (better?) send(sock, Sbuf, n, 0) */
			{
				perror("send_data()");
				exit(errno);
			}
		}
		/* memset(Sbuf, 0, sizeof(Sbuf)); useless for binary data */
	}

	return 0;
}

int recv_client_image(int sock, string path)
{

	int n = 0;
	char buf[50];
	int siz = 0;
	if ((n = recv(sock, &buf, sizeof(buf), 0) < 0))
	{
		perror("recv_size()");
		exit(errno);
	}
	siz = atoi(buf);

	char Rbuffer[siz];
	n = 0;
	int to_be_received = siz;
	int temp = 0;
	while (to_be_received > 0)
	{
		if ((n = recv(sock, Rbuffer + temp, sizeof(Rbuffer), 0)) < 0)
		{
			perror("recv_size()");
			exit(errno);
		}
		// cout << n << endl;
		temp += n;
		to_be_received = to_be_received - n;
	}
	FILE *image;
	image = fopen(path.c_str(), "wb");
	fwrite(Rbuffer, sizeof(char), sizeof(Rbuffer), image);
	fclose(image);

	return 0;
}

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

void server2(vector<int> &client_sockets, string msg, vector<string> required_files, vector<string> my_files, int unique_id, string my_path, int &master_socket)
{
	char buffer[10000];
	int readval;
	int sentval;
	int n;
	string greeting = msg;

	//  Sent Greetings
	for (auto i : client_sockets)
	{
		n = msg.length();
		sentval = send(i, msg.c_str(), n + 1, 0);
	}

	// //////////////// 2 ////////////////
	// received list of files that the client want
	vector<string> messages;
	for (auto i : client_sockets)
	{
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

	// sent the unique id if I have the file that the client wanted
	for (int i = 0; i < client_sockets.size(); i++)
	{
		msg = messages[i];
		n = msg.length();

		sentval = send(client_sockets[i], msg.c_str(), n + 1, 0);
	}

	// =================== 3rd phase completed ===================
	////////////// 4 //////////
	// received the list of files that the connected client has

	unordered_map<int, int> uid_to_client_socket;
	for (auto i : client_sockets)
	{
		memset(buffer, 0, 10000);
		readval = recv(i, &buffer, 10000, 0);
		vector<string> received_files = get_file_list(string(buffer), " ");
		int id = stoi(received_files[0]);
		uid_to_client_socket[id] = i;
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
	// sent an empty message

	for (int i = 0; i < client_sockets.size(); i++)
	{
		msg = "";
		n = msg.length();
		sentval = send(client_sockets[i], msg.c_str(), n + 1, 0);
	}

	// cout<<"5th stage completed"<<endl;

	// //////////////  6 //////////
	// // received the list of files that client wants from two hop away neighbours
	messages.clear();
	for (auto i : client_sockets)
	{
		memset(buffer, 0, 10000);
		readval = recv(i, &buffer, 10000, 0);
		vector<string> asked_files = get_file_list(string(buffer), " ");
		msg = "";
		for (auto i : asked_files)
		{
			if (neighbour_files.find(i) != neighbour_files.end())
			{
				msg = msg + to_string(neighbour_files[i]) + " " + to_string(uid_to_port[neighbour_files[i]]) + " ";
			}
			else
			{
				msg = msg + "0 0 ";
			}
		}
		messages.push_back(msg);
	}

	// // sent the uid and port of
	for (int i = 0; i < client_sockets.size(); i++)
	{
		msg = messages[i];
		n = msg.length();
		sentval = send(client_sockets[i], msg.c_str(), n + 1, 0);
	}

	unordered_map<int, int> count;
	// // obtaining count
	for (int i = 0; i < client_sockets.size(); i++)
	{
		memset(buffer, 0, 10000);
		readval = recv(client_sockets[i], &buffer, 10000, 0);
		vector<string> received = get_file_list(string(buffer), " ");
		int uid = stoi(received[0]);
		for (int j = 1; j < received.size(); j++)
		{
			if (count.find(stoi(received[j])) == count.end())
			{
				count[stoi(received[j])] = 1;
			}
			else
			{
				count[stoi(received[j])]++;
			}
		}
	}

	for (int i = 0; i < client_sockets.size(); i++)
	{
		int count_to_be_sent;
		int id = 0;
		for (auto j : uid_to_client_socket)
		{
			if (client_sockets[i] == j.second)
			{
				id = j.first;
			}
		}
		if (count.find(id) == count.end())
		{
			count_to_be_sent = 0;
		}
		else
		{
			count_to_be_sent = count[id];
		}

		msg = to_string(count_to_be_sent);

		n = msg.length();

		sentval = send(client_sockets[i], msg.c_str(), n + 1, 0);
	}

	while (!move_ahead)
	{
	};

	// // assuming count is a variable that stores number of clients which will contact this server.

	int i = 0;
	struct sockaddr_in remote_address;
	vector<int> all_client_sockets;
	for (auto i : client_sockets)
	{
		all_client_sockets.push_back(i);
	}
	int client_socket;
	vector<int> new_client_sockets;
	while (i < accept_requests)
	{
		// number of connections that will be established will be equal to number of neighbours
		socklen_t addrlen = sizeof(remote_address);
		client_socket = accept(master_socket, (SA *)&remote_address, &addrlen);
		// assumption is that accept function is just going to block the control flow of the thread and
		// not the entire program
		if (client_socket != -1)
		{
			all_client_sockets.push_back(client_socket);
			new_client_sockets.push_back(client_socket);
			i++;
		}
	}

	for (auto i : new_client_sockets)
	{
		n = greeting.length();
		sentval = send(i, greeting.c_str(), n + 1, 0);
	}

	vector<int> clsocket2;
	vector<string> asked2;
	for (auto i : all_client_sockets)
	{
		memset(buffer, 0, 10000);
		readval = recv(i, &buffer, 10000, 0);
		clsocket2.push_back(i);
		asked2.push_back(string(buffer));
	}

	for (int i = 0; i < clsocket2.size(); i++)
	{
		vector<string> asked_files = get_file_list(asked2[i], " ");

		if (asked_files.size() > 0)
		{
			for (auto j : asked_files)
			{
				string path = my_path + "/" + j;
				sentval = send_server_image(clsocket2[i], path);
			}
		}
	}
}

void server(int num_of_neighbours, int PORT, string msg, vector<string> files, vector<string> my_files, int id, string my_path)
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

	thread communication_thread = thread(server2, std::ref(client_sockets), msg, files, my_files, id, my_path, std::ref(master_socket));

	if (communication_thread.joinable())
	{
		communication_thread.join();
	}
}

void client(vector<int> neighbouring_ports, vector<int> &sockfd, vector<string> required_files, vector<string> my_files, string my_path, int unique_id)
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

	for (int i = 0; i < number_of_neighbours; i++)
	{
		readval = recv(sockfd[i], &buffer, 10000, 0);
		int uid1 = stoi(get_word(string(buffer), 5, " "));
		sockfd_to_uid[sockfd[i]] = uid1;
		uid_to_sockfd[uid1] = sockfd[i];
		uid_to_port[uid1] = stoi(get_word(string(buffer) + " ", 8, " "));
		cout << buffer << endl;
	}

	msg = "";
	for (auto file : required_files)
	{
		msg = msg + file + " ";
	}

	vector<string> list_of_files = get_file_list(msg, " ");

	n = msg.length();
	for (int i = 0; i < number_of_neighbours; i++)
	{
		sentval = send(sockfd[i], msg.c_str(), n + 1, 0);
	}

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

	/// =================== 3rd phase completed ===================
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

	for (auto file : required_files)
	{
		if (owner_list.find(file) == owner_list.end())
		{
			msg = msg + file + " ";
		}
	}

	// ///////////// 6 ///////////
	n = msg.length();
	for (int i = 0; i < number_of_neighbours; i++)
	{
		sentval = send(sockfd[i], msg.c_str(), n + 1, 0);
	}

	unordered_map<string, int> two_hop_away;
	unordered_map<string, int> two_hop_away_ports;
	// ////////////// 7 /////////////
	list_of_files = get_file_list(msg, " ");
	for (int j = 0; j < number_of_neighbours; j++)
	{
		memset(buffer, 0, 10000);
		readval = recv(sockfd[j], &buffer, 10000, 0);
		vector<string> uids_ports = get_file_list(string(buffer), " ");

		vector<int> uids;
		vector<int> ports;
		for (int i = 0; i < uids_ports.size(); i++)
		{
			if (i % 2 == 0)
			{
				uids.push_back(stoi(uids_ports[i]));
			}
			else
			{
				ports.push_back(stoi(uids_ports[i]));
			}
		}
		for (int i = 0; i < list_of_files.size(); i++)
		{
			if (owner_list.find(list_of_files[i]) == owner_list.end())
			{
				if (uids[i] != 0)
				{
					if (two_hop_away.find(list_of_files[i]) == two_hop_away.end())
					{
						two_hop_away[list_of_files[i]] = uids[i];
						one_hop_away[uids[i]] = sockfd[j];
						two_hop_away_ports[list_of_files[i]] = ports[i];
					}
					else
					{
						if (two_hop_away[list_of_files[i]] > uids[i])
						{
							one_hop_away.erase(one_hop_away.find(two_hop_away[list_of_files[i]]));
							two_hop_away[list_of_files[i]] = uids[i];
							two_hop_away_ports[list_of_files[i]] = ports[i];
							one_hop_away[uids[i]] = sockfd[j];
						}
					}
				}
			}
		}
	}

	vector<int> connections;
	for (auto i : two_hop_away_ports)
	{
		if (std::find(connections.begin(), connections.end(), i.second) == connections.end())
		{
			connections.push_back(i.second);
		}
	}

	connection_to_be_sent = connections.size();

	for (int j = 0; j < number_of_neighbours; j++)
	{
		msg = to_string(unique_id) + " ";
		for (auto i : one_hop_away)
		{
			if (i.second == sockfd[j])
			{
				msg = msg + to_string(i.first) + " ";
			}
		}

		n = msg.length();
		sentval = send(sockfd[j], msg.c_str(), n + 1, 0);
	}

	for (int i = 0; i < number_of_neighbours; i++)
	{
		memset(buffer, 0, 10000);
		readval = recv(sockfd[i], &buffer, 10000, 0);
		// cout << buffer << " received" << endl;
		accept_requests += stoi(string(buffer));
	}

	move_ahead = true;

	// // after notifying ==================================

	vector<int> new_sockfds;
	if (connection_to_be_sent > 0)
	{
		int socket_fds[connections.size()];
		bool request_sent[connections.size()];
		for (int i = 0; i < connections.size(); i++)
		{
			request_sent[i] = false;
		}
		for (int i = 0; i < connection_to_be_sent; i++)
		{
			socket_fds[i] = socket(AF_INET, SOCK_STREAM, 0);
		}
		bool exit2 = false;
		while (!exit2)
		{
			for (int i = 0; i < connection_to_be_sent; i++)
			{
				if (!request_sent[i])
				{
					servaddress.sin_port = htons(connections[i]);
					if (connect(socket_fds[i], (SA *)&servaddress, sizeof(servaddress)) != 0)
					{
					}
					else
					{
						new_sockfds.push_back(socket_fds[i]);
						request_sent[i] = true;
					}
				}
			}

			for (int i = 0; i < connections.size(); i++)
			{
				if (!request_sent[i])
				{
					exit2 = false;
				}
				else
				{
					exit2 = true;
					break;
				}
			}
		}
	}

	unordered_map<int, int> sockfd_to_uid2;
	for (int i = 0; i < connection_to_be_sent; i++)
	{
		memset(buffer, 0, 10000);
		readval = recv(new_sockfds[i], &buffer, 10000, 0);
		sockfd_to_uid2[new_sockfds[i]] = stoi(get_word(string(buffer), 5, " "));
	}

	unordered_map<string, int> all_required_files;
	for (auto i : owner_list)
	{
		all_required_files[i.first] = i.second;
	}
	for (auto i : two_hop_away)
	{
		all_required_files[i.first] = i.second;
	}

	unordered_map<int, int> all_sockfd_to_uid;
	for (auto i : sockfd_to_uid)
	{
		all_sockfd_to_uid[i.first] = i.second;
	}
	for (auto i : sockfd_to_uid2)
	{
		all_sockfd_to_uid[i.first] = i.second;
	}

	vector<int> all_sockfds;
	for (auto i : sockfd)
	{
		all_sockfds.push_back(i);
	}
	for (auto i : new_sockfds)
	{
		all_sockfds.push_back(i);
	}

	for (int i = 0; i < all_sockfds.size(); i++)
	{
		msg = "";
		for (auto file : all_required_files)
		{
			if (file.second == all_sockfd_to_uid[all_sockfds[i]])
			{
				msg = msg + file.first + " ";
			}
		}
		n = msg.length();
		sentval = send(all_sockfds[i], msg.c_str(), n + 1, 0);
	}

	for (int i = 0; i < all_sockfds.size(); i++)
	{
		msg = "";
		for (auto file : all_required_files)
		{
			if (file.second == all_sockfd_to_uid[all_sockfds[i]])
			{
				string p = my_path + "/Downloaded/";
				int status = mkdir(p.c_str(), 0777);
				string path = p + file.first;
				readval = recv_client_image(all_sockfds[i], path);
			}
		}
	}

	sort(required_files.begin(), required_files.end());

	for (auto file : required_files)
	{
		if (owner_list.find(file) != owner_list.end())
		{
			string path = my_path + "/Downloaded/" + file;
			string md5 = get_md5_sum(path);
			cout << "Found " << file << " at " << owner_list[file] << " with MD5 " << md5 << " at depth 1" << endl;
		}
		else if (two_hop_away.find(file) != two_hop_away.end())
		{
			string path = my_path + "/Downloaded/" + file;
			string md5 = get_md5_sum(path);
			cout << "Found " << file << " at " << two_hop_away[file] << " with MD5 " << md5 << " at depth 2" << endl;
		}
		else
		{
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

	thread server_thread = thread(server, number_of_neighbours, PORT, msg, files, my_files, unique_id, string(argv[2]));
	thread client_thread = thread(client, neighbour_ports, std::ref(sockfd), files, my_files, string(argv[2]), unique_id);

	if (client_thread.joinable())
	{
		client_thread.join();
	}
	if (server_thread.joinable())
		server_thread.join();

}
