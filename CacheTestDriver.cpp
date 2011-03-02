#include "MultiCacheSim.h"
#include <stdlib.h>
#include <ctime>
#include <dlfcn.h>

#define NUM_CACHES 16 

std::vector<MultiCacheSim *>mcs;

void *concurrent_accesses(void*np){  
  unsigned long tid = *((unsigned long*)(np));
  for(int i = 0; i < 100000; i++){
    unsigned long addr = 1; 
    unsigned long pc = rand() % 0xdeadbeff + 0xdeadbeef; 
    unsigned long type = rand() % 2;
    if(type == 0){
      std::vector<MultiCacheSim *>::iterator i,e;
      for(i = mcs.begin(), e = mcs.end(); i != e; i++){ 
        (*i)->readLine(tid, pc, addr);
      }
    }else{
      std::vector<MultiCacheSim *>::iterator i,e;
      for(i = mcs.begin(), e = mcs.end(); i != e; i++){ 
        (*i)->writeLine(tid, pc, addr);
      }
    }
  }
}


int main(int argc, char** argv){
  srand(time(NULL));

  pthread_t tasks[NUM_CACHES];

  char *ct = strtok(argv[1],","); 
  while(ct != NULL){
    void *chand = dlopen( ct, RTLD_LAZY | RTLD_LOCAL );
    if( chand == NULL ){
      fprintf(stderr,"Couldn't Load %s\n", argv[1]);
      fprintf(stderr,"dlerror: %s\n", dlerror());
      exit(1);
    }
  
    CacheFactory cfac = (CacheFactory)dlsym(chand, "Create");
  
    if( chand == NULL ){
      fprintf(stderr,"Couldn't get the Create function\n");
      fprintf(stderr,"dlerror: %s\n", dlerror());
      exit(1);
    }
  
    MultiCacheSim *c = new MultiCacheSim(stdout, 32767, 8, 32, cfac);
    c->createNewCache();//CPU 1
    c->createNewCache();//CPU 2
    c->createNewCache();//CPU 3
    c->createNewCache();//CPU 4
    mcs.push_back(c);

    ct = strtok(NULL,",");
  }

  for(int i = 0; i < NUM_CACHES; i++){
    pthread_create(&(tasks[i]), NULL, concurrent_accesses, (void*)(new int(i)));
  }
  
  for(int i = 0; i < NUM_CACHES; i++){
    pthread_join(tasks[i], NULL);
  }

  std::vector<MultiCacheSim *>::iterator i,e;
  for(i = mcs.begin(), e = mcs.end(); i != e; i++){ 
    fprintf(stderr,"%s",(*i)->Identify());
    fprintf(stderr,"--------------------------------\n");
    (*i)->dumpStatsForAllCaches();
    fprintf(stderr,"********************************\n");
  }
}


