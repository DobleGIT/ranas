/* 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
|                            |
|            batracios.C            |
|                            |
|     |
|                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include "batracios.h"
  
////:::::FUNCIONES::::://

void manejadora(int semid, int memid);
void waitSemaf(int semid, int sem_num);
void signalSemaf(int semid, int sem_num);
int valorSemaf(int semid, int sem_num);
void tratarSigint(int numsen);
void ranaMadre(int numeroRana);
void ranaHija(int numeroRanaHija,int *dx, int *dy);

//:::::VARIABLES GLOBALES::::://

int NUMMADRES=4;
int PROCMAX=25;
int lentitud, descansar;
int *pid = NULL; ////Vector con los pids de los procesos creados
int idMemoria=-1, idSemaforo=-1;
int *puntero=NULL;
struct posiciOn *ranaVector=NULL;
int *salvadas = NULL,*nacidas = NULL,*perdidas = NULL;

//:::::STRUCT SEMAFOROS SOLARIS::::://
union semun 
        {int             val;
         struct semid_ds *buf;
         //ushort_t	*array;
    };

////:::::::::::::::::::::::::::::::::::::::::::MAIN::::::::::::::::::::::::::::::::::::::::::://
int main (int argc, char * argv[]){
    
    
    struct sigaction ss;

    int i, valor_devuelto,j;
    struct sembuf wait1,wait2,wait3,signal1,signal2,signal3,wait4,signal4;

    union semun sem1;

    ////:::::OPERACION SEMAFOROS::::://
    ////Operacion de restar al semaforo 3//
    wait3.sem_num=3;
    wait3.sem_op= -1;
    wait3.sem_flg=0;

    ////Operacion de sumar al semaforo 3//
    signal3.sem_num=3;
    signal3.sem_op=1;
    signal3.sem_flg=0;

    ////:::::COMPROBACION ARGUMENTOS::::://
    if(argc!=3){
        perror("\nERROR en los argumentos del programa: El programa debe tener dos argumentos\n");
        return -1;
    }

    lentitud = atoi(argv[1]);

    if(lentitud < 0 || lentitud > 1000){
        perror("\nERROR en los argumentos del programa: La lentitud del programa debe estar comprendida entre 0 y 1000\n");
        return -1;
    }
    descansar = atoi(argv[2]);

    if(descansar  < 0){
        perror("\nERROR en los argumentos del programa:El descanso entre partos de la rana madre debe ser mayor que 0 \n");
        return -1;
    }
    
    //:::::REDEFINIR SEÑAL SIGINT ::::://
    ss.sa_handler=tratarSigint;
    sigemptyset(&ss.sa_mask);
    ss.sa_flags=0;

    if(sigaction(SIGINT, &ss, NULL)==-1){
        perror("\nERROR en la redefinición de SIGINT\n");
        return -1;
    }
    
    //--> NOTA: A partir de aquí al pulsar CTRL-C saltará a la funcion tratarctrlc//

    //:::::CREACION MEMORIA COMPARTIDA::::://
    idMemoria=shmget(IPC_PRIVATE, 2060*(sizeof(int)), IPC_CREAT | 0600);

    if(idMemoria == -1){
        perror("error de creación de memoria compartida");
        kill(getpid(), SIGINT);
    }
    puntero=(int *)shmat(idMemoria,0,0);
    
    if(*(puntero)==-1){
        perror("\nError en la asignacion de puntero a la memoria compartida\n");
        kill(getpid(), SIGINT);
    }
    
    puntero[2050]=0; ////contador de procesos
    puntero[2051]=0; ////nacidas
    puntero[2052]=0; ////salvadas
    puntero[2053]=0; ////perdidas
    ranaVector = &puntero[2056];
    //FIN CREACION Y ASIGNACION DE MEMORIA COMPARTIDA//

    //:::::CREACION SEMAFOROS::::://
    idSemaforo=semget(IPC_PRIVATE, 4, IPC_CREAT | 0600);

    if(idSemaforo == -1){
        perror("Error creación semáforos \n");
        kill(getpid(), SIGINT);
    }
    
    sem1.val=1;
    if(semctl(idSemaforo,1,SETVAL,sem1)==-1){
        perror("error al dar valor semáforo 1\n");
        kill(getpid(), SIGINT);
    }

    sem1.val=19; 
    if(semctl(idSemaforo,2,SETVAL,sem1)==-1){
    perror("error al dar valor semáforo 2 \n");
        kill(getpid(), SIGINT);
    }

    sem1.val=1;
    if(semctl(idSemaforo,3,SETVAL,sem1)==-1){
        perror("error al dar valor semáforo 3 \n");
        kill(getpid(), SIGINT);
    }

    int lTroncos[7] ={1,5,3,6,4,7,3};
    int lAguas[7]= {1,2,4,3,6,5,4};
    int dirs[7] = {0,1,0,1,0,1,0};

    if((BATR_inicio(lentitud, idSemaforo,lTroncos, lAguas,dirs, descansar, puntero))==-1){
        perror("error en inicio batracio\n");
        return 5;
    }
 
    //::::: CREACION DE LOS PROCESOS HIJOS ::::://
    pid=(int*)malloc((250)*sizeof(int)); 

    if(pid==NULL){
        perror("\nERROR en la reserva de memoria dinámina\n");
        return -1;
    }

    pid[0]=getpid();

    for(i=1;i<=4;i++){
        switch(fork()){
            case -1:
                perror("\nError en la creación de la rana madre\n");
                return -1;
                break;
            case 0:
                //* PROCESO HIJO (nuevo madre) *//
                pid[(puntero[2050]+1)]=(int)getpid();
                ranaMadre(i);
                break;
            default:
                
                break;
        }////fin del switch
    }////fin del for
    
    //::::: MOVIMIENTO DE LOS TRONCOS ::::://
    while(1) 
    {  
        if(semop(idSemaforo, &wait3, 1)==-1){
            perror("\nERROR en el semáforo wait3 3\n");
            kill(getppid(), SIGINT);
        }
        
        BATR_pausita();
        BATR_avance_troncos(0);
        BATR_pausita();
        BATR_avance_troncos(1);
        BATR_pausita();
        BATR_avance_troncos(2);
        BATR_pausita();
        BATR_avance_troncos(3);
        BATR_pausita();
        BATR_avance_troncos(4);
        BATR_pausita();
        BATR_avance_troncos(5);
        BATR_pausita();
        BATR_avance_troncos(6);
        BATR_pausita();
        int q;
        for(q=0;q<=puntero[2050];q++) //// comprobamos la posicion de cada rana para sumarle o restarle segun el tronco en el que este
        {    
            if((ranaVector[q].y)>3 && (ranaVector[q].y)<11)
            {
                if(((ranaVector[q].y)%2)==0)
                {
                    (ranaVector[q].x)=(ranaVector[q].x)+1;
                }else
                {
                    (ranaVector[q].x)=(ranaVector[q].x)-1;
                }
            }
        }
        
        if(semop(idSemaforo, &signal3, 1)==-1){
            perror("\nERROR en el semáforo signal 3\n");
            kill(getppid(), SIGINT);
        }
            sleep(1); ////este sleep es necesario para que funcione en ENCINA
    }////fin del for
    getchar();
}////fin del main

