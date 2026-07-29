#include "shuffle_fix.h"
#include <vector>
#include <algorithm>

static PlaylistState pls;
static std::vector<uint16_t> visited;

void omnia_shuffle_init(uint16_t total){
  pls.total=total; pls.currentIdx=1; pls.shuffle=SHUFFLE_OFF; pls.repeat=REPEAT_ALL;
  pls.shuffledPos=0;
  if(pls.shuffledOrder) free(pls.shuffledOrder);
  pls.shuffledOrder = (uint16_t*)malloc(total*sizeof(uint16_t));
  visited.clear();
  // init order 1..total
  for(uint16_t i=0;i<total;i++) pls.shuffledOrder[i]=i+1;
}

void omnia_shuffle_set(ShuffleMode s){
  pls.shuffle=s;
  if(s==SHUFFLE_ON){
    // Fisher-Yates shuffle
    for(uint16_t i=pls.total-1;i>0;i--){
      uint16_t j = random(0,i+1);
      std::swap(pls.shuffledOrder[i], pls.shuffledOrder[j]);
    }
    pls.shuffledPos=0;
    visited.clear();
  }
}

void omnia_shuffle_set_repeat(RepeatMode r){ pls.repeat=r; }

uint16_t omnia_shuffle_next(){
  if(pls.total==0) return 0;
  if(pls.shuffle==SHUFFLE_OFF){
    if(pls.currentIdx < pls.total) return pls.currentIdx+1;
    // достиг конца
    if(pls.repeat==REPEAT_ALL) return 1;
    if(pls.repeat==REPEAT_ONE) return pls.currentIdx;
    return 0; // STOP
  }else{
    // Shuffle ON
    if(pls.shuffledPos < pls.total){
      uint16_t idx = pls.shuffledOrder[pls.shuffledPos++];
      visited.push_back(idx);
      return idx;
    }else{
      // все проиграны
      if(pls.repeat==REPEAT_ALL){
        omnia_shuffle_set(SHUFFLE_ON); // новая перестановка
        return omnia_shuffle_next();
      }
      if(pls.repeat==REPEAT_ONE) return pls.currentIdx;
      return 0;
    }
  }
}

uint16_t omnia_shuffle_prev(){
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

void omnia_shuffle_on_track_change(uint16_t newIdx){ pls.currentIdx=newIdx; }
