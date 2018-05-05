#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
 
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
 
//Buffsize to store max length of message,user_len to store length size of user,optlen for instruction
#define BUFFSIZE 1024
#define User_LEN 32
#define OPTLEN 16
#define LINEBUFF 2048




/***************************Structures to be used********************************/



//Structure of Packet of data to be transferred
struct DATA_PACKET {
    char instr[OPTLEN]; 		// instruction data in data packet
    char named[User_LEN]; 		// name of client 
    char buff[BUFFSIZE]; 		// msg in the data packet
};
 
//Structure to hold User's Information
struct USER {
        int sockfd; 		// user's socket descriptor
        char named[User_LEN]; 	// user's name
};
 
//Structure To store thread info created to handle client
struct THREADINFO {
    pthread_t thread_ID; 	// thread's pointer
    int sockfd; 		// socket file descriptor
};






/************************FUNCTION PROTOTYPES AND GLOBAL VARIABLES*********************/ 

int isconnected, sockfd;		//isconnected to check if user is still connected; sockfd to store socket descriptor of client
char instr[LINEBUFF];			//instr to temporarily store msg of client
struct USER me;				             //To store client's detail
 void welcomemsg();
int connect_with_server();		        //Function to connect with server
void setnamed(struct USER *me);		   //Function to set name of user
void logout(struct USER *me);		    //Function to logout user
void login(struct USER *me);		    //Function for user login
void *receiver(void *param);	     	//Function to receive messages from other user's via server
void sendtoall(struct USER *me, char *msg);	//Funcction to send messages to all connected users
void sendtonamed(struct USER *me, char * target, char *msg);	//Function to send message to a particular user
void listall(struct USER *me);



 /********************************MAIN FUNCTION*********************************************/
 
