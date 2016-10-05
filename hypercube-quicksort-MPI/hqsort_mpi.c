#include <mpi.h>
#include <stdio.h>
#include <math.h>
#define MAXP 99
#define MAXD 99
#define MAX 99

/* Sequential Quicksort */
void quicksort(int list[],int left,int right) {
  int pivot,i,j;
  int temp;

  if (left < right) {
    i = left; j = right+1;
    pivot = list[left];
    do {
      while (list[++i] < pivot && i <= right);
      while (list[--j] > pivot);
      if (i < j) {
        temp = list[i]; list[i] = list[j]; list[j] = temp;
      }
    } while (i<j);
    temp = list[left]; list[left] = list[j]; list[j] = temp;
    quicksort(list,left,j-1);
    quicksort(list,j+1,right);
  }
}

int main(int argc, char *argv[]) {
  int nprocs; /* Number of processors */
  int myid;   /* My rank */
  int list[99],procs_cube[99];
  int n = 4;
  int L,i,j,p;
  int nsend,nrecv;

  MPI_Status status;
  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  srand((unsigned) myid+1);
  for (i=0; i<n; i++) list[i] = rand()%MAX;

  /* print list before */
  printf("Before: Node %d :",myid);
  for (i=0; i<n; i++) {
    printf(" ");
    printf("%d",list[i]);
  }
  printf("\n");

  int bitvalue = nprocs >> 1;
  int mask = nprocs - 1;
  int dim = log2(nprocs);
  int pivot = 0;
  int nprocs_cube;
  int c,partner,temp;

  MPI_Comm cube[MAXD][MAXP];
  MPI_Group cube_group[MAXD][MAXP];
  MPI_Comm_group(MPI_COMM_WORLD, &cube_group[dim][0]);
  cube[dim][0] = MPI_COMM_WORLD;

  for (L=dim; L>=1; L--) {
    if ((myid & mask) == 0) {
      for (i =0, pivot=0; i<n; i++) pivot += list[i];
      if (n>0) pivot /=n;
    }

    nprocs_cube = pow(2,L);
    c = myid/nprocs_cube;

    MPI_Bcast(&pivot,1,MPI_INT,0,cube[L][c]);

    /* partition list */
    if (n>0) {
      i = -1;
      j = n;
      do {
        while (list[++i] <= pivot && i<n);
        while (list[--j] > pivot && j > -1);
        if (i<j) {
          temp = list[i];
          list[i] = list[j];
          list[j] = temp;
        }
      } while (i<j);
    } else j = -1;

    /* Exchange sublists with partner */
    partner = myid ^ bitvalue;
    if ((myid & bitvalue) == 0)
    {
      nsend = n - (j + 1); /* length of right sublist */
      MPI_Send(&nsend, 1, MPI_INT, partner, 10, MPI_COMM_WORLD);
      MPI_Recv(&nrecv, 1, MPI_INT, partner, 10, MPI_COMM_WORLD, &status);
      n = n - nsend + nrecv;
      MPI_Send(&list[j+1], nsend, MPI_INT, partner, 20, MPI_COMM_WORLD);
      MPI_Recv(&list[j+1], nrecv, MPI_INT, partner, 20, MPI_COMM_WORLD, &status);
    }
    else
    {
      nsend = (j + 1); /* length of left sublist */
      MPI_Send(&nsend, 1, MPI_INT, partner, 10, MPI_COMM_WORLD);
      MPI_Recv(&nrecv, 1, MPI_INT, partner, 10, MPI_COMM_WORLD, &status);
      MPI_Send(&list[0], nsend, MPI_INT, partner, 20, MPI_COMM_WORLD);
      /* shift right sublist to the left */
      for (i = 0 ; i < n-nsend ; i++)
      {
        list[i] = list[j+1+i];
      }
      MPI_Recv(&list[n-nsend], nrecv, MPI_INT, partner, 20, MPI_COMM_WORLD, &status);
      n = n - nsend + nrecv;
    }

    /* Communicators */
    MPI_Comm_group(cube[L][c],&(cube_group[L][c]));
    nprocs_cube = nprocs_cube/2;
    for(p=0; p<nprocs_cube; p++) procs_cube[p] = p;
    MPI_Group_incl(cube_group[L][c],nprocs_cube,procs_cube,&(cube_group[L-1][2*c  ]));
    MPI_Group_excl(cube_group[L][c],nprocs_cube,procs_cube,&(cube_group[L-1][2*c+1]));
    MPI_Comm_create(cube[L][c],cube_group[L-1][2*c  ],&(cube[L-1][2*c ]));
    MPI_Comm_create(cube[L][c],cube_group[L-1][2*c+1],&(cube[L-1][2*c+1]));
    MPI_Group_free(&(cube_group[L  ][c   ]));
    MPI_Group_free(&(cube_group[L-1][2*c ]));
    MPI_Group_free(&(cube_group[L-1][2*c+1]));

    mask = mask ^ bitvalue;
    bitvalue = bitvalue >> 1;
  } /* endfor */

  /* run quicksort */
  quicksort(list,0,n-1);

  /* print list after */
  printf("After: Node %d :",myid);
  for (i=0; i<n; i++) {
    printf(" %d",list[i]);
  }
  printf("\n");

  MPI_Finalize();
  return 0;
}
