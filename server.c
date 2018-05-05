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

//Buffsize to store max length of message,user_len to store length size of user,optlen for instruction,Backlog for queue

#define BACKLOG 10
#define Max_CLI 10
#define BUFFSIZE 1024
#define User_LEN 32
#define OPTLEN 16



/***************************Structures to be used********************************/

//structure of packet of data to be transferred to and from client
struct DATA_PACKET {
    char instr[OPTLEN]; 		// instruction
    char named[User_LEN]; 		// client's name
    char buff[BUFFSIZE]; 		// msg in the data packet
};
 
//Structure to hold client's Information
struct CLI_THREAD {
    pthread_t thread_ID; 		// thread's pointer
    int sockfd; 			// socket file descriptor
    char named[User_LEN]; 		// client's name
};
 
//A linked list node to store each client's information(thread)
struct LLNODE {
    struct CLI_THREAD threadinfo;		//ThreadINFO type to store information of client
    struct LLNODE *next;
};
 
//A linked list containing pointers to both head and tail of the list
struct LLIST {
    struct LLNODE *head, *tail;
    int size;
};

//compare function that returns difference of sockets of two clients 
int compare(struct CLI_THREAD *a, struct CLI_THREAD *b) {
    return a->sockfd - b->sockfd;
}

/********************************LINKED LIST FUNCTIONS**********************************************/

//initializing the list used to store client details
void list_init(struct LLIST *ll) {
    ll->head = ll->tail = NULL;
    ll->size = 0;
}
 
 //inserting information about a client in a list
int list_insert(struct LLIST *ll, struct CLI_THREAD *thr_info) 
{
    //Checking if list size already exceeded maximum limit
    if(ll->size == Max_CLI) 
        return -1;

    //Checking if list is empty
    if(ll->head == NULL) 
    {
        ll->head = (struct LLNODE *)malloc(sizeof(struct LLNODE));
        ll->head->threadinfo = *thr_info;
        ll->head->next = NULL;
        ll->tail = ll->head;
    }

    //Inserting at the end of the list
    else 
    {
        ll->tail->next = (struct LLNODE *)malloc(sizeof(struct LLNODE));
        ll->tail->next->threadinfo = *thr_info;
        ll->tail->next->next = NULL;
        ll->tail = ll->tail->next;
    }
    ll->size++;		//size to contain the number of clients connected
    return 0;
}
 
 
//deleting a clients information when it disconnects
int list_delete(struct LLIST *ll, struct CLI_THREAD *thr_info) 
{
    struct LLNODE *curr, *temp;

    //Checking if the list is empty
    if(ll->head == NULL) 
        return -1;

    //Comparing list's head with client to be deleted
    if(compare(thr_info, &ll->head->threadinfo) == 0) 
    {
        temp = ll->head;
        ll->head = ll->head->next;
        if(ll->head == NULL) ll->tail = ll->head;
        free(temp);
        ll->size--;
        return 0;
    }

    //Traversing the list to get client info
    for(curr = ll->head; curr->next != NULL; curr = curr->next)
    {
        if(compare(thr_info, &curr->next->threadinfo) == 0) {
            temp = curr->next;
            if(temp == ll->tail) ll->tail = curr;
            curr->next = curr->next->next;
            free(temp);
            ll->size--;
            return 0;
        }
    }
    return -1;
}

//A server help function to list the clients connected
void list_dump(struct LLIST *ll) 
{
    struct LLNODE *curr;
    struct CLI_THREAD *thr_info;
    printf("Connection count: %d\n", ll->size);     //Printing the no of clients connected

    //Printing list of clients connected
    for(curr = ll->head; curr != NULL; curr = curr->next) 
    {
        thr_info = &curr->threadinfo;
        printf("[%d] %s\n", thr_info->sockfd, thr_info->named);
    }
}


/************************FUNCTION PROTOTYPES AND GLOBAL VARIABLES*********************/ 

//global variables to be used; the variable names are self-explanatory
int sockfd, newfd,portno,clilen;
struct CLI_THREAD thread_info[Max_CLI];
struct LLIST client_list;
pthread_mutex_t clientlist_mutex;           //Mutex lock to get a lock
 
//helper function to handle a client
void *client_handler(void *fd);

//helper function to serve the server
void *io_handler(void* param);