//:::::FUNCION PARA LA CREACION DE LAS RANAS MADRE::::://
void ranaMadre(int numeroRana){

    struct sembuf wait1,wait2,wait3,signal1,signal2,signal3,wait4,signal4;
    int ranaMadre,posi;


    //Operacion de restar al semaforo 1//
    wait1.sem_num=1;
    wait1.sem_op= -1;
    wait1.sem_flg=0;

    //Operacion de sumar al semaforo 1//
    signal1.sem_num=1;
    signal1.sem_op= 1;
    signal1.sem_flg=0;

    //Operacion de restar al semaforo 2//
    wait2.sem_num=2;
    wait2.sem_op= -1;
    wait2.sem_flg=0;

    //Operacion de sumar al semaforo 3//
    signal2.sem_num=2;
    signal2.sem_op= 1;
    signal2.sem_flg=0;

    //Operacion de restar al semaforo 3//
    wait3.sem_num=3;
    wait3.sem_op= -1;
    wait3.sem_flg=0;

    //Operacion de sumar al semaforo 3//
    signal3.sem_num=3;
    signal3.sem_op=1;
    signal3.sem_flg=0;

    int *dx,*dy,j,w;
    int primeraRanita=0; ////si no se ha creado ninguna ranita se salta el wait 1
    
    BATR_descansar_criar();

    if(BATR_parto_ranas((numeroRana-1),&dx,&dy)==-1){
        perror("\nERROR ranamadre\n");
        kill(getppid(), SIGINT);
    }

    while(1){

        if(semop(idSemaforo, &wait2, 1)==-1){
            perror("\nERROR en el semáforo wait 2\n");
            kill(getppid(), SIGINT);
        }
        if(primeraRanita != 0)
        {   
            if(semop(idSemaforo, &wait1, 1)==-1){
                perror("\nERROR en el semáforo wait 1\n");
                kill(getppid(), SIGINT);
            }
            
            BATR_descansar_criar();

            if(BATR_parto_ranas((numeroRana-1),&dx,&dy)==-1){
                perror("\nERROR ranamadre\n");
                kill(getppid(), SIGINT);
            }
        }
        primeraRanita = 1;
    
        switch(fork()){
            case -1:
                perror("\nError en la creación de la rana hija\n");
                return -1;
                break;
            case 0:
                //* PROCESO HIJO (nuevo ranita) *//
                puntero[2050]++;
                posi = (puntero[2050]);
                pid[posi]=(int)(getpid());
                ranaHija(numeroRana+1,dx,dy);
                break;
            default:
                //* PROCESO PADRE *//
                break;
        }
        sleep(1);
    }

}////fin funcion ranaMadre

