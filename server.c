#include <sys/socket.h>	//Sockets
#include <sys/types.h> 		//sockets
#include <dirent.h>		//Files handling
#include <netinet/in.h> //Internet sockets
#include <pthread.h>		//Threads
#include <stdio.h>			//Standard I/O library
#include <stdlib.h>		//Standard C library
#include <string.h>		//String handling
#include "queue.h"			//Queue definition
#include <netdb.h> 			//gethostbyaddr
#include <unistd.h>     	//read, write, close
#include <signal.h>			//Signals
#include <errno.h>


#define MAX_CONNECTIONS 15



//Δηλωση δεικτων τυπου FILE
FILE* pollLog;
FILE* pollStats;

//Δεικτες για το argv[4] και argv[5] δλδ τα αρχεια που θα χρησιμοποιησω
const char* pollLogName;
const char* pollStatsName;

int mainStop=0; //Για οταν λαβω control C αυξανεται κανα 1

//Struct που αποθηκευω για καθε ατομο το ονομα επιθετο και το κομμα που ψηφησε
typedef struct {
    char name[100];
	char lastname[100];
    char komma[100];
} Vote;

//Struct που αποθηκευω ποσους ψηφους ειχε καθε κομμα
typedef struct {
    char komma[100];
    int count;
} kommaCount;


Vote votes[200];
int votesCount = 0;
int kommaCounts[200];

//Τα mutexes και τα conditions που θα χρησιμοποιησω παρακατω
pthread_cond_t conditionEmpty;				//Για οταν το queue ειναι αδειο
pthread_mutex_t mutexEmpty;					//Για οταν το queue ειναι αδειο
pthread_cond_t conditionFull;				//Για οταν το queue ειναι γεματο
pthread_mutex_t mutexFull;					//Για οταν το queue ειναι γεματο


void * thread_function(void * arg);					//Η συναρτηση για τα worker threads μου
void * handle_connection(void * p_client_socket);	//Η συναρτηση που καλειται μεσα στη συναρτηση των workers
void pollStatsVotes();								//Η συναρτηση που εκτυπωνει συνολικα τα votes καθε κομματα και τα total


//Ο sig handler που εκτυπωνει τους ψηφους στο stat αρχειο
void signal_handler(int signal) {
    if (signal == SIGINT) {
		//Broadcast τα condition που περιμενουν για να ξυπνησουν, επειδη ελαβα control C
		pthread_cond_broadcast(&conditionEmpty);
		pthread_cond_broadcast(&conditionFull);

		printf("\nGot Ctrl+C\n");
        mainStop = 1;				//Αυξανω το global variable κατα 1

		//Δημιουργω το PollStats
    	pollStats = fopen(pollStatsName, "a");
		if (pollStats == NULL) {
        	perror("fopen problem stat");
        	exit(1);
    	}
        pollStatsVotes();
		fclose(pollStats);
    }
}

