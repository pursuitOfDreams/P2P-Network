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
vector<string> founds;
unordered_map<int, int> sockfd_to_uid;
unordered_map<int, int> clientsocket_to_uid;
unordered_map<string, int> owner_list;
// Semaphore variables
std::mutex mu;
int readercount = 0;
unsigned char result[MD5_DIGEST_LENGTH];

// step 1 : server will send the information about itself
// step 2 : client will receive the information sent by the server
// step 3 : client will check the information sent
// step 4 : if the file



// Get the size of the file by its file descriptor
unsigned int get_size_by_fd(int fd) {
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0) exit(-1);
    return statbuf.st_size;
}

unsigned char* get_MD5(string file_path){
	int file_descript;
    unsigned int file_size;
    char* file_buffer;
	memset(result,0,MD5_DIGEST_LENGTH);

	file_descript = open(file_path.c_str(), O_RDONLY);
    if(file_descript < 0) exit(-1);

    file_size = get_size_by_fd(file_descript);
    // printf("file size:\t%lu\n", file_size);

    file_buffer = (char *)mmap(0, file_size, PROT_READ, MAP_SHARED, file_descript, 0);
    MD5((unsigned char*) file_buffer, file_size, result);
    munmap(file_buffer, file_size); 

	return result;

    // print_md5_sum(result);
    // printf("  %s\n", argv[1]);
}

string get_md5_sum(string file_path) {
    int i;
	unsigned char* md=get_MD5(file_path);
    string result;
    char buf[32];
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
            sprintf(buf,"%02x",md[i]);
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
	int temp=0;
	while (to_be_received > 0)
	{
		if ((n = recv(sock, Rbuffer+temp, sizeof(Rbuffer), 0)) < 0)
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

void server2(vector<int> &client_sockets, string msg, vector<string> required_files, vector<string> my_files, int unique_id, string my_path)
{
	char buffer[10000];
	int readval;
	int sentval;
	int n;

	for (auto i : client_sockets)
	{
		n = msg.length();
		sentval = send(i, msg.c_str(), n + 1, 0);
	}

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

	for (int i = 0; i < client_sockets.size(); i++)
	{
		msg = messages[i];
		n = msg.length();

		sentval = send(client_sockets[i], msg.c_str(), n + 1, 0);
	}

	vector<int> clsocket;
	vector<string> asked;
	for (auto i : client_sockets)
	{	
		memset(buffer,0,10000);
		readval = recv(i, &buffer, 10000, 0);
		clsocket.push_back(i);
		asked.push_back(string(buffer));
	}

	for (int i = 0; i < clsocket.size(); i++)
	{
		vector<string> asked_files = get_file_list(asked[i], " ");

		if (asked_files.size()>0){
			for (auto j : asked_files)
			{
				string path = my_path + "/" + j;
				sentval=send_server_image(clsocket[i], path);
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

	thread communication_thread = thread(server2, std::ref(client_sockets), msg, files, my_files, id, my_path);

	if (communication_thread.joinable())
	{
		communication_thread.join();
	}
}

void client(vector<int> neighbouring_ports, vector<int> &sockfd, vector<string> required_files, vector<string> my_files, string my_path)
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
		sockfd_to_uid[sockfd[i]] = stoi(get_word(string(buffer), 5, " "));
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


	for (int i = 0; i < number_of_neighbours; i++)
	{
		msg = "";
		for (auto file : owner_list)
		{
			if (file.second == sockfd_to_uid[sockfd[i]])
			{
				
				msg = msg + file.first + " ";
			}
		}
		n = msg.length();
		sentval = send(sockfd[i], msg.c_str(), n + 1, 0);
	}

	for (int i = 0; i < number_of_neighbours; i++)
	{
		msg = "";
		for (auto file : owner_list)
		{
			if (file.second == sockfd_to_uid[sockfd[i]])
			{
				string p = my_path + "/Downloaded/";
				int status = mkdir(p.c_str(),0777);
				string path = p + file.first;
				readval = recv_client_image(sockfd[i], path);
			}
		}
	}

	sort(required_files.begin(),required_files.end());

	for (auto file : required_files)
	{
		if (owner_list.find(file) == owner_list.end())
		{
			cout << "Found " << file << " at 0 with MD5 0 at depth 0" << endl;
		}
		else
		{	
			string path= my_path+"/Downloaded/"+file;
			string md5= get_md5_sum(path);
			cout << "Found " << file << " at " << owner_list[file] << " with MD5 "<<md5<<" at depth 1" << endl;
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
	thread client_thread = thread(client, neighbour_ports, std::ref(sockfd), files, my_files, string(argv[2]));

	if (client_thread.joinable())
	{
		client_thread.join();
	}
	if (server_thread.joinable())
		server_thread.join();


}
