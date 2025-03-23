// mpiexec.exe -n 5 ./main.exe

#define MSMPI_NO_SAL 


#include <mpi.h>
#include <stdio.h>
#include <windows.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0



int main(int argc, char** argv) {
    // inicijalizacija MPI okruženja 
    MPI_Init(NULL, NULL);

    // dohvat ukupnog broja procesa u grupi procesa MPI_COMM_WORLD
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // dohvat trenutnog ranga procesa: 0, 1, 2... i 
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    //provjera: potrebna su najmanje dva procesa za algoritam
    if(world_size < 2) {
        fprintf(stderr, "Za rad algoritma potrebna su najmanje dva procesa, trenutno: %d", world_size);
        fflush(stderr); 
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    //inicijalizacija pomoćnih boolean varijabli
    boolean prev_fork = FALSE; //prethodnik ~ lijevi susjed ~ lijeva vilica ( FALSE -> nemam lijevu vilicu)
    boolean next_fork = FALSE; //sljedbenik ~ desni susjed ~ desna vilica (FALSE -> nemam desnu vilicu)
    boolean prev_fork_req = FALSE; //bilježenje zahtjeva za lijevu vilicu
    boolean next_fork_req = FALSE; //bilježenje zahtjeva za desnu vilicu
    boolean prev_fork_dirty = TRUE; //sve vilice na početku prljave
    boolean next_fork_dirty = TRUE; // sve vilice na početku prljave
    boolean prev_fork_clean = FALSE; //sve vilice na početku prljave
    boolean next_fork_clean = FALSE; // sve vilice na početku prljave
    boolean hungry = FALSE;

    /*
    -> ako je vilica prljava, može ju dati susjedu kada ju zatraži
    -> ako je vilica čista, filozof je dobio vilicu i ne može ju odmah dati
    */

    //inicijalizacija ostalih pomoćnih varijabli
    MPI_Status status;
    int flag = 0;
    int left_neighbor = (world_rank == 0) ? world_size - 1 : world_rank - 1;
    int right_neighbor = (world_rank == world_size - 1) ? 0 : world_rank + 1;
    time_t start_time;
    int seconds;


    //dodjela vilice svakom procesu
    if(world_rank == 0) { //samo 0-ti proces ima vilicu od prethodnika (od zadnjeg)
        prev_fork = TRUE;
    } else {
        prev_fork = FALSE;
    }

    if(world_rank == world_size - 1) { //samo n-ti proces nema vilicu od sljedbenika (od prvog)
        next_fork = FALSE;
    } else {
        next_fork = TRUE;
    }

    /*
    Očekivano:
    0: 1,1 -> ima obje
    1: 0, 1 -> ima samo desnu (lijevu od sljedbenika)
    2: 0, 1 -> ima samo desnu (lijevu od sljedbenika)
    ...
    n: 0,0 -> nema niti jednu
    */
    printf("Proces %d ima sljedeće stanje vilica: prev -> %d, next -> %d", world_rank, prev_fork, next_fork);
    fflush(stdout);

    //sinkronizacija procesa nakon podjele viljuški
    MPI_Barrier(MPI_COMM_WORLD); 
    
    
    while(1) {//zauvijek

        //dohvat početnog vremena
        start_time = time(NULL);
        
        //generiranje nasumičnog broja sekundi između 1 i 5 sekundi
        seconds = rand() % 5 + 1; 

        //misli nasumičan broj sekundi------------------------------

        //uvlačenje kontrolnog ispisa
        for(int i = 0; i < world_rank; i++) {
            printf("\t");
        }

        //ispis stanja filozofa
        printf("Proces %d: mislim (moje vilice: %d, %d)\n", world_rank, prev_fork, next_fork);
        fflush(stdout);

        
        //provjera pristiglih zahtjeva za vrijeme razmišljanja
        while(difftime(time(NULL), start_time) <= seconds){

            //uvlačenje kontrolnog ispisa
            for(int i = 0; i < world_rank; i++) {
                printf("\t");
            }

            //provjera starijih zahtjeva i odgovaranje na njih ???? 

            //provjera pristiglih poruka
            printf("Proces %d: provjera pristiglih poruka dok mislim (moje vilice: %d, %d)\n", world_rank, prev_fork, next_fork);
            fflush(stdout);
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);


            if(flag) { //postoji poruka koju mogu primiti

                char message_recv[100];
                MPI_Recv(message_recv, sizeof(message_recv), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                if(strcmp(message_recv, "Izvoli vilicu") == 0) {

                    // Ispis o primljenoj poruci
                    for (int i = 0; i < world_rank; i++) {
                        printf("\t");
                    }
                    

                    if(status.MPI_SOURCE == left_neighbor && !prev_fork) { //dobijam vilicu i označujem da je čista
                        prev_fork = TRUE;
                        prev_fork_clean = TRUE;
                        prev_fork_dirty = FALSE;
                    } else if(status.MPI_SOURCE == right_neighbor) {
                        next_fork = TRUE;
                        next_fork_clean = TRUE;
                        next_fork_dirty = FALSE;
                    }

                    printf("Proces %d: primio vilicu od procesa %d (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                    fflush(stdout);

                } else if (strcmp(message_recv, "Daj mi vilicu") == 0) {
                    // Ispis o primljenoj poruci
                    for (int i = 0; i < world_rank; i++) {
                        printf("\t");
                    }
                    printf("Proces %d: primio zahtjev za vilicu od procesa %d (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                    fflush(stdout);

                    if(status.MPI_SOURCE == left_neighbor && prev_fork) {
                        if(prev_fork_dirty) { //dajem prljavu vilicu
                            prev_fork = FALSE;
                            char message_send[] = "Izvoli vilicu";
                            MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, left_neighbor, 0, MPI_COMM_WORLD);  
                            // Ispis o primljenoj poruci
                            for (int i = 0; i < world_rank; i++) {
                                printf("\t");
                            }
                            printf("Proces %d: odgovara na zahtjev za vilicu od procesa %d (L) (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                            fflush(stdout);
                        } else { //nemam prljavu vilicu pa bilježim zahtjev
                            prev_fork_req = TRUE;
                            // Ispis o primljenoj poruci
                            for (int i = 0; i < world_rank; i++) {
                                printf("\t");
                            }
                            printf("Proces %d: bilježi zahtjev za vilicu od procesa %d (L) (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                            fflush(stdout);
                        }

                    } else if(status.MPI_SOURCE == right_neighbor && next_fork) {
                        if(next_fork_dirty) {
                            next_fork = FALSE;
                            char message_send[] = "Izvoli vilicu";
                            MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, right_neighbor, 0, MPI_COMM_WORLD);  
                            // Ispis o primljenoj poruci
                            for (int i = 0; i < world_rank; i++) {
                                printf("\t");
                            }
                            printf("Proces %d: odgovara na zahtjev za vilicu od procesa %d (D) (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                            fflush(stdout);
                        } else {
                            next_fork_req = TRUE;
                            // Ispis o primljenoj poruci
                            for (int i = 0; i < world_rank; i++) {
                                printf("\t");
                            }
                            printf("Proces %d: bilježi zahtjev za vilicu od procesa %d (D) (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                            fflush(stdout);
                        }

                    }

                }

            }
            
            Sleep(1000); //svakih sekundu
        }

    
        //želim jesti, tražim vilice---------------------------------------
        
        while(!(prev_fork && next_fork)) {

            //last asked varijabla: ja sam zadnji pitao za vilicu
            

            //nitko me nije pitao, smijem ja traziti
            if(!prev_fork) {
                //uvlačenje kontrolnog ispisa
                for(int i = 0; i < world_rank; i++) {
                    printf("\t");
                }
                //ispis stanja filozofa
                printf("Proces %d: tražim vilicu (L:%d) (moje vilice: %d, %d)\n", world_rank, left_neighbor, prev_fork, next_fork);
                fflush(stdout);
    
                char message_send[] = "Daj mi vilicu";
                MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, left_neighbor, 0, MPI_COMM_WORLD);
                
            }

        
            if(!next_fork) {
                //uvlačenje kontrolnog ispisa
                for(int i = 0; i < world_rank; i++) {
                    printf("\t");
                }
    
                //ispis stanja filozofa
                printf("Proces %d: tražim vilicu (D: %d) (moje vilice: %d, %d)\n", world_rank, right_neighbor, prev_fork, next_fork);
                fflush(stdout);
    
                char message_send[] = "Daj mi vilicu";
                MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, right_neighbor, 0, MPI_COMM_WORLD);
                
               
            }

            //uvlačenje kontrolnog ispisa
            for(int i = 0; i < world_rank; i++) {
                printf("\t");
            }

            //provjera pristiglih poruka
            printf("Proces %d: provjera pristiglih poruka dok čekam vilice (moje vilice: %d, %d)\n", world_rank, prev_fork, next_fork);
            fflush(stdout);
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);


            if(flag) { //postoji poruka koju mogu primiti

                char message_recv[100];
                MPI_Recv(message_recv, sizeof(message_recv), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                if(strcmp(message_recv, "Izvoli vilicu") == 0) {

                    // Ispis o primljenoj poruci
                    for (int i = 0; i < world_rank; i++) {
                        printf("\t");
                    }
                    

                    if(status.MPI_SOURCE == left_neighbor && !prev_fork) { //dobijam vilicu i označujem da je čista
                        prev_fork = TRUE;
                        prev_fork_clean = TRUE;
                        prev_fork_dirty = FALSE;
                    } else if(status.MPI_SOURCE == right_neighbor) {
                        next_fork = TRUE;
                        next_fork_clean = TRUE;
                        next_fork_dirty = FALSE;
                    }

                    printf("Proces %d: primio vilicu od procesa %d (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                    fflush(stdout);

                } else if (strcmp(message_recv, "Daj mi vilicu") == 0) {
                    // Ispis o primljenoj poruci
                    for (int i = 0; i < world_rank; i++) {
                        printf("\t");
                    }
                    printf("Proces %d: primio zahtjev za vilicu od procesa %d (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                    fflush(stdout);

                    if(status.MPI_SOURCE == left_neighbor && prev_fork) {
                        if(prev_fork_dirty) { //dajem prljavu vilicu
                            prev_fork = FALSE;
                            char message_send[] = "Izvoli vilicu";
                            MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, left_neighbor, 0, MPI_COMM_WORLD);  
                            // Ispis o primljenoj poruci
                            for (int i = 0; i < world_rank; i++) {
                                printf("\t");
                            }
                            printf("Proces %d: odgovara na zahtjev za vilicu od procesa %d (L) (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                            fflush(stdout);
                        } else { //nemam prljavu vilicu pa bilježim zahtjev
                            prev_fork_req = TRUE;
                            // Ispis o primljenoj poruci
                            for (int i = 0; i < world_rank; i++) {
                                printf("\t");
                            }
                            printf("Proces %d: bilježi zahtjev za vilicu od procesa %d (L) (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                            fflush(stdout);
                        }

                    } else if(status.MPI_SOURCE == right_neighbor && next_fork) {
                        if(next_fork_dirty) {
                            next_fork = FALSE;
                            char message_send[] = "Izvoli vilicu";
                            MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, right_neighbor, 0, MPI_COMM_WORLD);  
                            // Ispis o primljenoj poruci
                            for (int i = 0; i < world_rank; i++) {
                                printf("\t");
                            }
                            printf("Proces %d: odgovara na zahtjev za vilicu od procesa %d (D) (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                            fflush(stdout);
                        } else {
                            next_fork_req = TRUE;
                            // Ispis o primljenoj poruci
                            for (int i = 0; i < world_rank; i++) {
                                printf("\t");
                            }
                            printf("Proces %d: bilježi zahtjev za vilicu od procesa %d (D) (moje vilice: %d, %d)\n", world_rank, status.MPI_SOURCE, prev_fork, next_fork);
                            fflush(stdout);
                        }

                    }

                }

            }
            
            seconds = rand() % 3 + 1;
            Sleep(seconds * 1000);

            


        }

        //jedi nasumičan broj sekundi - tad ne provjeravaš zahtjeve-----------------------
        seconds = rand() % 5 + 1;
        
        //uvlačenje kontrolnog ispisa
        for(int i = 0; i < world_rank; i++) {
            printf("\t");
        }

        //ispis stanja filozofa
        printf("Proces %d: jedem (moje vilice: %d, %d)\n", world_rank, prev_fork, next_fork);
        Sleep(seconds * 1000);
        prev_fork_clean = FALSE;
        prev_fork_dirty = TRUE;
        next_fork_clean = FALSE;
        next_fork_dirty = TRUE;

        // odgovaranje na postojeće zahtjeve------------------------
        //uvlačenje kontrolnog ispisa
        for(int i = 0; i < world_rank; i++) {
            printf("\t");
        }
        printf("Proces %d: odgovaram na postojeće zahtjeve -> L: %d, D: %d (moje vilice: %d, %d)\n", world_rank, prev_fork_req, next_fork_req, prev_fork, next_fork);
        fflush(stdout);

        char message_send[] = "Izvoli vilicu";
        if(prev_fork_req) {
            MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, left_neighbor, 0, MPI_COMM_WORLD); //slanje prethodniku
            prev_fork_req = FALSE;
            prev_fork = FALSE;
        } else if(next_fork_req){
            MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, right_neighbor, 0, MPI_COMM_WORLD); //slanje sljedbeniku
            next_fork_req = FALSE;
            next_fork = FALSE;
        }

        
    }

    MPI_Finalize(); //kraj programa

    return 0;
}