#include "shuffle_fix.h"
#include "../config.h"
#include <Arduino.h>

static PlaylistState pls = {0};
static bool pls_initialized = false;

void omnia_shuffle_init(uint16_t total){
  if(total==0) total=1;
  if(total>500) total=500; // safety limit
  if(pls.shuffledOrder){
    free(pls.shuffledOrder);
    pls.shuffledOrder = nullptr;
  }
  pls.total = total;
  pls.currentIdx = 1;
  pls.repeat = REPEAT_ALL;
  pls.shuffledPos = 0;
  pls.shuffledOrder = (uint16_t*)malloc(total*sizeof(uint16_t));
  if(!pls.shuffledOrder){
    Serial.println("##SHUFFLE#: malloc failed!");
    pls.total=0;
    return;
  }
  for(uint16_t i=0;i<total;i++) pls.shuffledOrder[i]=i+1;
  pls_initialized = true;
}

void omnia_shuffle_set(ShuffleMode s){
  if(!pls_initialized || !pls.shuffledOrder){
    omnia_shuffle_init(config.playlistLength());
  }
  pls.shuffle = s;
  if(s==SHUFFLE_ON && pls.shuffledOrder){
    for(uint16_t i=pls.total-1;i>0;i--){
      uint16_t j = random(i+1); // 0..i
      uint16_t tmp = pls.shuffledOrder[i];
      pls.shuffledOrder[i]=pls.shuffledOrder[j];
      pls.shuffledOrder[j]=tmp;
    }
    pls.shuffledPos=0;
    if(pls.currentIdx>0 && pls.currentIdx<=pls.total){
      for(uint16_t i=0;i<pls.total;i++){
        if(pls.shuffledOrder[i]==pls.currentIdx){
          uint16_t tmp = pls.shuffledOrder[i];
          pls.shuffledOrder[i]=pls.shuffledOrder[0];
          pls.shuffledOrder[0]=tmp;
          break;
        }
      }
      pls.shuffledPos=1; // next is second
    }
  }
}

void omnia_shuffle_set_repeat(RepeatMode r){ pls.repeat=r; }

uint16_t omnia_shuffle_next(){
  uint16_t curTotal = config.playlistLength();
  if(curTotal==0) curTotal=1;
  if(!pls_initialized || !pls.shuffledOrder || curTotal != pls.total){
    omnia_shuffle_init(curTotal);
    // keep shuffle mode if was ON
    if(pls.shuffle==SHUFFLE_ON) {
      // already ON, re-shuffle will happen in set
    }
  }
  if(pls.total==0 || !pls.shuffledOrder) return 1;
  if(pls.shuffle==SHUFFLE_OFF){
    uint16_t next = pls.currentIdx + 1;
    if(next > pls.total){
      if(pls.repeat==REPEAT_ALL) next=1;
      else if(pls.repeat==REPEAT_ONE) next=pls.currentIdx;
      else next=0;
    }
    return next;
  }else{
    if(pls.shuffledPos < pls.total){
      return pls.shuffledOrder[pls.shuffledPos++];
    }else{
      if(pls.repeat==REPEAT_ALL){
        omnia_shuffle_set(SHUFFLE_ON);
        if(pls.shuffledPos < pls.total) return pls.shuffledOrder[pls.shuffledPos++];
        else return 1;
      }
      if(pls.repeat==REPEAT_ONE) return pls.currentIdx;
      return 0;
    }
  }
}

uint16_t omnia_shuffle_prev(){
  uint16_t curTotal = config.playlistLength();
  if(curTotal==0) curTotal=1;
  if(!pls_initialized || !pls.shuffledOrder || curTotal != pls.total){
    omnia_shuffle_init(curTotal);
  }
  if(pls.total==0 || !pls.shuffledOrder) return 1;
  if(pls.shuffle==SHUFFLE_OFF){
    if(pls.currentIdx > 1) return pls.currentIdx-1;
    if(pls.repeat==REPEAT_ALL) return pls.total;
    return pls.currentIdx;
  }else{
    if(pls.shuffledPos>1){
      pls.shuffledPos-=2;
      return pls.shuffledOrder[pls.shuffledPos++];
    }
    return pls.currentIdx;
  }
}

void omnia_shuffle_on_track_change(uint16_t newIdx){
  pls.currentIdx=newIdx;
  if(newIdx>pls.total) pls.currentIdx=pls.total;
}
