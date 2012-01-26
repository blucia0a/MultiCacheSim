#include "MSI_SMPCache.h"

MSI_SMPCache::MSI_SMPCache(int cpuid, 
                           std::vector<SMPCache * > * cacheVector,
                           int csize, 
                           int cassoc, 
                           int cbsize, 
                           int caddressable, 
                           const char * repPol, 
                           bool cskew) : 
                             SMPCache(cpuid,cacheVector){
  
  fprintf(stderr,"Making a MSI cache with cpuid %d\n",cpuid);
  CacheGeneric<MSI_SMPCacheState> *c = 
    CacheGeneric<MSI_SMPCacheState>::create(csize, 
                                            cassoc, 
                                            cbsize, 
                                            caddressable, 
                                            repPol, 
                                            cskew);
  cache = (CacheGeneric<StateGeneric<> >*)c; 

}

void MSI_SMPCache::fillLine(uint32_t addr, uint32_t msi_state){

  //this gets the state of whatever line this address maps to 
  MSI_SMPCacheState *st = (MSI_SMPCacheState *)cache->findLine2Replace(addr); 

  if(st==0){
    /*No state*/
    return;
  }

  /*Set the tags to the tags for the newly cached block*/
  st->setTag(cache->calcTag(addr));

  /*Set the state of the block to the msi_state passed in*/
  st->changeStateTo((MSIState_t)msi_state);
  return;
    
}
  

MSI_SMPCache::RemoteReadService MSI_SMPCache::readRemoteAction(uint32_t addr){

  /*This method implements snoop behavior on all the other 
   *caches that this cache might be interacting with*/

  /*Loop over the other caches in the simulation*/
  std::vector<SMPCache * >::iterator cacheIter;
  std::vector<SMPCache * >::iterator lastCacheIter;
  for(cacheIter = this->getCacheVector()->begin(), 
      lastCacheIter = this->getCacheVector()->end(); 
      cacheIter != lastCacheIter; 
      cacheIter++){

    /*Get a pointer to the other cache*/
    MSI_SMPCache *otherCache = (MSI_SMPCache*)*cacheIter; 
    if(otherCache->getCPUId() == this->getCPUId()){

      /*We don't want to snoop our own access*/
      continue;

    }

    /*Get the state of the block this addr maps to in the other cache*/      
    MSI_SMPCacheState* otherState = 
      (MSI_SMPCacheState *)otherCache->cache->findLine(addr);

    /*If otherState == NULL here, the tags didn't match, so the
     *other cache didn't have this line cached*/
    if(otherState){
      /*The tags matched -- need to do snoop actions*/

      /*Other cache has recently written the line*/
      if(otherState->getState() == MSI_MODIFIED){
    
        /*Modified transitions to Shared on a remote Read*/ 
        otherState->changeStateTo(MSI_SHARED);

        /*Return a Remote Read Service indicating that 
         *1)The line was not shared (the false param)
         *2)The line was provided by otherCache, as only it had it cached
        */
        return MSI_SMPCache::RemoteReadService(false,true);

      /*Other cache has recently read the line*/
      }else if(otherState->getState() == MSI_SHARED){  
        
        /*Return a Remote Read Service indicating that 
         *1)The line was shared (the true param)
         *2)The line was provided by otherCache 
        */
        return MSI_SMPCache::RemoteReadService(true,true);

      /*Line was cached, but invalid*/
      }else if(otherState->getState() == MSI_INVALID){ 

        /*Do Nothing*/

      }

    }/*Else: Tag didn't match. Nothing to do for this cache*/

  }/*Done with other caches*/

  /*If all other caches were MSI_INVALID*/
  return MSI_SMPCache::RemoteReadService(false,false);
}


void MSI_SMPCache::readLine(uint32_t rdPC, uint32_t addr){
  /*
   *This method implements actions taken on a read access to address addr
   *at instruction rdPC
  */

  /*Get the state of the line to which this address maps*/
  MSI_SMPCacheState *st = 
    (MSI_SMPCacheState *)cache->findLine(addr);    
  
  /*Read Miss - tags didn't match, or line is invalid*/
  if(!st || (st && !(st->isValid())) ){

    /*Update event counter for read misses*/
    numReadMisses++;

    if(st){

      /*Tag matched, but state was invalid*/
      numReadOnInvalidMisses++;

    }

    /*Make the other caches snoop this access 
     *and get a remote read service object describing what happened.
     *This is effectively putting the access on the bus.
    */
    MSI_SMPCache::RemoteReadService rrs = readRemoteAction(addr);
    numReadRequestsSent++;
    
    if(rrs.providedData){

      /*If it was shared or modified elsewhere,
       *the line was provided by another cache.
       *Update these counters to reflect that
      */
      numReadMissesServicedByOthers++;

      if(rrs.isShared){
        numReadMissesServicedByShared++;
      }else{
        numReadMissesServicedByModified++;
      }

    } 

    /*Fill the line*/
    fillLine(addr,MSI_SHARED); 
      
  }else{

    /*Read Hit - any state but Invalid*/
    numReadHits++; 
    return; 

  }

}


