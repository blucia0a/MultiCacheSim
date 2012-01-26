#include "MultiCacheSim.h"

MultiCacheSim::MultiCacheSim(FILE *cachestats, int size, int assoc, int bsize, CacheFactory c){
  cacheFactory = c;
  CacheStats = cachestats;
  num_caches = 0;
  cache_size = size;
  cache_assoc = assoc;
  cache_bsize = bsize; 

  #ifndef PIN
  pthread_mutex_init(&allCachesLock, NULL);
  #else
  InitLock(&allCachesLock);
  #endif
}

SMPCache *MultiCacheSim::findCacheByCPUId(int CPUid){
    std::vector<SMPCache *>::iterator cacheIter = allCaches.begin();
    std::vector<SMPCache *>::iterator cacheEndIter = allCaches.end();
    for(; cacheIter != cacheEndIter; cacheIter++){
      if((*cacheIter)->CPUId == CPUid){
        return (*cacheIter);
      }
    }
    return NULL;
} 
  
void MultiCacheSim::dumpStatsForAllCaches(bool concise){
   
    std::vector<SMPCache *>::iterator cacheIter = allCaches.begin();
    std::vector<SMPCache *>::iterator cacheEndIter = allCaches.end();
    for(; cacheIter != cacheEndIter; cacheIter++){
      if(!concise){
        (*cacheIter)->dumpStatsToFile(CacheStats);
      }else{

    fprintf(CacheStats,"CPUId, numReadHits, numReadMisses, numReadOnInvalidMisses, numReadRequestsSent, numReadMissesServicedByOthers, numReadMissesServicedByShared, numReadMissesServicedByModified, numWriteHits, numWriteMisses, numWriteOnSharedMisses, numWriteOnInvalidMisses, numInvalidatesSent\n");

        (*cacheIter)->conciseDumpStatsToFile(CacheStats);
      }
    }
}

void MultiCacheSim::createNewCache(){
    #ifndef PIN
    pthread_mutex_lock(&allCachesLock);
    #else
    GetLock(&allCachesLock,1); 
    #endif


    SMPCache * newcache;
    newcache = this->cacheFactory(num_caches++, &allCaches, cache_size, cache_assoc, cache_bsize, 1, "LRU", false);
    allCaches.push_back(newcache);


    #ifndef PIN
    pthread_mutex_unlock(&allCachesLock);
    #else
    ReleaseLock(&allCachesLock); 
    #endif
}

void MultiCacheSim::readLine(unsigned long tid, unsigned long rdPC, unsigned long addr){
    #ifndef PIN
    pthread_mutex_lock(&allCachesLock);
    #else
    GetLock(&allCachesLock,1); 
    #endif


    SMPCache * cacheToRead = findCacheByCPUId(tidToCPUId(tid));
    if(!cacheToRead){
      return;
    }
    cacheToRead->readLine(rdPC,addr);


    #ifndef PIN
    pthread_mutex_unlock(&allCachesLock);
    #else
    ReleaseLock(&allCachesLock); 
    #endif
    return;
}
  
void MultiCacheSim::writeLine(unsigned long tid, unsigned long wrPC, unsigned long addr){
    #ifndef PIN
    pthread_mutex_lock(&allCachesLock);
    #else
    GetLock(&allCachesLock,1); 
    #endif


    SMPCache * cacheToWrite = findCacheByCPUId(tidToCPUId(tid));
    if(!cacheToWrite){
      return;
    }
    cacheToWrite->writeLine(wrPC,addr);


    #ifndef PIN
    pthread_mutex_unlock(&allCachesLock);
    #else
    ReleaseLock(&allCachesLock); 
    #endif
    return;
}

int MultiCacheSim::getStateAsInt(unsigned long tid, unsigned long addr){

  SMPCache * cacheToWrite = findCacheByCPUId(tidToCPUId(tid));
  if(!cacheToWrite){
    return -1;
  }
  cacheToWrite->getStateAsInt(addr);

}

int MultiCacheSim::tidToCPUId(int tid){
    //simple for now, perhaps we want to be fancier
    return tid % num_caches; 
}

char *MultiCacheSim::Identify(){
  SMPCache *c = findCacheByCPUId(0);
  if(c != NULL){
    return c->Identify();
  }
}
  
MultiCacheSim::~MultiCacheSim(){
    std::vector<SMPCache *>::iterator cacheIter = allCaches.begin();
    std::vector<SMPCache *>::iterator cacheEndIter = allCaches.end();
    for(; cacheIter != cacheEndIter; cacheIter++){
      delete (*cacheIter);
    }
}