int main(int argc, char **argv){

	if(argc != 6) {
		printf("Not enough arguments\n");
		exit(1);
	}

	//Χειριζομαστε το signal χρησιμοποιωντας εναν handler για SIGINT
	struct sigaction sa;
	sa.sa_handler = signal_handler;
   	sigemptyset(&sa.sa_mask);

	

	queue queue; // Δηλωνω μια μεταβλητη queue

	int port = atoi(argv[1]); 			//To port απτο οποιο θα συνδεθω
	int queue_size = atoi(argv[3]); 	//Το μεγεθος των μεγιστων συνδεσεων που μπορει να δεχτει το queue   
	pollLogName = argv[4];				//To pollLog
    pollStatsName = argv[5];			//Το pollStats

    Queue_Create(&queue, queue_size); 	//Δημιουργω το queue

	pthread_mutex_init(&mutexEmpty, NULL);				//Αρχικοποιω το mutex που θα χρησιμοποιησω παρακατω
	pthread_cond_init(&conditionEmpty, NULL);   		//Αρχικοποιω το condition variable για οταν ειναι κενο το queue
	pthread_mutex_init(&mutexFull, NULL);				//Αρχικοποιω το mutex που θα χρησιμοποιησω παρακατω
	pthread_cond_init(&conditionFull, NULL);   			//Αρχικοποιω το condition variable για οταν ειναι γεματο το queue

	//Δημιουργω το PollLog
    pollLog = fopen(pollLogName, "a");
    if (pollLog == NULL) {
        perror("fopen problem log");
        exit(1);
    }



    int listening_socket;	//το socket μου
	pthread_t * threads;	//τα threads

	// Create and bind servers listening TCP Socket
	struct sockaddr_in server, client;
	socklen_t clientlen;
	struct sockaddr * serverptr = (struct sockaddr *) &server;
	struct sockaddr * clientptr = (struct sockaddr *) &client;
	struct hostent *rem;



	if((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {	// Create socket
		perror("socket");
		exit(1);
	}
	server.sin_family = AF_INET;				// Internet domain
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);			// The given port


	/* Bind socket to address */
	if (bind(listening_socket, serverptr, sizeof(server)) < 0){
		perror("bind");
		exit(1);
	}


	// Set the socket to listen for connections
	if(listen(listening_socket, MAX_CONNECTIONS) == -1) {
		perror("listen");
		exit(1);
	}

	//Δεσμευω χωρο για τοσα threads οσα πηρα απτα arguments
	threads = malloc(atoi(argv[2]) * sizeof(pthread_t));
	if(threads == NULL) {
		perror("malloc");
		exit(1);
	}

	//Δημιουργω τα threads περνοντας τους σαν ορισμα το thread_function και queue που θα χρησιμοποιησω
	for(int i = 0; i < atoi(argv[2]); i++) {
		int error;
		if((error = pthread_create(&threads[i], NULL, thread_function, &queue)) != 0) {
			perror("pthread_create problem");
			exit(1);
		}
	}

	//Ενεργοποιω το sig handler
	sigaction(SIGINT, &sa, NULL);
	while(1){

		printf("Waiting for connections...\n");
		
		int client_socket;				//το client socket που θα χρησιμοποιησω για να αποδεχτω μια συνδεση
		clientlen = sizeof(client);

		//Εαν λαβω πριν το accept control C βγαιλω απτο loop
		if (mainStop) {
			break;
    	}
		
		//accept connection
		if ((client_socket = accept(listening_socket, clientptr, &clientlen))< 0){
			if (errno == EINTR) {
            	// Accept was interrupted by a signal
            	printf("Accept was stopped by control C\n");
				pthread_cond_broadcast(&conditionEmpty); //Broadcast τα condition που περιμενουν για να ξυπνησουν, επειδη ελαβα control C
				pthread_cond_broadcast(&conditionFull);
        	}
		}

		//Εαν λαβω control C μετα το accept βγαινω απτο loop
		if(mainStop == 1){
			break;
		}
		//find clients name
		if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr,sizeof(client.sin_addr.s_addr), client.sin_family))== NULL) {
			herror("gethostbyaddr"); 
			exit(1);
		}
		printf("Accepted connection from %s\n", rem->h_name);

		//Δεσμευω χωρο για μια μεταβλητη pclient ωστε να αποθηκευσω το socket descriptor που πηρα απτην accept
		//Το βαζω μεσα στο queue
		int *pclient = malloc(sizeof(int));
		*pclient = client_socket;
		pthread_mutex_lock(&mutexFull);		//Κλειδωνω το mutex
		while(Queue_IsFull(&queue)){
			pthread_cond_wait(&conditionFull, &mutexFull);	//Αν ειναι γεματο περιμενω καποιο σημα, με το που αδειασει μια θεση, στελνω σημα
			if(mainStop == 1){
				pthread_mutex_unlock(&mutexFull);//Ξεκλειδωνω το mutex επειδη ελαβα sig int και βγαινω απτο loop
				break;
			}
		}

		enqueue(&queue, pclient);			//Βαζω τη συνδεση στο queue
		pthread_cond_broadcast(&conditionEmpty); //Δινω σημα οτι το queue δεν ειναι πλεον αλλο empty
		pthread_mutex_unlock(&mutexFull);	//Ξεκλειδωνω το mutex
	}

	//Περιμενω να τελειωσουν τα threds
	for(int i = 0; i < atoi(argv[2]); i++){
		if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
	}
	
	//Αποδεσμευω
	free(threads);
	pthread_mutex_destroy(&mutexEmpty);
	pthread_cond_destroy(&conditionEmpty);
	pthread_mutex_destroy(&mutexFull);
	pthread_cond_destroy(&conditionFull);

	fclose(pollLog);

	return 0;
	
}

