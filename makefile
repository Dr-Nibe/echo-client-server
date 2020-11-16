all: echo-server echo-client

echo-server: echo-server.cpp
		g++ -o echo-server echo-server.cpp -pthread

echo-client: echo-client.cpp
		g++ -o echo-client echo-client.cpp -pthread

clean:
		rm -f echo-client echo-server