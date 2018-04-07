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
void setupSocket(int* socketFD, struct sockaddr_in* serv_addr, char* server);
void setupQuery(char* query, int* queryLen, char* name, char* type);
void errorExit(int code, char* msg);

char* ntohType(int qType)
{
	qType = ntohs(qType);
	
	switch(qType)
	{
		case 1:
			return "A";
		case 2:
			return "PTR";
		case 5:
			return "CNAME";
		case 12:
			return "PTR";
		case 28:
			return "AAAA";
		default:
			errorExit(1, "Invalid qType in response");
	}
	
	return NULL;
}

int ntohName(char* resultName, unsigned char* response, unsigned char* dnsName, int queryLen)	// hostName == &response[queryLen]
{
	printf("==============\n");
	// --- Decoding offset/dataLen ---
	unsigned short offset;	// First two bytes of name (can be offset or dataLen!!!)
	memcpy(&offset, dnsName, 2);
	offset = ntohs(offset);	
	
	int dataLen = 2;
	if(offset > 0b1100000000000000)	// Is offset used?
	{
		// Offset used
		offset -= 0b1100000000000000;	// Remove offset indicator
	}
	else
	{
		// Offset not used
		dataLen = offset;
		offset = dnsName - response;	// Compute offset @todo WHY?
		
	}
	printf("Offset=%d\ndataLen=%d\n", offset, dataLen);
	
	char newName[255];

		//int sectionLen = dnsName[0];
		//printf("\nsectionlen=%x\n",sectionLen);
	
	int i = 0;
	
	/*
	while(i < dataLen)
	{
		printf("%d < %d\n", i, dataLen);
		int sectionLen = dnsName[i+2];
		printf("sectionLen = %x\n", sectionLen);
		
		if(sectionLen > 0b1100000000000000)
		
		strncpy(newName, (char*)&dnsName[i+3], sectionLen);
		i += sectionLen;	
	}
	printf("%d < %d\n", i, dataLen);
	printf("\nnewName=%s\n",newName);
	*/
	
	// --- Decoding length ---



	
	// --- Converting DNS name to host name ---
	char normalName[255];
	memset(normalName, '\0', sizeof(normalName));
	
		
	i = 0;
	do
	{
		sectionLen = response[offset+i];
		memcpy(&normalName[i], (char*)&response[offset+i+1], sectionLen); // Offset in message + char position + 1 because of section len in index 0
		normalName[i+sectionLen] = '.';
		i += sectionLen+1;
	}
	while(sectionLen != 0);
	
	normalName[i-2] = '\0';	// Change last '.' to \0
	
	
	// --- Return result ---
	strcpy(resultName, normalName);
	
	return 2;
}

void decodeResponse(unsigned char* response, int responseLen, int queryLen)
{
	/*
	// --- Debug print ---
	printf("==========[RECEIVED]==========\n");
	for(int i=0; i < responseLen; i++)	// @todo Is it good idea to use n instead of responseLen?
    {
		printf("%02X(%d) ", response[i], response[i]);
	}
	printf("\n");	
	*/
	
	// --- Getting answers count ---
	unsigned short answerCount;
	memcpy(&answerCount, &response[6], 2);
	answerCount = ntohs(answerCount);
	
	
	//for(int i = 0; i < answerCount; i++)
	//{
		int bytesUsed = 0;	// Number of bytes used for name
		
		char name[255];
		bytesUsed = ntohName(name, response, &response[queryLen], queryLen);
		
		unsigned short qType;
		memcpy(&qType, &response[queryLen+bytesUsed], 2);
		char *type = ntohType(qType);
		
		
		unsigned short qClass;
		memcpy(&qClass, &response[queryLen+bytesUsed+2], 2);
		qClass = ntohs(qClass);	
		if(qClass != 1)
			errorExit(1, "Unexpected qClass in response");

	// --- Debug print ---
	/*printf("==========[RECEIVED]==========\n");
	for(int i=queryLen+bytesUsed+8; i < responseLen; i++)	// @todo Is it good idea to use n instead of responseLen?
    {
		printf("%02X(%d) ", response[i], response[i]);
	}
	printf("\n");	*/
		
		char cName[255];
		//printf("DBG=%x\n",response[queryLen+bytesUsed+8]);
		ntohName(cName, response, &response[queryLen+bytesUsed+8], queryLen);
		
		
		printf("%s. IN %s %s\n", name, type, cName);
		
		/*
		//printf("\n==========[PRINTING]==========\n");
		unsigned short dataLen;
		memcpy(&dataLen, &response[queryLen+10], 2);
		dataLen = ntohs(dataLen);	
		printf("\ndateLen=%d\n", dataLen);	
		
		//printf("\n===============================\n");
		*/
	//}

}


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


	// --- Setting up socket ---
	int socketFD;
	struct sockaddr_in serv_addr;
	
	setupSocket(&socketFD, &serv_addr, server);
	

	// --- Setting up query ---
	char query[255];
	int queryLen;
	setupQuery(query, &queryLen, name, type);
	
	
	// --- Sending query ---
	int n;
	n = sendto(socketFD, query, queryLen, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if(n < 0)
		errorExit(1, "Error sending query");
	
	
	// --- Receiving response ---	
	unsigned char response[65536];
	socklen_t responseLen;
	n = recvfrom(socketFD, response, sizeof(response), 0, (struct sockaddr*)&serv_addr, &responseLen);
	if(n < 0)
		errorExit(1, "Error receiving query");
		
	decodeResponse(response, n, queryLen);	// @todo Use n or reposnseLen?
		
	
	
		
	
	
	/*
	int answerNameIndex = 0;	
	int responseIndex = offset;
	
	int len = response[responseIndex];
	memcpy(&answerName[0], &response[offset+1], len);*/
	
	
	
/*	while(response[responseIndex] != 0)
	{
		answerName[answerNameIndex] = '.';
		memcpy(&answerName[answerNameIndex+1], &response[responseIndex+1], response[responseIndex]);
		
		responseIndex += response[responseIndex]+1;
		answerNameIndex += response[responseIndex];
	}*/
	
	
	
}