MSI_SMPCache::InvalidateReply MSI_SMPCache::writeRemoteAction(uint32_t addr){
    
    /*This method implements snoop behavior on all the other 
     *caches that this cache might be interacting with*/
    
    bool empty = true;

    /*Loop over all other caches*/
    std::vector<SMPCache * >::iterator cacheIter;
    std::vector<SMPCache * >::iterator lastCacheIter;
    for(cacheIter = this->getCacheVector()->begin(), 
        lastCacheIter = this->getCacheVector()->end(); 
        cacheIter != lastCacheIter; 
        cacheIter++){


      
      MSI_SMPCache *otherCache = (MSI_SMPCache*)*cacheIter; 
      if(otherCache->getCPUId() == this->getCPUId()){
        /*We don't snoop ourselves*/
        continue;
      }

      /*Get the line from the current other cache*/
      MSI_SMPCacheState* otherState = 
        (MSI_SMPCacheState *)otherCache->cache->findLine(addr);

      /*if it is cached by otherCache*/
      if(otherState && otherState->isValid()){

          /*Invalidate the line, because we're writing*/
          otherState->invalidate();
 
          /*The reply contains data, so "empty" is false*/
          empty = false;

      }

    }/*done with other caches*/

    /*Empty=true indicates that no other cache 
    *had the line or there were no other caches
    * 
    *This data in this object is not used as is, 
    *but it might be useful if you plan to extend 
    *this simulator, so i left it in.
    */
    return MSI_SMPCache::InvalidateReply(empty);
}


void MSI_SMPCache::writeLine(uint32_t wrPC, uint32_t addr){
  /*This method implements actions taken when instruction wrPC
   *writes to memory location addr*/

  /*Find the line to which this address maps*/ 
  MSI_SMPCacheState * st = (MSI_SMPCacheState *)cache->findLine(addr);    
   
  /*
   *If the tags didn't match, or the line was invalid, it is a 
   *write miss
   */ 
  if(!st || (st && !(st->isValid())) ){ 

    numWriteMisses++;
    
    if(st){

      /*We're writing to an invalid line*/
      numWriteOnInvalidMisses++;

    }
 
    /*
     * Let the other caches snoop this write access and update their
     * state accordingly.  This action is effectively putting the write
     * on the bus.
     */ 
    MSI_SMPCache::InvalidateReply inv_ack = writeRemoteAction(addr);
    numInvalidatesSent++;

    /*Fill the line with the new written block*/
    fillLine(addr,MSI_MODIFIED);

    return;

  }else if(st->getState() == MSI_SHARED){
    /*If the block is shared and we're writing, we've incurred a coherence
     *miss.  We need to upgrade to Modified to write, and all other
     *copies must be invalidated
    */
    numWriteMisses++;

    /*Write-on-shared Coherence Misses*/
    numWriteOnSharedMisses++;

    /*Let the other sharers snoop this write, and invalidate themselves*/
    MSI_SMPCache::InvalidateReply inv_ack = writeRemoteAction(addr);
    numInvalidatesSent++;

    /*Change the state of the line to Modified to reflect the write*/
    st->changeStateTo(MSI_MODIFIED);
    return;

  }else{ //Write Hit

    /*Already have it writable: No coherence action required!*/
    numWriteHits++;

    return;

  }

}

char *MSI_SMPCache::Identify(){
  return (char *)"MSI Cache Coherence";
}

MSI_SMPCache::~MSI_SMPCache(){

}

extern "C" SMPCache *Create(int num, std::vector<SMPCache*> *cvec, int csize, int casso, int bs, int addrble, const char *repl, bool skw){

  return new MSI_SMPCache(num,cvec,csize,casso,bs,addrble,repl,skw);

}