void ranaHija(int numeroRanaHija,int *dx, int *dy){

    struct sembuf wait1,wait2,wait3,signal1,signal2,signal3,wait4,signal4;

    int posicionVector = puntero[2050];

    //Operacion de restar al semaforo 1//
    wait1.sem_num=1;
    wait1.sem_op= -1;
    wait1.sem_flg=0;

    //Operacion de sumar al semaforo 1//
    signal1.sem_num=1;
    signal1.sem_op= 1;
    signal1.sem_flg=0;

    //Operacion de restar al semaforo 2//
    wait2.sem_num=2;
    wait2.sem_op= -1;
    wait2.sem_flg=0;

    //Operacion de sumar al semaforo 3//
    signal2.sem_num=2;
    signal2.sem_op= 1;
    signal2.sem_flg=0;

    //Operacion de restar al semaforo 3//
    wait3.sem_num=3;
    wait3.sem_op= -1;
    wait3.sem_flg=0;

    //Operacion de sumar al semaforo 3//
    signal3.sem_num=3;
    signal3.sem_op=1;
    signal3.sem_flg=0;

    puntero[2051]++;
  
    if(BATR_puedo_saltar(dx,dy,ARRIBA)==0){

        BATR_avance_rana_ini(dx,dy);
        BATR_avance_rana(&dx,&dy,ARRIBA);
        BATR_pausa();
        BATR_avance_rana_fin(dx,dy);

        if(semop(idSemaforo, &signal1, 1)==-1){
            perror("\nERROR en el semáforo wait 1\n");
            kill(getppid(), SIGINT);
        }
            
    }else if(BATR_puedo_saltar(dx,dy,IZQUIERDA)==0){

        BATR_avance_rana_ini(dx,dy);
        BATR_avance_rana(&dx,&dy,IZQUIERDA);
        BATR_pausa();
        BATR_avance_rana_fin(dx,dy);

        if(semop(idSemaforo, &signal1, 1)==-1){
            perror("\nERROR en el semáforo wait 1\n");
            kill(getppid(), SIGINT);
        }           
    }else if(BATR_puedo_saltar(dx,dy,DERECHA)==0){

        BATR_avance_rana_ini(dx,dy);
        BATR_avance_rana(&dx,&dy,DERECHA);
        BATR_pausa();
        BATR_avance_rana_fin(dx,dy);

        if(semop(idSemaforo, &signal1, 1)==-1){
            perror("\nERROR en el semáforo wait 1\n");
            kill(getppid(), SIGINT);
        }
    }

    ranaVector[posicionVector].x = dx;
    ranaVector[posicionVector].y = dy;
    
    while(1)
    {
    
        if(semop(idSemaforo, &wait3, 1)==-1){
            perror("\nERROR en el semáforo wait3 3\n");
            kill(getppid(), SIGINT);
        }
        
        if(ranaVector[posicionVector].x < 0 || ranaVector[posicionVector].x > 79)
        {
            puntero[2053]++;

            if(semop(idSemaforo, &signal3, 1)==-1){
                perror("\nERROR en el semáforo signal 2\n");
                kill(getppid(), SIGINT);
            }

            if(semop(idSemaforo, &signal2, 1)==-1){
                perror("\nERROR en el semáforo signal 2\n");
                kill(getppid(), SIGINT);
            }  

            kill(getppid(), SIGINT);

            return 1;
        }else if((ranaVector[posicionVector].y) == 11)
        {
            if(semop(idSemaforo, &signal3, 1)==-1){
                perror("\nERROR en el semáforo signal 2\n");
                kill(getppid(), SIGINT);
            }

            if(semop(idSemaforo, &signal2, 1)==-1){
                perror("\nERROR en el semáforo signal 2\n");
                kill(getppid(), SIGINT);
            }

            kill(getppid(), SIGINT);
            
            return 1;
        }else if(BATR_puedo_saltar((ranaVector[posicionVector].x),(ranaVector[posicionVector].y),ARRIBA)==0){
                    
                    
            BATR_avance_rana_ini((ranaVector[posicionVector].x),(ranaVector[posicionVector].y));
            BATR_avance_rana(&(ranaVector[posicionVector].x),&(ranaVector[posicionVector].y),ARRIBA);
            BATR_pausa();
            BATR_avance_rana_fin((ranaVector[posicionVector].x),(ranaVector[posicionVector].y));
                  
            if(semop(idSemaforo, &signal3, 1)==-1){
                perror("\nERROR en el semáforo signal 3\n");
                kill(getppid(), SIGINT);
            }
                
            if((ranaVector[posicionVector].y) == 11){

                puntero[2052]++;

                if(semop(idSemaforo, &signal2, 1)==-1){
                    perror("\nERROR en el semáforo signal 2\n");
                    kill(getppid(), SIGINT);
                }

                kill(getppid(), SIGINT);
                return 1;
            }
        }else if(BATR_puedo_saltar((ranaVector[posicionVector].x),(ranaVector[posicionVector].y),IZQUIERDA)==0){
            BATR_avance_rana_ini((ranaVector[posicionVector].x),(ranaVector[posicionVector].y));
            BATR_avance_rana(&(ranaVector[posicionVector].x),&(ranaVector[posicionVector].y),IZQUIERDA);
            BATR_pausa();
            BATR_avance_rana_fin((ranaVector[posicionVector].x),(ranaVector[posicionVector].y));

            if(semop(idSemaforo, &signal3, 1)==-1){
                perror("\nERROR en el semáforo signal 2\n");
                kill(getppid(), SIGINT);
            }
                    
        }else if(BATR_puedo_saltar((ranaVector[posicionVector].x),(ranaVector[posicionVector].y),DERECHA)==0){
            BATR_avance_rana_ini((ranaVector[posicionVector].x),(ranaVector[posicionVector].y));
            BATR_avance_rana(&(ranaVector[posicionVector].x),&(ranaVector[posicionVector].y),DERECHA);
            BATR_pausa();
            BATR_avance_rana_fin((ranaVector[posicionVector].x),(ranaVector[posicionVector].y));
            
            if(semop(idSemaforo, &signal3, 1)==-1){
                perror("\nERROR en el semáforo signal 2\n");
                kill(getppid(), SIGINT);
            }
        }else{

            BATR_pausa();
            if(semop(idSemaforo, &signal3, 1)==-1){
                perror("\nERROR en el semáforo signal 2\n");
                kill(getppid(), SIGINT);
            }                 
        }
    sleep(1);
    }
}

