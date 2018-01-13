// BFSTEST : Test breadth-first search in a graph.
//
// example: cat sample.txt | ./bfstest 1
//
// John R. Gilbert, 17 Feb 20ll

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define NUM_THREADS 8

typedef struct graphstruct { // A graph in compressed-adjacency-list (CSR) form
  int nv;            // number of vertices
  int ne;            // number of edges
  int *nbr;          // array of neighbors of all vertices
  int *firstnbr;     // index in nbr[] of first neighbor of each vtx
} graph;


/* global state */
struct timespec  start_time;
struct timespec  end_time;


typedef struct graph_args_struct {
  int id;
  int ncolumn;
  int nedges;
  int *tail;
  int *head;
  int *Graph;
  int max;
} graph_args;

typedef struct bfs_args_struct
{
  int sv;  
  int nv;
  int nc;
  int *level;
  int *levelsize;
  int thislevel;
  int *Graph;
  int *parent;
} bfs_args;  
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t * mutexes;



unsigned int seed = 0x12345678;
unsigned int myrand(unsigned int *seed, unsigned int input) {
  *seed = (*seed << 13) ^ (*seed >> 15) + input + 0xa174de3;
  return *seed;
};

void sig_check(int *level, int nv) {
  int i;
  unsigned int sig = 0x123456;

  for(i = 0; i < nv; i++)
  {
    myrand(&sig, level[i]);
  }

  printf("Computed check sum signature:0x%08x\n", sig);
  if(sig == 0x18169857)
    printf("Result check of sample.txt by signature successful!!\n");
  else if(sig == 0xef872cf0)
    printf("Result check of TEST1 by signature successful!!\n");
  else if(sig == 0x29c12a44)
    printf("Result check of TEST2 by signature successful!!\n");
  else if(sig == 0xe61d1d00)
    printf("Result check of TEST3 by signature successful!!\n");
  else
    printf("Result check by signature failed!!\n");
}


/* Read input from stdio (for genx.pl files, no more than 40 seconds) */
int read_edge_list (int **tailp, int **headp) {
  int max_edges = 100000000;
  int nedges, nr, t, h;

  *tailp = (int *) calloc(max_edges, sizeof(int));
  *headp = (int *) calloc(max_edges, sizeof(int));
  nedges = 0;
  nr = scanf("%i %i",&t,&h);
  while (nr == 2) {
    if (nedges >= max_edges) {
      printf("Limit of %d edges exceeded.\n",max_edges);
      exit(1);
    }
    (*tailp)[nedges] = t;
    (*headp)[nedges++] = h;
    nr = scanf("%i %i",&t,&h);
  }
  return nedges;
}


void print_CSR_graph (graph *G) {
  int vlimit = 20;
  int elimit = 50;
  int e,v;
  printf("\nGraph has %d vertices and %d edges.\n",G->nv,G->ne);
  printf("firstnbr =");
  if (G->nv < vlimit) vlimit = G->nv;
  for (v = 0; v <= vlimit; ++v) printf(" %d",G->firstnbr[v]);
  if (G->nv > vlimit) printf(" ...");
  printf("\n");
  printf("nbr =");
  if (G->ne < elimit) elimit = G->ne;
  for (e = 0; e < elimit; ++e) printf(" %d",G->nbr[e]);
  if (G->ne > elimit) printf(" ...");
  printf("\n\n");
}





/* Modify the next two functions */


//--------------thread jobs-----------------
//global arg: NUM_THREADS
//arg: id, Graph, nedges, nc, tail, head
void * thread_job_build_graph(void * arg)
{
  graph_args * p = (graph_args * ) arg;
  int index, getpos, i;
  int from = p->nedges * p->id / NUM_THREADS;
  int to = p->nedges * (p->id + 1) / NUM_THREADS;
  for (i = from; i < to; ++i) {
    index = p->ncolumn * p->tail[i];
    ++p->Graph[index];
    getpos = index + p->Graph[index];
    p->Graph[getpos] = p->head[i];
  }
}

//global arg: NUM_THREADS
//arg: id, nedges, tail, head, max,Graph
void * thread_job_find_max_number(void * arg)
{
  graph_args * p = (graph_args * ) arg;  
  int i, index;
  int from = p->nedges * p->id / NUM_THREADS;
  int to = p->nedges * (p->id + 1) / NUM_THREADS; 
  for (i = from; i < to; ++i) {
    ++p->Graph[p->head[i]];
    if (p->Graph[ p->head[i] ] > p->max) p->max = p->Graph[ p->head[i] ];
  }
}

