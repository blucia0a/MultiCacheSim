#include "CacheInterface.h"
#include "SMPCache.h"

#include "MESI_SMPCache.h"
#include "MSI_SMPCache.h"


#ifndef PIN
#include <pthread.h>
#else
#include "pin.H"
#endif
  
class MultiCacheSim : public CacheInterface{

public:


  //FIELDS
  //Number of Caches in the multicachesim
  int num_caches;

  //The vector that contains the caches
  std::vector<SMPCache * > allCaches;

  //The lock that protects the vector so it isn't corrupted by concurrent updates
  #ifndef PIN
  pthread_mutex_t allCachesLock;
  #else
  PIN_LOCK allCachesLock;
  #endif

  //Cache Parameters
  int cache_size;
  int cache_assoc;
  int cache_bsize;

  //The output file to dump stats to at the end
  FILE* CacheStats;

  CacheFactory cacheFactory;
  //METHODS
  //Constructor
  //MultiCacheSim(FILE *cachestats,int size, int assoc, int bsize);
  MultiCacheSim(FILE *cachestats,int size, int assoc, int bsize, CacheFactory c);

  //Adds a cache to the multicachesim
  void createNewCache();
 
  //These three functions implement the CacheInterface interface 
  void readLine(unsigned long tid, unsigned long rdPC, unsigned long addr);
  void writeLine(unsigned long tid, unsigned long wrPC, unsigned long addr);
  void dumpStatsForAllCaches();

  //Utility Function to get the cache object that has the specified CPUid
  SMPCache *findCacheByCPUId(int CPUid);

  //Translate from program threadID to multicachesim CPUId
  int tidToCPUId(int tid);

  char *Identify();

  //Destructor
  ~MultiCacheSim();

};