int main(int argc, char **argv) 
{
    	int  namedlen,portno,n;			//local variable to store length of username,portno and n as a error check
    	struct sockaddr_in serv_addr;		//sockaddr_in type defined in <inet.h> to store server address
   	struct hostent *server;			//hostent type for the usage of gethostbyname
   	memset(&me, 0, sizeof(struct USER));    //memset function to set the me variable(used to contain user details) contents to 0 


	//Checking for command line arguments
    	if (argc < 3)
  	{
     		 fprintf(stderr,"usage %s hostname port\n", argv[0]);
    		 exit(0);
 	}
    
    	//Getting portno from command line argument 
   	portno = atoi(argv[2]);

   	//creating a socket using internet communication domain(AF_INET), two way communication protocol(SOCK_STREAM) and tcp protocol(0)
   	sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   	//Checking if socket is successfully created
  	if (sockfd < 0) 
  	{
      		perror("ERROR opening socket");
     		exit(1);
  	}
    
    	//storing server information in server; IP address of server passed through command line argument
   	server = gethostbyname(argv[1]);
   
   	//Checking if server not available
   	if (server == NULL) 
   	{
     		 fprintf(stderr,"ERROR, no such host\n");
     		 exit(0);
 	}
   
   
 	bzero((char *) &serv_addr, sizeof(serv_addr));     	//Clearing the serv_addr variable contents
 	
 	
 	//Storing the server details  (communication proocol used(AF_INET(IPV4)),IP address,port no. of server)
  	serv_addr.sin_family = AF_INET;				
   	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
   	serv_addr.sin_port = htons(portno);
   
  
   	//Sending a connection request to server by providing socket of the client,address of server and checking if connection wasaccepted
  	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
  	{
     		 perror("ERROR connecting");
      		 exit(1);
   	}
    
    
/****************************************Communication Starts****************************************************/
   	
   	welcomemsg();		//welcome  greetings to user
   	
   	
   	//While loop to continuosly getting instructions from the user
   	 while(gets(instr))
    	 {
        
        	 //Checking if user wants help in using the chat system
        	 if(!strncmp(instr, "help", 4)) 
        	 {
           		//Help instruction read from help.txt file
           		FILE *fin = fopen("help.txt", "r");
            		if(fin != NULL) 
            		{
                		while(fgets(instr, LINEBUFF-1, fin)) 
                			puts(instr);
                		fclose(fin);
            		}
            		else
            		{
               			 fprintf(stderr, "Help file not found...\n");
            		}
       		 }

      		 //Checking if user wants to exit from the chat system
      		 else if(!strncmp(instr, "exit", 4)) 
        	 {
            		
            		//If user wants to exit send a logout request to server and breakout from the while loop and exit
            		logout(&me);
           		break;
           		
       		 }
       		 
       		 
       		 //Checking if user wants to login in the chat system
      		 else if(!strncmp(instr, "login", 5)) 
        	 {
           		 
           		 //gets gets an entire line from command line. So, getting the instruction option from instr using strtok funcion
           		 char *ptr = strtok(instr, " ");
            		 ptr = strtok(0, " ");
            		 memset(me.named, 0, sizeof(char) * User_LEN);
            		 
            		 //Checking if user name to login is provided
           		 if(ptr != NULL)
            		 {
               			 namedlen =  strlen(ptr);
                		 if(namedlen > User_LEN)
                		 	 ptr[User_LEN] = 0;
                		 strcpy(me.named, ptr);
           		 }
           		 
           		 //If username not provided server takes care of anonymous users
           		 else 
           		 {
                		strcpy(me.named, "Anonymous");
            		 }
            		 
            /***************sending request to server to login a user***********/		 
            		 
            		 //Checking if user already connected
    			 if(isconnected) 
    			 {
      				  fprintf(stderr, "You are already connected to server\n");// at %s:%d\n", SERVERIP, SERVERPORT);
        			
    			 }
    			
    			 struct USER *mee=&me;
    			 //setting the isconnected variable to represent that user is now connected
   			 isconnected = 1;
        		 mee->sockfd = sockfd;
        		 if(strcmp(mee->named, "Anonymous")) 
            			 setnamed(mee);			//informing the server about this is an anonymous user
            			 
        		 printf("Logged in as %s\n", mee->named);
       				
       				
       			//creating a thread to now receive packets from server if the server sends any and proceed to get new userrequests
        		 struct THREADINFO threadinfo;
        		 pthread_create(&threadinfo.thread_ID, NULL, receiver, (void *)&threadinfo);
 
        	  }
        
       		  //checking if user wants to send message to a particular user
       		  else if(!strncmp(instr, "send:", 5)) 
       		  {
            		 //getting the first part of string containing the option and the second call to get the name of the client user 					wants to chat(It is the functionality of strtok string function).
            		 char *ptr = strtok(instr, " ");
            	         char temp[User_LEN];
            		 ptr = strtok(0, " ");
            		 
            		 //Checking if their exists any user name
            		 memset(temp, 0, sizeof(char) * User_LEN);
           		 if(ptr != NULL) 
           		 {
               			 //storing the message in ptr and user name in temp
               			 namedlen =  strlen(ptr);
                		 if(namedlen > User_LEN) 
                		 	ptr[User_LEN] = 0;		//puts an end of string after username 
                		 strcpy(temp, ptr);
                		 
                		 //gets to the end of string previously stored 
               			 while(*ptr) 
               			 	ptr++; 
               			 ptr++;
               			 while(*ptr <= ' ')
               			 	 ptr++;
               			 	 
               			 //calls sendtonamed function to send the request to the server
                		 sendtonamed(&me, temp, ptr);
            		 }
        	   }
        	   
        	   //Checking if the user want to send some message to all the clients connected through calling sendtoall function
       		   else if(!strncmp(instr, "send-to-all", 11)) 
       		   {
            			sendtoall(&me, &instr[12]);
       		   }
       		   
       		   //Checking if user wants to logout by calling logout function
        	   else if(!strncmp(instr, "logout", 6)) 
        	   {
            			logout(&me);
        	   }
        	   else if(!strncmp(instr, "list", 4)) 
       		   {
            			listall(&me);
       		   }
        	   
        	   //If the user instruction does not match the specified instructions, it generates an error
        	   else fprintf(stderr, "Unknown instr...\n");
    	}
    	return 0;
}
 
/******************logout function which sends the server a request to logout**************/