/********************************MAIN FUNCTION*********************************************/
int main(int argc, char **argv)
{
    	int err_ret, sin_size;						           //local variable to get errno defined in <err.h>
    	struct sockaddr_in serv_addr, client_addr,cli_addr;		//storing the server and client address
    	pthread_t interrupt;	                                //storing I/O thread information
 
    	/* initialize linked list */
    	list_init(&client_list);
 
    	/* initiate mutex so that its parallel use is not allowed*/
    	pthread_mutex_init(&clientlist_mutex, NULL);
 
 	//variables to store information(self-explanatory)
  	int newsockfd, portno, clilen;
   	char buffer[256];				//buffer to store msg
  	int  n;						//check variable 
   
   	//creating a socket using internet communication domain(AF_INET), two way communication protocol(SOCK_STREAM) and tcp protocol(0)
  	sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   
   	//checking if socket created properly
  	if (sockfd < 0) 
  	{
      		perror("ERROR opening socket");
      		exit(1);
  	}
   
   	// Initializing socket structure 
  	bzero((char *) &serv_addr, sizeof(serv_addr));
   	portno =atoi(argv[1]);				//portno to be used provided by command line argument
   
   	//initializing the server address
   	serv_addr.sin_family = AF_INET;			//internet domain for communication
  	serv_addr.sin_addr.s_addr = INADDR_ANY;		//getting self ip-address
   	serv_addr.sin_port = htons(portno);		//getting portno.
   
  	// Now binding the host address to socket created previosly using bind() call
  	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
  	{
    		  perror("ERROR on binding");
     		  exit(1);
   	}
      
   /***********Starts listening for the incoming clients requests***********************/
 
   
   	listen(sockfd,5);		//maximum 5 clients accepted, start listening on the socket
   	clilen = sizeof(cli_addr);  //Getting 


	//creating a thread to handle input/output 
    	if(pthread_create(&interrupt, NULL, io_handler, NULL) != 0)
    	{
       		 err_ret = errno;
       		 fprintf(stderr, "pthread_create() failed...\n");
       		 return err_ret;
  	}
 
   

    
    	printf("Starting server...\n");
   
    	//continuosly accept requests from user
    	while(1) 
  	{
        	sin_size = sizeof(struct sockaddr_in);
        	
        	//accept a new request on the socket created,store client's address in cli_addr and store the client's socket fd in newfd
        	if((newfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen))== -1)
       		{
           		 err_ret = errno;
            		fprintf(stderr, "accept() failed...\n");
            		return err_ret;
        	}
        	else 
        	{
           		 if(client_list.size == Max_CLI)
           		 {
               			 fprintf(stderr, "No,more connections possible...\n");
               			 continue;
           		 }
           		 printf("Accepted connection request from client...\n");
           		 
           		 //storing client's info in the list
           		 struct CLI_THREAD threadinfo;
            		 threadinfo.sockfd = newfd;
            		 strcpy(threadinfo.named, "Anonymous");
            		 pthread_mutex_lock(&clientlist_mutex);		//creating a mutex lock show that two client's do not 											simulataneousy access the list 
            		 list_insert(&client_list, &threadinfo);
           		 pthread_mutex_unlock(&clientlist_mutex);	//unlocking the lock
           		 
           		 //Creating a new thread to handle requests from client and proceed to accept new requests
            		 pthread_create(&threadinfo.thread_ID, NULL, client_handler, (void *)&threadinfo);
       		 }
   	 }
 
    return 0;
}


/***************I/O HANDLER FOR SERVER REQUESTS********************/
void *io_handler(void* param)
{
    
    char instr[OPTLEN];
    
    //Scanning requests from server admin
    while(scanf("%s", instr)==1) 
    {
        
        //Checking if admin wants to close the server
        
        if(!strcmp(instr, "exit")) 
        {
            /* clean up */
            printf("Terminating server...\n");
            pthread_mutex_destroy(&clientlist_mutex);	//destroy mutex locks
            close(sockfd);				//close the socket
            exit(0);
        }
        
        //Checking if admin wants to display the currently connected clients
        else if(!strcmp(instr, "list"))
        {
            pthread_mutex_lock(&clientlist_mutex);
            list_dump(&client_list);
            pthread_mutex_unlock(&clientlist_mutex);
        }
        
        else 
        {
            fprintf(stderr, "Unknown command: %s...\n", instr);
        }
    }
    return NULL;
}

