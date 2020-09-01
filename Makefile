all: WTF WTFServer

WTFServer: WTFServer.c
	gcc WTFServer.c -lpthread -lcrypto -o WTFServer

WTF: WTF.c
	gcc WTF.c -lpthread -lcrypto -o WTF
clean:
	rm -rf WTF WTFServer