void logout(struct USER *me) 
{
    
    struct DATA_PACKET data_packet;
    
    //checking if user wants to logout without previous login
    if(!isconnected)
    {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    
    //sending request to server to logout the user
    memset(&data_packet, 0, sizeof(struct DATA_PACKET));
    strcpy(data_packet.instr, "exit");
    strcpy(data_packet.named, me->named);
    send(sockfd, (void *)&data_packet, sizeof(struct DATA_PACKET), 0);
    
    //setting the isconnected variable to indicate user logout
    isconnected = 0;
}

/****************Function to get list of connected clients********************/
void listall(struct USER *me)
{
	int sent;					//variable to check proper working of connection
    struct DATA_PACKET data_packet;		//packet to be sent to other client
    printf("**************Here is the list of connected clients*****************\n\n");
    //checking if user is properly logged in
    if(!isconnected) 
    {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    
    //requesting server to send the list of all connected clients
   
    
    memset(&data_packet, 0, sizeof(struct DATA_PACKET));
    strcpy(data_packet.instr, "list");
    strcpy(data_packet.named, me->named);
    strcpy(data_packet.buff, "listbgf");
    
    sent = send(sockfd, (void *)&data_packet, sizeof(struct DATA_PACKET), 0);
}

 /************************FUNCTION TO ASSIGN NAME TO FD*********************************/


void setnamed(struct USER *me)
{
    int sent;
    struct DATA_PACKET data_packet;
    
    if(!isconnected) 
    {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    
    
    //sending the server the information that this is anonymous user
    memset(&data_packet, 0, sizeof(struct DATA_PACKET));
    strcpy(data_packet.instr, "named");
    strcpy(data_packet.named, me->named);
   
    sent = send(sockfd, (void *)&data_packet, sizeof(struct DATA_PACKET), 0);
}


/************************RECEIVER FUNCTION to receive messages from server*********************************/ 
void *receiver(void *param)
 {
    int recvd;			//check variable to check if connection is proper
    struct DATA_PACKET data_packet;
    
    //receiving information from server until user is connected
    while(isconnected) 
    {
        //check if user receives messages from server properly 
        recvd = recv(sockfd, (void *)&data_packet, sizeof(struct DATA_PACKET), 0);
        if(!recvd)
        {
            fprintf(stderr, "Connection lost from server...\n");
            isconnected = 0;			//setting the connected check variable(global)  to 0
            close(sockfd);			//closing the socket connection
            break;
        }
         
        //printing the message received from server/client
        if(recvd > 0) 
        {
            if(strcmp(data_packet.instr,"list")==0)
            {
            	printf("%s\n",data_packet.buff);
            }
            else
            printf("[%s]: %s\n", data_packet.named, data_packet.buff);

        }
        
        //clearing the datapacket to receive new messages
        memset(&data_packet, 0, sizeof(struct DATA_PACKET));
    }
    return NULL;
}


/**************************FUNCTION to send msg to particular user*************************/
void sendtonamed(struct USER *me, char *target, char *msg)
{
    int sent, targetlen;		//variables to check proper working of connection and to get length of client name
    struct DATA_PACKET data_packet;
    
    
    //checking if user provides client name he wants to chat to
    if(target == NULL)
    {
        return;
    }
    
    //checking if message to be sent is not null
    if(msg == NULL) 
    {
        return;
    }
    
    //checking if user is properly logged in
    if(!isconnected) 
    {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    
    
    
    //requesting server to send message to a particular client
    msg[BUFFSIZE] = 0;
    targetlen = strlen(target);
    
    memset(&data_packet, 0, sizeof(struct DATA_PACKET));
    strcpy(data_packet.instr, "send:");
    strcpy(data_packet.named, me->named);
    strcpy(data_packet.buff, target);        //person to be sent the message
    strcpy(&data_packet.buff[targetlen], " ");   
    strcpy(&data_packet.buff[targetlen+1], msg);     //message  for more info see in server
   
    sent = send(sockfd, (void *)&data_packet, sizeof(struct DATA_PACKET), 0);
}


/**************Function to send a message to all the connected clients*****************/
void sendtoall(struct USER *me, char *msg) 
{
    int sent;					//variable to check proper working of connection
    struct DATA_PACKET data_packet;		//packet to be sent to other client
    
    //checking if user is properly logged in
    if(!isconnected) 
    {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    
    //requesting server to broadcast the message to all connected clients
    msg[BUFFSIZE] = 0;
    
    memset(&data_packet, 0, sizeof(struct DATA_PACKET));
    strcpy(data_packet.instr, "send-to-all");
    strcpy(data_packet.named, me->named);
    strcpy(data_packet.buff, msg);
    
    sent = send(sockfd, (void *)&data_packet, sizeof(struct DATA_PACKET), 0);
}

/*************welcome screen message************************/
void welcomemsg()
{
	printf("*******************Welcome To Our Chat Server***********************\n");
	printf("********Follow the instructions in the help menu to use our application******\n");
}

/********************************END OF CLIENT PROGRAM****************************************/
