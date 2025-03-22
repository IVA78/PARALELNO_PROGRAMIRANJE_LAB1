// mpiexec.exe -n 5 ./main.exe

#define MSMPI_NO_SAL 

#include <mpi.h>
#include <stdio.h>
#include <windows.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>


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
    boolean prev_fork = FALSE; //prethodnik ~ lijevi susjed ~ lijeva vilica
    boolean next_fork = FALSE; //sljedbenik ~ desni susjed ~ desna vilica
    boolean prev_fork_req = FALSE; //bilježenje zahtjeva za lijevu vilicu
    boolean next_fork_req = FALSE; //bilježenje zahtjeva za desnu vilicu
    boolean prev_fork_dirty = FALSE;
    boolean next_fork_dirty = FALSE;
    boolean hungry = FALSE;

    //inicijalizacija ostalih pomoćnih varijabli
    MPI_Status status;
    int flag = 0;


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
        time_t start_time = time(NULL);
        
        //generiranje nasumičnog broja sekundi između 1 i 5 sekundi
        int seconds = rand() % 5 + 1; 

        // Početno vrijeme za provjeru poruka
        time_t last_check_time = time(NULL); 

        //misli slučajan broj sekundi i provjeravaj zahtjeve dok misliš
        while(difftime(time(NULL), start_time) <= seconds) {
            Sleep(1000);
            //uvlačenje kontrolnog ispisa
            for(int i = 0; i < world_rank; i++) {
                printf("\t");
            }

            //ispis stanja filozofa
            printf("Proces %d: mislim\n", world_rank);

            //uvlačenje kontrolnog ispisa
            for(int i = 0; i < world_rank; i++) {
                printf("\t");
            }

            //provjera pristiglih poruka
            printf("Proces %d: provjera pristiglih poruka\n", world_rank);
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

            if (flag) {
                // Ako postoji poruka, primi je
                char message_recv[100];
                MPI_Recv(message_recv, sizeof(message_recv), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                // Ispis o primljenoj poruci
                for (int i = 0; i < world_rank; i++) {
                    printf("\t");
                }
                
                //odgovaranje na zahtjev 
                int left_neighbor = (world_rank == 0) ? world_size - 1 : world_rank - 1;
                int right_neighbor = (world_rank == world_size - 1) ? 0 : world_rank + 1;

                printf("Proces %d: primio zahtjev za vilicu od procesa %d (%s)\n", world_rank, status.MPI_SOURCE, (status.MPI_SOURCE == left_neighbor) ? "L" : "D");
                
                char message_send[] = "Izvoli vilicu";
                if(status.MPI_SOURCE == left_neighbor) {
                    MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, left_neighbor, 0, MPI_COMM_WORLD); //slanje prethodniku
                    prev_fork_dirty = FALSE;
                } else {
                    MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, right_neighbor + 1, 0, MPI_COMM_WORLD); //slanje sljedbeniku
                    next_fork_dirty = FALSE;
                }

            }
            
            fflush(stdout);
        }

        //sad sam gladan pa želim jesti
        hungry = TRUE;
        while(!(prev_fork && next_fork) && (!prev_fork_dirty && !next_fork_dirty)) { //sve dok nemam obje vilice i sve dok obje nisu čiste
            

            char message_send[] = "Daj mi vilicu!";

            if(!prev_fork) { //ako nemam lijeve vilice, šaljem zahtjev za nju prethodniku (lijevom susjedu)

                //uvlačenje kontrolnog ispisa
                for(int i = 0; i < world_rank; i++) {
                    printf("\t");
                }

                //ispis stanja filozofa
                int left_neighbor = (world_rank == 0) ? world_size - 1 : world_rank - 1;
                int right_neighbor = (world_rank == world_size - 1) ? 0 : world_rank + 1;
                printf("Proces %d: tražim vilicu (%d)\n", world_rank, left_neighbor);

                MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, left_neighbor, 0, MPI_COMM_WORLD);
                while(!prev_fork) { //sve dok ne dobijem vilicu, čekam poruku (bilo koju)
                    char message_recv[100];
                    MPI_Recv(message_recv, sizeof(message_recv), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                    if(strcmp(message_recv, "Daj mi vilicu")) { //stigao je zahtjev za vilicom, zabilježi ga
                        //uvlačenje kontrolnog ispisa
                        for(int i = 0; i < world_rank; i++) {
                            printf("\t");
                        }

                        
                        if(status.MPI_SOURCE == left_neighbor) {
                            prev_fork_req = TRUE; //zahtjev od prethodnika
                            //ispis stanja filozofa
                            printf("Proces %d: drugi traže moju vilicu (%d)\n", world_rank, left_neighbor);
                        } else {
                            next_fork_req = TRUE; //zahtjev od sljedbenika
                            //ispis stanja filozofa
                            printf("Proces %d: drugi traže moju vilicu (%d)\n", world_rank, right_neighbor);
                        }

                    } else { // dobila sam vilicu, ažuriraj varijable

                        //uvlačenje kontrolnog ispisa
                        for(int i = 0; i < world_rank; i++) {
                            printf("\t");
                        }

                        if(status.MPI_SOURCE == left_neighbor) {
                            prev_fork = TRUE; //zahtjev od prethodnika
                            //ispis stanja filozofa
                            printf("Proces %d: dobio sam vilicu (%d)\n", world_rank, left_neighbor);
                        } else {
                            next_fork = TRUE; //zahtjev od sljedbenika
                            //ispis stanja filozofa
                            printf("Proces %d: dobio sam vilicu (%d)\n", world_rank, right_neighbor);
                        }

                    }

                }
            }

            if(!next_fork) { //ako nemam desne vilice, šaljem zahtjev sljedbeniku (desnom susjedu)

                //uvlačenje kontrolnog ispisa
                for(int i = 0; i < world_rank; i++) {
                    printf("\t");
                }

                int left_neighbor = (world_rank == 0) ? world_size - 1 : world_rank - 1;
                int right_neighbor = (world_rank == world_size - 1) ? 0 : world_rank + 1;

                //ispis stanja filozofa
                printf("Proces %d: tražim vilicu (%d)\n", world_rank, right_neighbor);

                MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, right_neighbor, 0, MPI_COMM_WORLD);

                while(!next_fork) { //sve dok ne dobijem vilicu, čekam poruku (bilo koju)
                    char message_recv[100];
                    MPI_Recv(message_recv, sizeof(message_recv), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                    if(strcmp(message_recv, "Daj mi vilicu")) { //stigao je zahtjev za vilicom, zabilježi ga
                        //uvlačenje kontrolnog ispisa
                        for(int i = 0; i < world_rank; i++) {
                            printf("\t");
                        }

                        
                        if(status.MPI_SOURCE == left_neighbor) {
                            prev_fork_req = TRUE; //zahtjev od prethodnika
                            //ispis stanja filozofa
                            printf("Proces %d: drugi traže moju vilicu (%d)\n", world_rank, left_neighbor);
                        } else {
                            next_fork_req = TRUE; //zahtjev od sljedbenika
                            //ispis stanja filozofa
                            printf("Proces %d: drugi traže moju vilicu (%d)\n", world_rank, right_neighbor);
                        }

                    } else { // dobila sam vilicu, ažuriraj varijable
                        //uvlačenje kontrolnog ispisa
                        for(int i = 0; i < world_rank; i++) {
                            printf("\t");
                        }

                        

                        if(status.MPI_SOURCE == left_neighbor) {
                            prev_fork = TRUE; //zahtjev od prethodnika
                            //ispis stanja filozofa
                            printf("Proces %d: dobio sam vilicu (%d)\n", world_rank, left_neighbor);
                        } else {
                            next_fork = TRUE; //zahtjev od sljedbenika
                            //ispis stanja filozofa
                            printf("Proces %d: dobio sam vilicu (%d)\n", world_rank, right_neighbor);
                        }

                    }

                }
            }

            fflush(stdout);
        }


        start_time = time(NULL);
        seconds = rand() % 5 + 1;
        //jedi slučajan broj sekundi
        
        //uvlačenje kontrolnog ispisa
        for(int i = 0; i < world_rank; i++) {
            printf("\t");
        }

        //ispis stanja filozofa
        printf("Proces %d: jedem\n", world_rank);
        Sleep(seconds);
        prev_fork_dirty = TRUE;
        next_fork_dirty = TRUE;

      

        //odgovaranje na postojeće zahtjeve
        //uvlačenje kontrolnog ispisa
        for(int i = 0; i < world_rank; i++) {
            printf("\t");
        }
        printf("Proces %d: odgovaram na pristigle zahtjeve -> L: %d, D: %d\n", world_rank, prev_fork_req, next_fork_req);
        fflush(stdout);
        

        hungry = FALSE;
        char message_send[] = "Izvoli vilicu";
        int left_neighbor = (world_rank == 0) ? world_size - 1 : world_rank - 1;
        int right_neighbor = (world_rank == world_size - 1) ? 0 : world_rank + 1;
        if(prev_fork_req) {
            MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, left_neighbor, 0, MPI_COMM_WORLD); //slanje prethodniku
            prev_fork_req = FALSE;
            prev_fork = FALSE;
            prev_fork_dirty = FALSE;
        } else if(next_fork_req){
            MPI_Send(message_send, strlen(message_send) + 1, MPI_CHAR, right_neighbor, 0, MPI_COMM_WORLD); //slanje sljedbeniku
            next_fork_req = FALSE;
            next_fork = FALSE;
            next_fork_dirty = FALSE;
        }

    }

    MPI_Finalize(); //kraj programa

    return 0;
}