void * thread_function(void * arg){
	//Μπλοκαρω το control C για τα worker threads
	sigset_t setwork;
    sigemptyset(&setwork);
    sigaddset(&setwork, SIGINT);
    pthread_sigmask(SIG_BLOCK, &setwork, NULL);

	queue *q = (queue *)arg; //Cast το argument σε εναν queue pointer
	while(1){
		pthread_mutex_lock(&mutexEmpty);		//Κλειδωσω το mutex
		while(Queue_IsEmpty(q)){
			pthread_cond_wait(&conditionEmpty, &mutexEmpty);	//Αν ειναι αδεια περιμενω καποιο signal με το που μπει μια συνδεση να την παρω
			if(mainStop == 1){
				pthread_mutex_unlock(&mutexEmpty);		//Ξεκλειδωνω το mutex επειδη ελαβα sig int και βγαινω απτο loop
				break;
			}
		}
		if(mainStop == 1){
			pthread_mutex_unlock(&mutexEmpty);		//Ξεκλειδωνω το mutex επειδη ελαβα sig int και βγαινω απτο loop
			break;
		}

		int *pclient = dequeue(q);				//Βγαζω τη συνδεση απτο queue
		pthread_cond_broadcast(&conditionFull); //Δινω σημα οτι το queue δεν ειναι πλεον αλλο γεματο
		
		handle_connection(pclient);				//Με την καθε συνδεση εκτελω μια διαδικασια οπως θα δουμε παρακατω
		pthread_mutex_unlock(&mutexEmpty);		//Ξεκλειδωνω το mutex
	}
	return 0;
}