/******************Thread Function to handle client's request******************/
void *client_handler(void *fd)
 {
    
    
    struct CLI_THREAD threadinfo = *(struct CLI_THREAD *)fd;
    struct DATA_PACKET data_packet;
    struct LLNODE *curr;
    int bytes, sent;
    
    //continuosly receiving messages from the client
    while(1)
    {
        
        //recv function to receive packet of data from client
        bytes = recv(threadinfo.sockfd, (void *)&data_packet, sizeof(struct DATA_PACKET), 0);
        
        //If server has somehow disconnected close the connection
        if(!bytes) 
        {
            fprintf(stderr, "Connection lost from [%d] %s...\n", threadinfo.sockfd, threadinfo.named);
            pthread_mutex_lock(&clientlist_mutex);
            list_delete(&client_list, &threadinfo);
            pthread_mutex_unlock(&clientlist_mutex);
            break;
        }
      
      	//Inserting user details
        if(!strcmp(data_packet.instr, "named"))
        {
            printf("Client named : %s\n", data_packet.named);
            pthread_mutex_lock(&clientlist_mutex);
            
            //traversing the list and disconnecting the anonymous client connection
            for(curr = client_list.head; curr != NULL; curr = curr->next) 
            {
                if(compare(&curr->threadinfo, &threadinfo) == 0) 
                {
                    if(strcmp(data_packet.named,"Anonymous"))
                    {
                    strcpy(curr->threadinfo.named, data_packet.named);
                    strcpy(threadinfo.named, data_packet.named);
                    break;
                    }
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }
        
        //Checking if user wants to know the list of connected clients
        else if(!strcmp(data_packet.instr,"list"))
        {
        	 struct LLNODE *curr;
        	 struct DATA_PACKET sdata_packet;
    		struct CLI_THREAD *thr_info;
    		struct LLIST *ll=&client_list;
    		//traversing the list to send connected clients info
    		for(curr = ll->head; curr != NULL; curr = curr->next) 
    		{
        		thr_info = &curr->threadinfo;
        		strcpy(sdata_packet.instr, "list");
                    	strcpy(sdata_packet.named, data_packet.named);
                    	strcpy(sdata_packet.buff, thr_info->named);
                    	sent = send(threadinfo.sockfd, (void *)&sdata_packet, sizeof(struct DATA_PACKET), 0);
    		}
        	
        }
        //Checking if client has Send a request to send a msg to another client 
        else if(!strcmp(data_packet.instr, "send:")) 
        {
            int i;
            char target[User_LEN];
            for(i = 0; data_packet.buff[i] != ' '; i++); 
            data_packet.buff[i++] = 0;
            strcpy(target, data_packet.buff);
            pthread_mutex_lock(&clientlist_mutex);
            
            //traversing the list and 
            for(curr = client_list.head; curr != NULL; curr = curr->next)
            {
                if(strcmp(target, curr->threadinfo.named) == 0)
                {
                    struct DATA_PACKET sdata_packet;
                    memset(&sdata_packet, 0, sizeof(struct DATA_PACKET));
                    if(!compare(&curr->threadinfo, &threadinfo)) 
                        continue;
                    strcpy(sdata_packet.instr, "msg");
                    strcpy(sdata_packet.named, data_packet.named);
                    strcpy(sdata_packet.buff, &data_packet.buff[i]);
                    sent = send(curr->threadinfo.sockfd, (void *)&sdata_packet, sizeof(struct DATA_PACKET), 0);
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }
        
        //Checking if client has send a request to send messages to all connected clients
         else if(!strcmp(data_packet.instr, "send-to-all")) 
         {
            pthread_mutex_lock(&clientlist_mutex);
            for(curr = client_list.head; curr != NULL; curr = curr->next)
            {
                struct DATA_PACKET sdata_packet;
                memset(&sdata_packet, 0, sizeof(struct DATA_PACKET));
                if(!compare(&curr->threadinfo, &threadinfo)) continue;
                strcpy(sdata_packet.instr, "msg");
                strcpy(sdata_packet.named, data_packet.named);
                strcpy(sdata_packet.buff, data_packet.buff);
                sent = send(curr->threadinfo.sockfd, (void *)&sdata_packet, sizeof(struct DATA_PACKET), 0);
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }
        
        //checking if client wants to exit
        else if(!strcmp(data_packet.instr, "exit")) 
        {
            printf("[%d] %s has disconnected...\n", threadinfo.sockfd, threadinfo.named);
            pthread_mutex_lock(&clientlist_mutex);
            list_delete(&client_list, &threadinfo);
            pthread_mutex_unlock(&clientlist_mutex);
            //break;
        }
        else {
            fprintf(stderr, "Garbage data from [%d] %s...\n", threadinfo.sockfd, threadinfo.named);
        }
    }
 
    // cleaning up after connection has ended
    close(threadinfo.sockfd);
 
    return NULL;
}
/*************************************END OF SERVER PROGRAM*************************************************/
