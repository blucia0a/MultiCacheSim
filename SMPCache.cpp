#include "SMPCache.h"

SMPCache::SMPCache(int cpuid, std::vector<SMPCache * > * cacheVector){

  CPUId = cpuid;
  allCaches = cacheVector;
  numReadHits = numReadMisses = numReadMissesServicedByOthers = numWriteHits = numWriteMisses = numCoherenceMisses = 0;

}

void SMPCache::dumpStatsToFile(FILE* outFile){
  fprintf(outFile, "-----Cache %d-----\nReadHits:%d\nReadMisses:%d\nReadMissesServicedRemotely:%d\nWriteHits:%d\nCoherenceMisses:%d\nWriteMisses:%d\n",CPUId,numReadHits,numReadMisses,numReadMissesServicedByOthers,numWriteHits,numCoherenceMisses,numWriteMisses);
}

int SMPCache::getCPUId(){
  return CPUId;
}

std::vector<SMPCache * > *SMPCache::getCacheVector(){
  return allCaches;
}
