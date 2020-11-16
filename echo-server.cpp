#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <thread>

#define MAX_CLI_NUM 20 // the maximum number of clients that can be connected at the same time

using namespace std;

int cli_sd[MAX_CLI_NUM]; // client socket descriptor

void usage() {
	cout << "syntax: echo-server <port> [-e[-b]]\n";
	cout << "  -e: echo\n";
	cout << "  -b: broadcast\n";
	cout << "sample: echo-server 1234 -e -b\n";
}

// initialize cli_sd
void init() {
	for (int i = 0; i < MAX_CLI_NUM; i++)
		cli_sd[i] = -1;
}

struct Param {
	bool broadcast{false};
	bool echo{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-b") == 0) {
				broadcast = true;
				continue;
			}
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				continue;
			}
			port = stoi(argv[i]);
		}
		return port != 0;
	}
} param;

void recvThread(int i) {
	cout << "connected\n";
	int sd = cli_sd[i];
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			cerr << "recv return " << res << endl;
			perror("recv");
			break;
		}
		buf[res] = '\0';

		cout << buf;
		cout.flush();

		// broadcast
		if (param.broadcast) {
			for (int j = 0; j < MAX_CLI_NUM; j++)
				if (cli_sd[j] != -1) {
					res = send(cli_sd[j], buf, res, 0);
					if (res == 0 || res == -1) {
						cerr << "send return " << res << endl;
						perror("send");
						break;
					}
				}
		}

		// echo
		if (param.echo) {
			res = send(cli_sd[i], buf, res, 0);
			if (res == 0 || res == -1) {
				cerr << "send return " << res << endl;
				perror("send");
				break;
			}
		}
	}
	cout << "disconnected\n";
    close(sd);
	cli_sd[i] = 0;
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

	init();

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int optval = 1;
	int res = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5);
	if (res == -1) {
		perror("listen");
		return -1;
	}

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);

		int i;
		for (i = 0; i < MAX_CLI_NUM; i++)
			if (cli_sd[i] == -1) break;
		if (i == MAX_CLI_NUM) {
			puts("Too many client!");
			return -1;
		}

		cli_sd[i] = accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd[i] == -1) {
			perror("accept");
			break;
		}

		thread* t = new thread(recvThread, i);
		t->detach();
	}
	close(sd);
}
