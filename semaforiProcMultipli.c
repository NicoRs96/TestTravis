/*Semplice programma in cui il processo main padre legge stringhe in input da scanf e il processo figlio le stampa
Uso dei semafori posix allocati in area di memoria condivisa creata con mmap. Faccio così perchè la fork duplica la memoria
e se i semafori non sono in una zona di memoria unica  e condivisa non funziona la sincronizzazione perchè poi ogni processo ha due suoi semafori per un totale di 4 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/stat.h> 
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>



#define SIZE 5
#define LUNGHEZZA 4096

sem_t *leggi;
sem_t *scrivi;

void *buffer;

void gestore(){
    printf("\nSegnale intercettato da %i\n", getpid());
}



int main (int argc, char** argv){

    int ret;
    printf("\nPartito!!!\n");

    /*leggi = malloc(sizeof(sem_t));
    scrivi = malloc(sizeof(sem_t));*/

    leggi = (sem_t *) mmap(NULL, SIZE*sizeof(sem_t), PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0,0);
    scrivi = (sem_t *) mmap(NULL, SIZE*sizeof(sem_t), PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0,0);

    for (int i = 0; i < SIZE; i++){
    
        sem_init(&(leggi[i]), 1, 0);
        sem_init(&(scrivi[i]), 1 ,1);
    }

    
    /*leggi = sem_open("leggi", O_CREAT, 0666, 0);
    
    if(leggi == NULL){
        printf("Errore nel semaforo leggi");
        return -1;
    }

    scrivi = sem_open("scrivi", O_CREAT, 0666, 1);
       
    if(scrivi == NULL){
        printf("Errore nel semaforo scrivi ");
        return -1;
    }*/

    signal(SIGINT, gestore);

    pid_t figlio;

    buffer =  (char *) mmap(NULL, LUNGHEZZA, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS ,0,0);

     if(buffer == MAP_FAILED){
        printf("Errore nel mmap");
        return -1;
    }

    figlio = fork();

    if(figlio == -1){
        printf("Errore nel fork");
        return -1;
    }

    if(figlio == 0) {
       
       signal(SIGINT, SIG_IGN); //altrimenti pure questo processo cattura ctrl-c

        int t = 0;

        doagain:
                        
            ret = sem_wait(&(leggi[t]));
            if(ret == -1 && errno != EINTR){
                printf("errore nel semaforo leggi  nel figlio");
                exit(EXIT_FAILURE);
            }

            printf("Sono il figlio e leggo %s\n", (char*) buffer);
            
            ret = sem_post(&(scrivi[t]));
            if(ret == -1 && errno != EINTR){
                printf("errore nel semaforo scrivi nel figlio");
                exit(EXIT_FAILURE);
            }
            
            t = (t+1)%SIZE;
            goto doagain;
        }
    

    else{   
        int j = 0;
        
        again:
            
            ret = sem_wait(&(scrivi[j]));
            
            if(ret == -1 && errno != EINTR){
                printf("errore nel semaforo scrivi nel padre");
                exit(EXIT_FAILURE);
            }

            printf("\nInserisci stringa:");
            ret = scanf("%s", (char*) buffer);
            if(ret == EOF && errno == EINTR) goto again;
            fflush(stdin);
            
            ret = sem_post(&(leggi[j]));
            if(ret == -1 && errno != EINTR){
                printf("errore nel semaforo leggi nel padre");
                exit(EXIT_FAILURE);
            }

            j = (j+1)%SIZE;
            goto again;
    }
}