void getArguments(int argc, char** argv, char** server, int* timeout, char** type, int* iterative, char** name)
{
	int c;
	int hFlag, sFlag; // TFlag, tFlag, iFlag;
	
	// --- Loading arguments ---
	while((c = getopt(argc, argv, "hs:T:t:i")) != -1)
	{
		switch(c)	// @todo Using operator twice check
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

void setupSocket(int* socketFD, struct sockaddr_in* serv_addr, char* server)
{
	*socketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(*socketFD < 0)
		errorExit(1, "Couldn't create socket");
	
	serv_addr->sin_family = AF_INET;	// Symbolic constant
    serv_addr->sin_port = htons(53);	// Convert port 53 (DNS port) to network byte order
    serv_addr->sin_addr.s_addr = inet_addr(server);	// DNS server adress	
}

void setupQuery(char* query, int* queryLen, char* name, char* type)
{
	unsigned short transactionID; 
	unsigned short flags;
	unsigned short questions;
	unsigned short qType;
	unsigned short qClass;
	
	
	// --- Setting up basic info ---
	transactionID = htons(getpid());	// Sometimes second byte is minus and is strange AF
	flags = htons(0b0000000100000000);		
	questions = htons(1);	// Questions count
	qClass = htons(1);	// Class IN
	
	
	// --- Converting name to query style ---
	// www.fit.vutbr.cz -> 3www3fit5vutbr2cz
	char qName[strlen(name)+2];
	strcpy(&qName[1], name);
	
	int prevIndex = 0;
	int len;
	while((len = strcspn(&qName[prevIndex+1], ".")) != 0)
	{
		len = strcspn(&qName[prevIndex+1], ".");
		qName[prevIndex] = len;
		prevIndex += len+1; 
	}
	int indexAfterName = 12+strlen(qName)+1;
	
	
	// --- Getting type number ---
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

	
	
	// --- Creating query ---
	*queryLen = indexAfterName + 4;
	memset(query, '\0', *queryLen); 	// Earse buffer
	
	memcpy(query, &transactionID, 2);
	memcpy(&query[2], &flags, 2);
	memcpy(&query[4], &questions, 2);
	memcpy(&query[12], &qName, strlen(qName)+1);
	memcpy(&query[indexAfterName], &qType, 2);
	memcpy(&query[indexAfterName+2], &qClass, 2);
	
	/*
	// --- Debug print ---
	printf("==========[SENT]==========\n");
	for(int i=0; i<indexAfterName+4; i++)
    {
		printf("%02X(%d) ", query[i], query[i]);
	}
	printf("\n");	
	*/
}

void errorExit(int code, char* msg)
{
	fprintf(stderr,"ERROR: %s\n", msg);
	exit(code);	
}