//arg: nv, level, Graph, parent,  sv, nc, levelsize, thislevel 
//build its own queue
void * thread_job_bfs(void* arg){
  bfs_args * p = (bfs_args *) arg;
  int * queue;
  int front = 0,back = 0;
  int index;
  int v, w, i, e;
  int thislevelQ;

  queue = (int *) malloc(sizeof(int) * p->nv);
  queue[back++]=p->sv;
  thislevelQ=back;
  while (p->levelsize[p->thislevel] > 0) {
    while(front != thislevelQ) {
      v = queue[front++];       // v is the current vertex to explore from
      int index = v * p->nc;
      for (e = 1; e <= p->Graph[index]; ++e) {
        w = p->Graph[index + e];          // w is the current neighbor of v
        if (p->level[w] == -1 || p->level[w] > p->thislevel + 1) {   // w has not already been reached
          p->level[w] = p->thislevel+1;
          p->parent[w] = v;
          ++p->levelsize[p->thislevel+1];
          queue[back++] = w;    // put w on queue to explore
        }
      }
    }
    thislevelQ = back;
    p->thislevel = p->thislevel+1;
  }
  free(queue);
}
//--------------thread jobs-----------------



// use a array K[nedges * ColNum] to simulate K[nedges][ColNum]
// put the amount of each node's tail in K[i][0] -> K[i * ColNum]
int * graph_from_edge_list (int *tail, int* head, int nedges, int * nv, int *nc) {
  int i, e, v, maxv, index, getpos;
  int *C, *K;
  int max = 0;
  int rc;
  void * status;

  //build arguments for threads
  graph_args * arg;
  arg = (graph_args *) malloc(sizeof(graph_args) * NUM_THREADS );

  
  
  //create pthreads
  pthread_t* thread_array = NULL;
  thread_array = malloc(sizeof(pthread_t) * NUM_THREADS);
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
 
  
  //find the max number of nodes , store it in *nv    
  for (i = 0; i < nedges; ++i) {
    if (head[i] > max) max = head[i];
    if (tail[i] > max) max = tail[i];
  }
  *nv = max + 1;
  
  //find the max number of neighbors a node could have by threads, store it in *nc
  max = 0;  
  C = (int*) calloc(*nv,sizeof(int));
  for(i = 0; i < NUM_THREADS; ++i) {
    arg[i].id = i;
    arg[i].nedges = nedges;
    arg[i].tail = tail;
    arg[i].head = head;
    arg[i].Graph = C;
    arg[i].max = 0;
  }
  for(i = 0; i < NUM_THREADS; ++i) 
    rc = pthread_create(thread_array + i, &attr, thread_job_find_max_number, arg + i);
  for(i = 0; i < NUM_THREADS; ++i)
    pthread_join(thread_array[i], &status);
  free(C);
  for (i = 0; i <NUM_THREADS; ++i) {
    if (arg[i].max > max) max = arg[i].max;
  }
  *nc = max + 1;


  // build a array with (*nv) * (*nc) elements and build a graph by graphs
  K = (int*) calloc(*nv * *nc,sizeof(int));
  for (i = 0; i < NUM_THREADS; ++i) {
    arg[i].Graph = K;
    arg[i].ncolumn = *nc;
  }
  for(i = 0; i < NUM_THREADS; ++i) {
    rc = pthread_create(thread_array + i, &attr, thread_job_build_graph, arg + i);
  }
  for(i = 0; i < NUM_THREADS; ++i)
    pthread_join(thread_array[i], &status);
  pthread_attr_destroy(&attr);
  return K;
}


