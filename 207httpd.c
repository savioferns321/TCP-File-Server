#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int SOCKET_LIMIT = 5;
const char *rootDir;

void extractUrl(char *buffer);
char *concat(const char *s1, char *s2);
char *http_get_mime(char *fname);
char *replace_str(char *str, char *orig, char *rep, int start);

ssize_t Writeline(int sockd, const void *vptr, size_t n) {
	size_t      nleft;
	ssize_t     nwritten;
	const char *buffer;

	buffer = vptr;
	nleft  = n;

	while ( nleft > 0 ) {
		if ( (nwritten = write(sockd, buffer, nleft)) <= 0 ) {
			if ( errno == EINTR ) {
				printf("Interrupted \n");
				nwritten = 0;
			}
			else
			{
				printf("%s\n", buffer );
				perror("Error in Writeline()");
			}
		}
		nleft  -= nwritten;
		buffer += nwritten;
	}

	return n;
}

void *WriteFile(void *ssock) {
	char buf[1024];
	char remainingReq[1024];

	char fsizeStr[4];
	int readLen, writeLen, fsize;
	char *fname = NULL;
	char *mime_type = NULL;
	unsigned char *fileContent = NULL;
	int cliSock = *(int *)ssock;
	char *response = NULL;
	unsigned char buffer[109];

	pthread_detach(pthread_self());
	bzero(buf, 1024);

	readLen = http_readline(cliSock, buf, 1024);
	printf("Client GET request :\n %s\n", buf);
	while (readLen != 0) {
		bzero(remainingReq, 1024);
		readLen = http_readline(cliSock, remainingReq, 1024);
		printf("%s\n", remainingReq);
	}

	//Extract the filename
	extractUrl(buf);
	if (strcmp(buf, "/") == 0) {
		strcpy(buf, "/index.html");
	}
	//sleep(10);
	fname = concat(rootDir, buf);
	fname = replace_str(fname, "/images/", "/img/", 0);
	//printf("%d File to search for is : %s\n", cliSock , fname );
	mime_type = http_get_mime(fname);

	//Verify that it is a legit file
	if ( access( fname, F_OK ) != -1 ) {
		// file exists
		FILE *fp;

		//Get File size
		if ((fp = fopen(fname, "r")) == NULL) {
			printf("error occurred while opening file%s\n", fname );
		}
		if ((fsize = http_get_filesize(fp)) < 0) {
			printf("Error occurred while gttting filsieze\n");
		}
		if ((fseek(fp, SEEK_SET, 0)) < 0) {
			printf("Error occurred while seeking file\n");
		}

		sprintf(fsizeStr, "%d", fsize);
		if (fseek(fp, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
			long bufsize = ftell(fp);
			if (bufsize == -1) {
				perror("File does not have content");
			}

			/* Go back to the start of the file. */
			if (fseek(fp, 0L, SEEK_SET) != 0) {
				printf("Couldnt seek to start of file \n");
			}

			bzero(buffer, 109);
			char *status_code = "HTTP/1.1 200 OK\r\n";
			strcpy(buffer, status_code);
			strcat(buffer, "Server: 207httpd/0.0.1\r\n");
			strcat(buffer, "Connection: close\r\n");
			strcat(buffer, "Content-Type: ");
			strcat(buffer, mime_type);
			strcat(buffer, "\r\n");
			strcat(buffer, "Content-Length: ");
			strcat(buffer, fsizeStr);
			strcat(buffer, "\r\n\r\n");
			Writeline(cliSock, buffer, strlen(buffer));

			size_t bytesSent = 0;
			size_t curbytes = 0;
			char fbuffer[512];

			while ((curbytes = fread(fbuffer, sizeof(char), 512, fp)) > 0) {
				int fileOffset = 0;
				while ((bytesSent = send(cliSock, fbuffer + fileOffset, curbytes, 0)) > 0
				        || (bytesSent == -1 && errno == EINTR)) {
					if (bytesSent > 0) {
						fileOffset += bytesSent;
						curbytes -= bytesSent;
					} else {
						printf("Send failed\n");
					}
				}
			}

		}
		if ((fclose(fp)) < 0)
			printf("Error in closing file \n");
		//printf("Operation complete \n");


	} else {
		// file doesn't exist
		fprintf(stderr, " File not found \n" );
		char *status_code = "HTTP/1.1 404 Not Found\r\n";
		strcpy(buffer, status_code);
		strcat(buffer, "Server: 207httpd/0.0.1\r\n");
		strcat(buffer, "Connection: close\r\n");
		strcat(buffer, "Content-Type: ");
		strcat(buffer, mime_type);
		strcat(buffer, "\r\n");
		strcat(buffer, "Content-Length: ");
		strcat(buffer, "0");
		strcat(buffer, "\r\n\r\n");

		strcat(buffer, "<html><head><title>File not found</title></head><body><h1>No file found</h1></body></html>");

		Writeline(cliSock, buffer, strlen(buffer));
	}

	//Prepare the response
	if (close(cliSock) < 0) {
		printf("Error in closing socket \n");
	} else {
		printf("Closed socket no: %d\n--------------------------------------------------------------------\n", cliSock);
		//free(client);
	}
	//free(response);
	fflush(stdout);
}

int main(int argc, char const *argv[])
{
	int msock, ssock, readLen, port, addrLen, counter = 0;
	struct sockaddr_in client;
	//char *substring;
	rootDir = argv[2];
	pthread_t	th;
	pthread_attr_t	ta;

	//Create master server socket
	msock = createSocket(atoi(argv[1]));

	pthread_attr_init(&ta);
	pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
	addrLen = sizeof(client);

	//Call accept function

	while (1) {

		ssock = accept(msock, (struct sockaddr *)&client, &addrLen);
		printf("Received request from client : %s : %d\n", inet_ntoa(client.sin_addr), (int)ntohs(client.sin_port));

		if (pthread_create(&th, &ta, (void *)WriteFile, (void *)&ssock) != 0) {
			perror("Error while creating thread.\n");
		}
	}

	pthread_attr_destroy(&ta);
	close(msock);
	return 0;
}

char *concat(const char *s1, char *s2)
{
	char *result = malloc(strlen(s1) + strlen(s2) + 1); //+1 for the zero-terminator
	//in real code you would check for errors in malloc here
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}


void extractUrl(char *buffer) {
	char *substring;
	substring = strtok(buffer, " ");
	substring = strtok(NULL, " ");
	strcpy(buffer, substring);
}



int createSocket(int port) {

	int enable = 1;
	struct sockaddr_in server;
	//printf("Entered createSocket \n");

	int n = socket(AF_INET, SOCK_STREAM, 0);
	if (n < 0) {
		perror("Error in creating socket");
		exit(-1);
	}

	if (setsockopt(n, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
		perror("Failed to set sock option SO_REUSEADDR");
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if (bind(n, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Error in binding socket");
		exit(-1);
	}

	if (listen(n, SOCKET_LIMIT) < 0) {
		perror("Error in listening to socket");
		exit(-1);
	}
	printf("Created socket successfully at port %d\n", port);
	return n;

}

char *replace_str(char *str, char *orig, char *rep, int start)
{
	static char temp[4096];
	static char buffer[4096];
	char *p;

	strcpy(temp, str + start);

	if (!(p = strstr(temp, orig))) // Is 'orig' even in 'temp'?
		return temp;

	strncpy(buffer, temp, p - temp); // Copy characters from 'temp' start to 'orig' str
	buffer[p - temp] = '\0';

	sprintf(buffer + (p - temp), "%s%s", rep, p + strlen(orig));
	sprintf(str + start, "%s", buffer);

	return str;
}

int http_readline(int sock, char *buf, int maxlen)
{
	int	n = 0;
	char *p = buf;
	while (n < maxlen - 1) {
		char c;
		//Error at this line for repeated requests
		int rc = read(sock, &c, 1);
		/*printf("Errno : %d RC : %d\n", errno, rc);
		printf("%c\n", c);*/
		if (rc == 1) {
			/* Stop at \n and strip away \r and \n. */
			if (c == '\n') {
				*p = 0; /* null-terminated */
				return n;
			} else if (c != '\r') {
				*p++ = c;
				n++;
			}
		} else if (rc == 0) {
			return -1;
			/* EOF */
		} else if (errno == EINTR) {
			continue;
			/* retry */
		} else {
			return -1;
			/* error */
		}
	}

	return -1; /* buffer too small */
}

int http_get_filesize(FILE *fp)
{
	int fsize;
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	rewind(fp);
	return fsize;
}

char *http_get_mime(char *fname)
{
	const char *extf = strrchr(fname, '.');
	if (extf == NULL) {
		return "application/octet-stream";
	} else if (strcmp(extf, ".html") == 0) {
		return "text/html";
	} else if (strcmp(extf, ".jpg") == 0) {
		return "image/jpeg";
	} else if (strcmp(extf, ".gif") == 0) {
		return "image/gif";
	} else {
		return "application/octet-stream";
	}
}