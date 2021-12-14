#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define GRID 2048
#define GER  2000

int iniciar(int** array, int dx, int rank, int size, int tag) {
  int i,j;
  
  for(i = 0; i < dx; i++) {
    for(j = 0; j < GRID; j++) array[i][j] = 0;
    
    if(rank == 0) {
      array[1][2] = 1;
      array[2][3] = 1;
      array[3][1] = 1;
      array[3][2] = 1;
      array[3][3] = 1;    
      array[10][31] = 1;
      array[10][32] = 1;
      array[11][30] = 1;
      array[11][31] = 1;
      array[12][31] = 1;
    }
       
    if(size > 1) {
      if(i == 0) {
        if(rank > 0) MPI_Send(array[i], GRID, MPI_INT, (rank-1)%size, tag, MPI_COMM_WORLD);
        else MPI_Send(array[i], GRID, MPI_INT, size-1, tag, MPI_COMM_WORLD);
      }
      if(i == (dx-1)) MPI_Send(array[i], GRID, MPI_INT, (rank+1)%size, tag+GER, MPI_COMM_WORLD);
    }
    
  }
  
}

int contar(int** array, int dx) {
  int i, j, n;
  
  n = 0;
  for(i = 0; i < dx; i++) {
    for(j = 0; j < GRID; j++) if(array[i][j] == 1) n++;
  }
  
  return n;
}

int viz(int** array, int* va, int* vb, int x, int y, int dx, int rank, int size, int tag) {
  int n, antx, proxx, anty, proxy;
  
  n = 0;
  
  if(y-1 < 0) anty = GRID-1;
  else anty = y-1;
  
  if(y+1 > GRID-1) proxy = 0;
  else proxy = y+1;
  
  if(array[x][anty] == 1) n++;
  if(array[x][proxy] == 1) n++;
  
  if(size == 1) {
    if(x-1 < 0) antx = GRID-1;
    else antx = x-1;
  
    if(x+1 > GRID-1) proxx = 0;
    else proxx = x+1;
    
    if(array[antx][anty] == 1) n++;
    if(array[antx][y] == 1) n++;
    if(array[antx][proxy] == 1) n++;
    if(array[proxx][anty] == 1) n++;
    if(array[proxx][y] == 1) n++;
    if(array[proxx][proxy] == 1) n++;
  } else {
    if(x-1 < 0) {
      if(va[anty] == 1) n++;
      if(va[y] == 1) n++;
      if(va[proxy] == 1) n++;
    } else {
      if(array[x-1][anty] == 1) n++;
      if(array[x-1][y] == 1) n++;
      if(array[x-1][proxy] == 1) n++;
    }
  
    if(x+1 > dx-1) {
      if(vb[anty] == 1) n++;
      if(vb[y] == 1) n++;
      if(vb[proxy] == 1) n++;
    } else {
      if(array[x+1][anty] == 1) n++;
      if(array[x+1][y] == 1) n++;
      if(array[x+1][proxy] == 1) n++;
    }
  }
  
  return n;
}

int atualizar(int** array, int** narray, int* va, int* vb, int dx, int rank, int size, int tag) {
  int i, j, nviz;
  MPI_Status status;
  
  
  if(size > 1) {
    if(rank == 0) MPI_Recv(va, GRID, MPI_INT, size-1, tag+GER, MPI_COMM_WORLD, &status);
    else MPI_Recv(va, GRID, MPI_INT, (rank-1)%size, tag+GER, MPI_COMM_WORLD, &status);
    MPI_Recv(vb, GRID, MPI_INT, (rank+1)%size, tag, MPI_COMM_WORLD, &status);
  }
  
  for(i = 0; i < dx; i++) {
    for(j = 0; j < GRID; j++) {
      nviz = viz(array, va, vb, i, j, dx, rank, size, tag);
      if(array[i][j] == 1 && (nviz == 2 || nviz == 3)) narray[i][j] = 1;
      else if(array[i][j] == 0 && nviz == 3) narray[i][j] = 1;
      else narray[i][j] = 0;
    }
  }
  
  for(i = 0; i < dx; i++) {
    for(j = 0; j < GRID; j++) {
      array[i][j] = narray[i][j];
    }
    
    if(size > 1 && tag < GER) {
      if(i == 0) {
        if(rank > 0) MPI_Send(array[i], GRID, MPI_INT, (rank-1)%size, tag+1, MPI_COMM_WORLD);
        else MPI_Send(array[i], GRID, MPI_INT, size-1, tag+1, MPI_COMM_WORLD);
      }
      if(i == (dx-1)) MPI_Send(array[i], GRID, MPI_INT, (rank+1)%size, tag+1+GER, MPI_COMM_WORLD);
    }
    
  }
  
  return 0;
}

int main(int argc, char ** argv) {

  clock_t begin = clock();
  int i, j, size, rank, ini, fim, dx, soma, total;
       
  MPI_Init (&argc,&argv);
    
    MPI_Comm_size (MPI_COMM_WORLD,&size);
    MPI_Comm_rank (MPI_COMM_WORLD,&rank);
    
    ini = rank*GRID/size;
    fim = (rank+1)*GRID/size;
    dx = fim-ini;
    
    int *va = (int*) malloc(GRID*sizeof(int));
    int *vb = (int*) malloc(GRID*sizeof(int));
    int **array = (int**) malloc(dx*sizeof(int*));
    for(i = 0; i < dx; i++) {
      array[i] = (int*) malloc(GRID*sizeof(int));
    }
    int **narray = (int**) malloc(dx*sizeof(int*));
    for(i = 0; i < dx; i++) {
      narray[i] = (int*) malloc(GRID*sizeof(int));
    }
    
    for(i = 0; i <= GER; i++) {
      if(i == 0) iniciar(array, dx, rank, size, i+1);
      else atualizar(array, narray, va, vb, dx, rank, size, i);
      soma = contar(array, dx);
      MPI_Reduce(&soma, &total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
      if(rank == 0) {
        if(i == 0) printf("**Game of Life\nCondição inicial: %d\n", total);
        else printf("Geração %d: %d\n", i, total);
      }
    }
   
   clock_t end = clock();
   if(rank == 0) printf("Tempo de execução: %lf s\n", (double)(end - begin) / CLOCKS_PER_SEC);
   
   MPI_Finalize();
}