void* handle_connection(void* p_client_socket) {

    int client_socket = *(int*)p_client_socket;
    char buffer[100];

    //Στελνω στον client το μηνυμα SEND NAME PLEASE
    write(client_socket, "SEND NAME PLEASE", sizeof("SEND NAME PLEASE"));

    //Λαμβανω το ονομα απτον client
    int readn = read(client_socket, buffer, sizeof(buffer));
    if (readn <= 0) {
        close(client_socket);
		perror("read");
        exit(1);
    }

	//Αντιγραφω το ονομα που ελαβα σε ενα πινακα name
    char name[100];
    strncpy(name, buffer, sizeof(name));
	memset(buffer, 0, sizeof(buffer));			//Αδειαζω το buffer για να το ξαναχρησιμοποιησω

	//Στελνω στον client το μηνυμα SEND LASTNAME PLEASE
	//Ειναι δικια μου προσθηκη αυτη η παραδοχη για να παιρνω ξεχωριστα το last name
	write(client_socket, "SEND LASTNAME", sizeof("SEND LASTNAME"));
	

	//Λαμβανω το επωνυμο απτον client
    readn = read(client_socket, buffer, sizeof(buffer));
    if (readn <= 0) {
        close(client_socket);
		perror("read");
        exit(1);
    }

	//Αντιγραφω το ονομα που ελαβα σε ενα πινακα lastname
    char lastname[100];
    strncpy(lastname, buffer, sizeof(lastname));
	memset(buffer, 0, sizeof(buffer));			//Αδειαζω το buffer για να το ξαναχρησιμοποιησω
	
	//Ελεγχω εαν καποιο απο τα ατομα που εχω καταχωρησει οτι εχουν ψηφισει συμπιπτει με αυτο που εξεταζω τωρα
	//Αν δεν συμπιπτει σημαινει οτι δεν εχει ψηφισει οποτε συνεχιζω τη διαδικασια
    int alreadyVoted = 0;
    for (int i = 0; i < votesCount; i++) {
        if (strcmp(votes[i].name, name) == 0 && strcmp(votes[i].lastname, lastname) == 0) {
            alreadyVoted = 1;
            break;
        }
    }

	//Αν ομως εχει ψηφισει στελνω το μηνυμα στον client και τερματιζω τη συνδεση
    if (alreadyVoted) {
        write(client_socket, "ALREADY VOTED", sizeof("ALREADY VOTED"));
        close(client_socket);
        return NULL;
    }

    //Στελνω στον client το μηνυμα SEND VOTE PLEASE
    write(client_socket, "SEND VOTE PLEASE", sizeof("SEND VOTE PLEASE"));

    //Λαμβανω την ψηφο του
    readn = read(client_socket, buffer, sizeof(buffer));
    if (readn <= 0) {
        close(client_socket);
        return NULL;
    }

	//Αντιγραφω την ψηφο που ελαβα σε ενα πινακα komma
    char komma[100];
    strncpy(komma, buffer, sizeof(komma));

	
    //Αποθηκευω στο struct για το συγκεκριμενο ατομο το ονομα, επιθετο και το κομμα που ψηφισε
    strncpy(votes[votesCount].name, name, sizeof(votes[votesCount].name));
	strncpy(votes[votesCount].lastname, lastname, sizeof(votes[votesCount].lastname));
    strncpy(votes[votesCount].komma, komma, sizeof(votes[votesCount].komma));
    votesCount++;


	//Γραφω στο pollLog το ονοματα επιθετο και αυτο που ψηφισε
    for (int i = votesCount - 1; i < votesCount; i++) {
        fprintf(pollLog, "%s %s %s\n", votes[i].name, votes[i].lastname, votes[i].komma);
    }
	fflush(pollLog);

	//Στελνω το μηνυμα οτι καταχωρθηκε η ψηφος
	write(client_socket, "VOTE FOR PARTY RECORDED", sizeof("VOTE FOR PARTY RECORDED"));
	close(client_socket);
	
	return NULL;
}



void pollStatsVotes() {
	
    //Υπολογιζω τις ψηφους καθε κομματος
    kommaCount kommaCounts[100];
    int kommaCount = 0;

    for (int i = 0; i < votesCount; i++) {
        int kommaflag = -1;

        //Ελεγχω αν εχει μετρηθει ξανα το κομμα που ελεγχω
        for (int j = 0; j < kommaCount; j++) {
            if (strcmp(kommaCounts[j].komma, votes[i].komma) == 0) {
                kommaflag = j;
                break;
            }
        }

        //Εαν δεν βρεθηκε καποιο κομμα το προσθετω στο στρακτ
        if (kommaflag == -1) {
            strncpy(kommaCounts[kommaCount].komma, votes[i].komma, sizeof(kommaCounts[kommaCount].komma));
            kommaCounts[kommaCount].count = 1;
            kommaCount++;
        } else {							//Εαν υπαρχει ηδη στο στρακτ απλα αυξανω την τιμη του κατα ενα
            kommaCounts[kommaflag].count++;
        }
    }

	
	//Γραφω στο stats ποσους ψηφους ειχε καθε κομμα
    for (int i = 0; i < kommaCount; i++) {
		fprintf(pollStats, "%s %d\n", kommaCounts[i].komma, kommaCounts[i].count);
    }

	//Εκτυπωνω τους συνολικους ψηφους
    fprintf(pollStats, "TOTAL %d\n", votesCount);
}
