#define DERECHA 0
#define IZQUIERDA 1
#define ARRIBA 2

struct posiciOn {int x,y;};

int BATR_pausa(void);
int BATR_pausita(void);

int BATR_inicio(int ret,int semAforos, int lTroncos[],int lAguas[],int dirs[],
                int tCriar,char *zona);
int BATR_avance_troncos(int fila);

void BATR_descansar_criar(void);
int BATR_parto_ranas(int i,int *dx,int *dy);

int BATR_puedo_saltar(int x,int y,int direcciOn);
int BATR_avance_rana_ini(int x,int y);
int BATR_avance_rana(int *x,int *y,int direcciOn);
int BATR_avance_rana_fin(int x,int y);

int BATR_comprobar_estadIsticas(int r_nacidas, int r_salvadas, int r_perdidas);
int BATR_fin(void);