void bfs (int s, int *G, int **levelp, int *nlevelsp, 
         int **levelsizep, int **parentp, int nv, int nc) {
  int *level, *levelsize, *parent;
  int thislevel, nthreads;
  int *queue, back, front;
  int i, v, w, e;
  int max = 0;

  bfs_args * arg;
  int rc;
  void * status;
  level = *levelp = (int *) calloc(nv, sizeof(int));
  levelsize = *levelsizep = (int *) calloc(nv, sizeof(int));
  parent = *parentp = (int *) calloc(nv, sizeof(int));
  queue = (int *) calloc(nv, sizeof(int));

  back = 0;   // position next element will be added to queue
  front = 0;  // position next element will be removed from queue
  for (v = 0; v < nv; ++v) level[v] = -1;
  for (v = 0; v < nv; ++v) parent[v] = -1;



// assign the starting vertex level 0 and put it on the queue to explore
  thislevel = 0;
  level[s] = 0;
  levelsize[0] = 1;
  queue[back++] = s;

  // loop over levels, then over vertices at this level, then over neighbors
  while (levelsize[thislevel] > 0) {
    levelsize[thislevel+1] = 0;
    for (i = 0; i < levelsize[thislevel]; ++i) {
      v = queue[front++];       // v is the current vertex to explore from
      int index = v * nc;
      for (e = 1; e <= G[index]; ++e) {
        w = G[index + e];          // w is the current neighbor of v
        if (level[w] == -1) {   // w has not already been reached
          parent[w] = v;
          level[w] = thislevel+1;
          ++levelsize[thislevel+1];
          queue[back++] = w;    // put w on queue to explore
        }
      }
    }
    thislevel = thislevel+1;
    if (levelsize[thislevel] > 3) {
      nthreads = levelsize[thislevel];
      break;
    }
  }

  //create pthreads
  pthread_t* thread_array = NULL;
  thread_array = malloc(sizeof(pthread_t) * nthreads);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  //build arguments for threads
  arg = (bfs_args *) malloc(sizeof(bfs_args) * nthreads );
  //arg: nv, level, Graph, parent,  sv, nc, levelsize, thislevel 
  for (i = 0; i < nthreads; ++i) {
    arg[i].nv = nv;
    arg[i].level = level;
    arg[i].Graph = G;
    arg[i].parent = parent;
    arg[i].nc = nc;
    arg[i].levelsize = levelsize;
    arg[i].thislevel = thislevel;
    arg[i].sv = queue[front++];
  }

  //distribute nodes in level 1 to each threads
  for(i = 0; i < nthreads; ++i) {
    rc = pthread_create(thread_array + i, &attr, thread_job_bfs, arg + i);
  }
  for(i = 0; i < nthreads; ++i)
    pthread_join(thread_array[i], &status);

  pthread_attr_destroy(&attr);

  for (i = 0; i < nthreads; ++i) {
    if (arg[i].thislevel > max) max = arg[i].thislevel;
  }
  
  *nlevelsp = max;

  free(queue);
}


int main (int argc, char* argv[]) {
  int *G;
  int *level, *levelsize, *parent;
  int *tail, *head;
  int nedges;
  int nlevels;
  int startvtx;
  int i, v, reached;
  int nv, nc;
  if (argc == 2) {
    startvtx = atoi (argv[1]);
  } else {
    printf("usage:   bfstest <startvtx> < <edgelistfile>\n");
    printf("example: cat sample.txt | ./bfstest 1\n");
    exit(1);
  }
  nedges = read_edge_list (&tail, &head);
  clock_gettime(CLOCK_REALTIME, &start_time); //stdio scanf ended, timer starts  //Don't remove it

  /* You can modify the function below */
  G = graph_from_edge_list (tail, head, nedges, &nv, &nc);
  free(tail);
  free(head);
  bfs (startvtx, G, &level, &nlevels, &levelsize, &parent, nv, nc);
  /* You can modify the function above */
  clock_gettime(CLOCK_REALTIME, &end_time);  //graph construction and bfs completed timer ends  //Don't remove it

  //print_CSR_graph (G);
  printf("Starting vertex for BFS is %d.\n\n",startvtx);

  reached = 0;
  for (i = 0; i < nlevels; i++) reached += levelsize[i];
  printf("Breadth-first search from vertex %d reached %d levels and %d vertices.\n",
    startvtx, nlevels, reached);
  for (i = 0; i < nlevels; i++) printf("level %d vertices: %d\n", i, levelsize[i]);
  if (nv < 20) {
    printf("\n  vertex parent  level\n");
    for (v = 0; v < nv; v++) printf("%6d%7d%7d\n", v, parent[v], level[v]);
  }
  printf("\n");

//Don't remove it
  printf("s_time.tv_sec:%ld, s_time.tv_nsec:%09ld\n", start_time.tv_sec, start_time.tv_nsec);
  printf("e_time.tv_sec:%ld, e_time.tv_nsec:%09ld\n", end_time.tv_sec, end_time.tv_nsec);
  if(end_time.tv_nsec > start_time.tv_nsec)
  {
    printf("[diff_time:%ld.%09ld sec]\n",
    end_time.tv_sec - start_time.tv_sec,
    end_time.tv_nsec - start_time.tv_nsec);
  }
  else
  {
    printf("[diff_time:%ld.%09ld sec]\n",
    end_time.tv_sec - start_time.tv_sec - 1,
    end_time.tv_nsec - start_time.tv_nsec + 1000*1000*1000);
  }
  sig_check(level, nv);
}




