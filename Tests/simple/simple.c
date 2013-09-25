#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#define MAXVAL 100

struct wonk{
  int a;
} *shrdPtr;

pthread_mutex_t lock;

struct wonk *getNewVal(struct wonk**old){
  free(*old);
  *old = NULL;
  struct wonk *newval = (struct wonk*)malloc(sizeof(struct wonk));
  newval->a = 1;
  return newval;
}

void *updaterThread(void *arg){

  int i;
  for(i = 0; i < 10; i++){    
    pthread_mutex_lock(&lock);
    struct wonk *newval = getNewVal(&shrdPtr);
    shrdPtr = newval;
    pthread_mutex_unlock(&lock);
  }

}

void swizzle(int *result){
    
    pthread_mutex_lock(&lock);//400a4e
    if(shrdPtr != NULL){
     

      *result += shrdPtr->a;     
  
    }
    pthread_mutex_unlock(&lock);

}

void *accessorThread(void *arg){

  int *result = (int*)malloc(sizeof(int));; 
  *result = 0;

  while(*result < MAXVAL){ 
    swizzle(result);
    usleep(10 + (rand() % 100) );
  }
  
  pthread_exit(result); 
}

int main(int argc, char *argv[]){

  int res = 0;
  shrdPtr= (struct wonk*)malloc(sizeof(struct wonk));
  shrdPtr->a = 1;

  pthread_mutex_init(&lock,NULL);

  pthread_t acc[4],upd;
  pthread_create(&acc[0],NULL,accessorThread,(void*)shrdPtr);
  pthread_create(&acc[1],NULL,accessorThread,(void*)shrdPtr);
  pthread_create(&acc[2],NULL,accessorThread,(void*)shrdPtr);
  pthread_create(&acc[3],NULL,accessorThread,(void*)shrdPtr);
  pthread_create(&upd,NULL,updaterThread,(void*)shrdPtr);

  pthread_join(upd,NULL);
  pthread_join(acc[0],(void*)&res);
  pthread_join(acc[1],(void*)&res);
  pthread_join(acc[2],(void*)&res);
  pthread_join(acc[3],(void*)&res);
  fprintf(stderr,"Final value of res was %d\n",res); 
}
