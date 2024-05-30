#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> // Threads


#define MAX_LEN 1024


//struct που το περναω ως argv στο client thread function για το καθε thread
struct ThreadArgs {
    char* hostname;
    int port;
    char* name;
    char* lastname;
    char* vote;
};

void* client_thread_function(void* arg) {
    struct ThreadArgs* args = (struct ThreadArgs*)arg; //Cast το argument σε εναν struct ThreadArgs pointer

    char* hostname = args->hostname;
    int port = args->port;
    char* name = args->name;
    char* lastname = args->lastname;
    char* vote = args->vote;

    int listening_socket;
    char buffer[100];
    int readn;

    struct sockaddr_in server;
    struct sockaddr* serverptr = (struct sockaddr*)&server;
    struct hostent* rem;

    /* Create socket */
    if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        pthread_exit(NULL);
    }

    /* Find server address */
    if ((rem = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname");
        pthread_exit(NULL);
    }

    server.sin_family = AF_INET;                      /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);                    /* Server port */

    /* Initiate connection */
    if (connect(listening_socket, serverptr, sizeof(server)) < 0) {
        perror("connect problem");
        pthread_exit(NULL);
    }

    printf("Connecting to %s port %d\n", hostname, port);
    
    //Λαμβανω "SEND NAME PLEASE" απτον σερβερ
    readn = read(listening_socket, buffer, sizeof(buffer));
    if (readn <= 0) {
        perror("read problem");
        close(listening_socket);
        pthread_exit(NULL);
    }

    if (strcmp(buffer, "SEND NAME PLEASE") != 0) {
        printf("Unexpected message from the server: %s\n", buffer);
        close(listening_socket);
        pthread_exit(NULL);
    }
    printf("%s: \n", buffer);
   
    //Στελνω το ονομα στον σερβερ
    if (write(listening_socket, name, strlen(name) + 1) < 0) {
        perror("write problem");
        close(listening_socket);
        pthread_exit(NULL);
    }
  
    //Λαμβανω το μηνυμα SEND LASTNAME απτον σερβερ
    readn = read(listening_socket, buffer, sizeof(buffer));
    if (readn <= 0) {
        perror("read problem");
        close(listening_socket);
        pthread_exit(NULL);
    }
    
    //Στελνω το επιθετο στον σερβερ
    if (write(listening_socket, lastname, strlen(lastname) + 1) < 0) {
        perror("write problem");
        close(listening_socket);
        pthread_exit(NULL);
    }
   
    


    //Λαμβανω "SEND VOTE PLEASE" απτον σερβερ
    readn = read(listening_socket, buffer, sizeof(buffer));
    if (readn <= 0) {
        perror("read problem");
        close(listening_socket);
        pthread_exit(NULL);
    }

    //Αν εδω λαβω οτι εχει ψηφισει το εκτυπωνω και κλεινω τη συνδεση
    if (strcmp(buffer, "ALREADY VOTED") == 0) {
        printf("%s: \n", buffer);
        close(listening_socket); /* Close socket */
        pthread_exit(NULL);
    }

    //Αν λαβω SEND VOTE PLS στελνω την ψηφο
    if (strcmp(buffer, "SEND VOTE PLEASE") != 0) {
        printf("Unexpected message from the server: %s\n", buffer);
        close(listening_socket);
        pthread_exit(NULL);
    }
    printf("%s: \n", buffer);

    //Στελνω την ψηφο στο σερβερ
    if (write(listening_socket, vote, strlen(vote) + 1) < 0) {
        perror("write problem");
        close(listening_socket);
        pthread_exit(NULL);
    }

    //Λαμβανω VOTE FOR PARTY RECORDED απτον σερβερ εφοσον εγινε η καταχωρηση της ψηφου
    readn = read(listening_socket, buffer, sizeof(buffer));
    if (readn <= 0) {
        perror("read problem");
        close(listening_socket);
        pthread_exit(NULL);
    }

    if (strcmp(buffer, "VOTE FOR PARTY RECORDED") != 0) {
        printf("Unexpected message from the server: %s\n", buffer);
        close(listening_socket);
        pthread_exit(NULL);
    }
    printf("%s \n", buffer);

    close(listening_socket); //Κλεινω τη συνδεση
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Please give host name and port number and file\n");
        exit(1);
    }

    FILE *fp;
    FILE *fpp;
    char line1[MAX_LEN];
    char line2[MAX_LEN];
    char *token;

    int linecounter = 0;  //για να μετρησω ποσες γραμμες εχει το Input
    int linecounter2 = 0;
    int num_tokens = 0;
    char **array = (char **)malloc((3) * sizeof(char *));

    //Πρωτα το ανοιγω για να υπολογισω τις γραμμες
    fpp = fopen(argv[3], "r");
    if (fpp == NULL) {
        perror("Error opening file");
        return 1;
    }

    
    while (!feof(fpp) && !ferror(fpp)) {
        if(fgets(line1, MAX_LEN, fpp) != NULL){
            linecounter++;
        }
    }

    fclose(fpp);

    //Αφου εχω υπολογισει τις γραμμες δεσμευω τοσο χωρο οσες ειναι οι γραμμες για τα threads
    pthread_t* threads = malloc(linecounter * sizeof(pthread_t));
    if (threads == NULL) {
        perror("malloc");
        exit(1);
    }
    
    //Δεσμευω χωρο
    struct ThreadArgs* args = malloc(linecounter * sizeof(struct ThreadArgs));
    if (args == NULL) {
        perror("malloc");
        exit(1);
    }

    //Το ξανανοιγω για κανω parse τις γραμμες μια μια και να σπασω σε words name lastname και vote
    fp = fopen(argv[3], "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    while (!feof(fp) && !ferror(fp)) {
        if(fgets(line1, MAX_LEN, fp) != NULL){
            linecounter2++;
            
            //Αφαιρω την αλλαγη γραμμης
            line1[strcspn(line1, "\r\n")] = 0;
            strcpy(line2, line1);
   


            //Χωριζω τη γραμμη σε tokens χρησιμοποιωντας ως delimiter το κενο
            token = strtok(line1, " ");

            //Μετραω τα tokens
            while (token != NULL) {
                num_tokens++;
                token = strtok(NULL, " ");
            }


            //Τοποθετω το καθε token(word) στον πινακα argv
            token = strtok(line2, " ");

            for (int i = 0; token != NULL; i++){
                array[i] = malloc(sizeof(char) * (strlen(token) +1));
                strcpy(array[i], token);
                token = strtok(NULL, " ");
            }

            //Αποθηκευω στο struct μου name, lastname, vote, hostname, port
            args[linecounter2-1].name = array[0];
            args[linecounter2-1].lastname = array[1];
            args[linecounter2-1].vote = array[2];
            args[linecounter2-1].hostname = argv[1];
            args[linecounter2-1].port = atoi(argv[2]);
            
        }

    }

    //Δημιουργω τοσα threads οσες ειναι οι γραμμες
    //Του περναω σαν ορισμα το (void*)&args[i] οπου περιεχει πληροφοριες (name last name...) για το εκαστοτε thread
    for (int i = 0; i < linecounter2; i++) {
        if (pthread_create(&threads[i], NULL, client_thread_function, (void*)&args[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    //Περιμενω να τελειωσουν τα threads
    for (int i = 0; i < linecounter2; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }


    //Απελευθερωνω μνημη
    for (int i = 0; i < 3; i++) {
        free(array[i]);
    }

    free(array);
    free(threads);
    free(args);

    fclose(fp);
    
    return 0;
}