//:::::FUNCION TRATAMIENTO SIGINT::::://
void tratarSigint(int numsen){

    //* Variable contadora *//
    int i;

    //* DETIENE TODOS LOS PROCESOS MENOS EL PRINCIPAL *//
    if(pid[0]==getpid()){

        wait(0);
        //* MATA TODOS LOS PROCESOS HIJOS *//
    
        for(i=250;i<=1;i--){
            kill(pid[i], SIGKILL);
        }
    
        //:::::ELIMINACION DE LOS MECANISMOS IPC::::://
        //* --> Semaforos *//
        if(idSemaforo!=-1){

            if(semctl(idSemaforo, 0, IPC_RMID)==-1){
                perror("\nError en la eliminación de los semáforos\n");
                exit(-1);
            }

            idSemaforo=-1;
        }   
        BATR_comprobar_estadIsticas(puntero[2051],puntero[2052],puntero[2053]);
        
        //* --> Memoria Compartida *//
        if(puntero!=NULL){
            if(shmdt(puntero)==-1){
            perror("\nError en la liberación del puntero de la memoria compartida\n");
            exit(-1);
            }
        puntero=NULL;
        }
        if(idMemoria!=-1){
            if(shmctl(idMemoria, IPC_RMID, 0)==-1){
                perror("\nError en la eliminación de la memoria compartida\n");
                exit(-1);
            }

            idMemoria=-1;
        }
        BATR_fin();
    
        fprintf(stdout, "\nFIN DEL PROGRAMA\n");
        //:::::: MUERTE PROCESOS :::::://
        kill(getpid(), SIGKILL);
    }
    else
    exit(0);
}
