all: echo-client echo-server

echo-client: echo-client.c
	gcc -o echo-client echo-client.c -pthread

echo-server: echo-server.c
	gcc -o echo-server echo-server.c -pthread

clean:
	rm -f echo-client echo-server
