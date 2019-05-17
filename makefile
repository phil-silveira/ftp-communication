all: s.exe c.exe

s.exe: ftpServer.c
	gcc ftpServer.c -o s.exe

c.exe: ftpClient.c
	gcc ftpClient.c -o c.exe

clean:
	rm *.exe
