/**
 * @file ipk-lookup.c
 * @author Jiri Furda (xfurda00)
 */ 
 
// === Libraries ===
#include <stdio.h>
#include <stdlib.h>	// exit
#include <unistd.h>	// getopt
#include <string.h>
#include <sys/socket.h> // socket
#include <netinet/in.h> // IPPROTO_UDP
#include <arpa/inet.h>	// inet_addr


// === Prototypes ===
void getArguments(int argc, char** argv, char** server, int* timeout, char** type, int* iterative, char** name);
void setFlag(int* flag);
void errorExit(int code, char* msg);



// === Functions ===
int main(int argc, char** argv)
{
	// --- Default values ---
	char* server;	// @todo Findout max length
	int timeout = 5;
	char* type = "A";
	int iterative = 0;
	char* name;
	
	
	// --- Loading arguments ---
	getArguments(argc, argv, &server, &timeout, &type, &iterative, &name);
	
	
	// --- Arguments debug ---
	printf("%s\n%d\n%s\n%d\n%s\n", server, timeout, type, iterative, name);
	
	
	
	//////////////////////////////////
	// --- Setting up socket ---
	int socketFD;
	socketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;	// Symbolic constant
    serv_addr.sin_port = htons(53);	// Convert port 53 (DNS port) to network byte order
    serv_addr.sin_addr.s_addr = inet_addr(server);	// DNS server adress
    
    
    
    // --- Query ---
    unsigned short transactionID;    
	unsigned short flags;
	unsigned short questions;
	unsigned short answer;
	unsigned short authority;
	unsigned short additional;
	char qName[strlen(name)+2];
	unsigned short qType;
	unsigned short qClass;
	
	char query[255];
	
	transactionID = htons(getpid());	// Sometimes second byte is minus and is strange AF
	
	flags = 0b0000000100000000;
		
	questions = htons(1);
	answer = 0;
	authority = 0;
	additional = 0;
		
	memcpy(query, &transactionID, 2);
	memcpy(&query[2], &flags, 2);
	memcpy(&query[4], &questions, 2);
	memcpy(&query[6], &answer, 2);
	memcpy(&query[8], &authority, 2);
	memcpy(&query[10], &additional, 2);	
		
	printf("=======\n");
	/*
	for(int i=0; i<4; i++)
    {
		printf("%02X(%d) ", query[i], query[i]);
	}
	printf("\n");
	*/
	
	
	strcpy(&qName[1], name);
	
	int prevIndex = 0;
	int len;

	while((len = strcspn(&qName[prevIndex+1], ".")) != 0)
	{
		len = strcspn(&qName[prevIndex+1], ".");
		qName[prevIndex] = len;
		prevIndex += len+1; 
	}
	
/*	printf("=======\n");
	for(int i=0; i<strlen(qName)+1; i++)
    {
		printf("%02X(%c) ", qName[i], qName[i]);
	}
*/
	
	memcpy(&query[12], &qName, strlen(qName)+1);
	int indexAfterName = 12+strlen(qName)+1;
	
	
	if(!strcmp(type, "A"))
		qType = 1;
	else if(!strcmp(type, "AAAA"))
		qType = 28; // @todo Not sure 
	else if(!strcmp(type, "NS"))
		qType = 2;
	else if(!strcmp(type, "PTR"))
		qType = 12;
	else if(!strcmp(type, "CNAME"))
		qType = 5;
	
	qType = htons(qType);
	
	memcpy(&query[indexAfterName], &qType, 2);
	
	qClass = htons(1);
	
	memcpy(&query[indexAfterName+2], &qClass, 2);
	
	printf("=======\n");
	
	for(int i=0; i<indexAfterName+4; i++)
    {
		printf("%02X(%d) ", query[i], query[i]);
	}
	printf("\n");
	
	
	sendto(socketFD, query, indexAfterName+4, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
}


void getArguments(int argc, char** argv, char** server, int* timeout, char** type, int* iterative, char** name)
{
	int c;
	int hFlag, sFlag, TFlag, tFlag, iFlag;
	
	// --- Loading arguments ---
	while((c = getopt(argc, argv, "hs:T:t:i")) != -1)
	{
		printf("Argument: %c\n",c);
		switch(c)
		{
			case 'h':
				//setFlag(&hFlag);
				hFlag = 1;
				break;
			case 's':
				//setFlag(&sFlag);
				sFlag = 1;
				*server = optarg;
				break;
			case 'T':
				//setFlag(&TFlag);
				*timeout = atoi(optarg);
				break;
			case 't':
				//setFlag(&tFlag);
				if(!strcmp(optarg, "A") || !strcmp(optarg, "AAAA") || !strcmp(optarg, "NS") || !strcmp(optarg, "PTR") || !strcmp(optarg, "CNAME"))
					*type = optarg;
				else
					errorExit(2, "Invalid -t value");
				break;
			case 'i':
				//setFlag(&iFlag);
				*iterative = 1;
				break;
			case '?':
				errorExit(2, "Invalid option used");
				break;
			default:
				errorExit(2, "Strange error while loading arguments");
				break;
		}
	}
	
	// --- Help option ---
	if(hFlag == 1)
	{
		if(argc == 2)
		{
			printf("@todo");
			exit(0);
		}
		else
			errorExit(2, "Option -h cannot be combinated with other arguments");
	}
	
	// --- Arguments check ---
	if(sFlag != 1)
		errorExit(2, "Option -s is required");
	
	if(argc > 9 || optind > 8)
		errorExit(2, "Too many arguments");

	if(argc < 4 || optind < 3)
		errorExit(2, "Too few arguments");
	
	if(argc != optind+1)
		errorExit(2, "Invalid arguments usage (maybe name is missing?)");
		
	*name = argv[optind];
}

void setFlag(int* flag)
{
	if(*flag == 1)
		errorExit(2, "Option can't be used two times");
	else
		*flag = 1;
}

void errorExit(int code, char* msg)
{
	fprintf(stderr,"ERROR: %s\n", msg);
	exit(code);	
}
