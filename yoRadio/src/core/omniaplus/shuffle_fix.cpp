#include "shuffle_fix.h"
#include "../config.h"
#include <Arduino.h>

static PlaylistState pls = {0};
static bool pls_initialized = false;

void omnia_shuffle_init(uint16_t total){
  if(total==0) total=1;
  if(total>500) total=500;
  if(pls.shuffledOrder){
    free(pls.shuffledOrder);
    pls.shuffledOrder = nullptr;
  }
  pls.total = total;
  pls.currentIdx = config.lastStation();
  if(pls.currentIdx==0) pls.currentIdx=1;
  if(pls.currentIdx>total) pls.currentIdx=total;
  // keep repeat mode
  if(!pls_initialized) pls.repeat = REPEAT_OFF; // default OFF for flagship: stop at end unless user sets ALL
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
      uint16_t j = random(i+1);
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
      pls.shuffledPos=1;
    }
  }
}

void omnia_shuffle_set_repeat(RepeatMode r){ pls.repeat=r; }
RepeatMode omnia_shuffle_get_repeat(){ return pls.repeat; }
ShuffleMode omnia_shuffle_get_shuffle(){ return pls.shuffle; }
PlaylistState* omnia_shuffle_get_state(){ return &pls; }

bool omnia_shuffle_is_last(){
  if(pls.shuffle==SHUFFLE_OFF){
    return pls.currentIdx >= pls.total;
  }else{
    return pls.shuffledPos >= pls.total;
  }
}

uint16_t omnia_shuffle_next(){
  uint16_t curTotal = config.playlistLength();
  if(curTotal==0) curTotal=1;
  if(!pls_initialized || !pls.shuffledOrder || curTotal != pls.total){
    omnia_shuffle_init(curTotal);
  }
  if(pls.total==0 || !pls.shuffledOrder) return 0; // 0 means stop
  if(pls.shuffle==SHUFFLE_OFF){
    if(pls.currentIdx < pls.total){
      return pls.currentIdx+1;
    }else{
      // at end
      if(pls.repeat==REPEAT_ALL) return 1;
      if(pls.repeat==REPEAT_ONE) return pls.currentIdx;
      return 0; // REPEAT_OFF -> stop
    }
  }else{
    if(pls.shuffledPos < pls.total){
      return pls.shuffledOrder[pls.shuffledPos++];
    }else{
      if(pls.repeat==REPEAT_ALL){
        omnia_shuffle_set(SHUFFLE_ON);
        if(pls.shuffledPos < pls.total) return pls.shuffledOrder[pls.shuffledPos++];
        else return 0;
      }
      if(pls.repeat==REPEAT_ONE) return pls.currentIdx;
      return 0; // REPEAT_OFF -> stop after shuffled all
    }
  }
}

uint16_t omnia_shuffle_prev(){
  uint16_t curTotal = config.playlistLength();
  if(curTotal==0) curTotal=1;
  if(!pls_initialized || !pls.shuffledOrder || curTotal != pls.total){
    omnia_shuffle_init(curTotal);
  }
  if(pls.total==0 || !pls.shuffledOrder) return pls.currentIdx;
  if(pls.shuffle==SHUFFLE_OFF){
    if(pls.currentIdx > 1) return pls.currentIdx-1;
    if(pls.repeat==REPEAT_ALL) return pls.total;
    return pls.currentIdx;
  }else{
    // In shuffle mode, flagship behavior: PREV goes to previous in shuffled order (history), not random new
    // This allows going back
    if(pls.shuffledPos>1){
      pls.shuffledPos-=2;
      uint16_t prev = pls.shuffledOrder[pls.shuffledPos++];
      return prev;
    }
    return pls.currentIdx; // at start of shuffled list
  }
}

void omnia_shuffle_on_track_change(uint16_t newIdx){
  pls.currentIdx=newIdx;
  if(newIdx>pls.total) pls.currentIdx=pls.total;
}
