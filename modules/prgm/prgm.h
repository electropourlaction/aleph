//  prgm.h
//  aleph-blackfin

#ifndef _ALEPH_PRGM_H_
#define _ALEPH_PRGM_H_

//#include "fract_math.h"

//standard libraries
#include <stdlib.h>
#include <string.h>

//aleph-common
#include "fix.h"
#include "types.h"

//audiolib env
#include "env_tcd_buffer.h"
#include "filter_1p.h"
#include "constants.h"

//bfin
#include "bfin_core.h"
#include "module.h"
#include "params.h"

//MDMA DEBUG
#include "defBF532.h"

#define N_CHN 6                         //number of channels
#define N_INPUTS 23                     //number of selectable inputs
#define N_DIROUTS 8                     //number of selectable direct outputs
#define N_HEADS 2                       //number of play|record heads


//  channel
typedef struct _prgmChn {
    //  input
    fract32 (*input)(void *);           //pointer to input
    
    //  cv
    fract32 (*cvprocess)(void *);       //pointer to cv process (frame)
    fract32 cvframe;                    //internal cv output value

    bufferHead head[N_HEADS];           //play|record heads
    u32 loop;
    fract32 x;                          //parameters
    fract32 y;
    fract32 c0;
    fract32 c1;
    fract32 c2;
    fract32 c3;
    
    u8 trig;
    u8 retrig;
    u8 send_retrig;

    //  output
    u8 flag;
    fract32 (*process)(void *);         //pointer to audio process (frame)

    //  parameters
    fract32 a;                          //modulated parameter
    u8 amod;
    fract32 amodl;                      //parameter mod level

    fract32 b;                          //parameter
    fract32 c;                          //parameter
    fract32 d;                          //parameter
    
    //  mix parameters
    fract32 aux;                        //aux send level
    u8 auxmod;
    fract32 auxmodl;                    //aux modulation level

    fract32 auxcv;                      //auxcv send level
    
    fract32 snd;                        //channel send level
    u8 sndmod;
    fract32 sndmodl;                    //send modulation level
    
    u8 to_mix;                          //route to mix
    u8 to_grp1;                         //route to group 1
    u8 to_grp2;                         //route to group 2
    
    //  deck parameters
    u8 deck_input;                      //deck input select

    //  mute|solo
    u8 mute;
    u8 auxmute;
    fract32 hold;
    fract32 tmp;
    
    //  slew filters
    filter_1p_lo pSlew;                 //parameter slew
    filter_1p_lo auxSlew;               //aux slew
    filter_1p_lo sndSlew;               //send slew
} prgmChn;

typedef struct _prgmChn *PrgmChnptr;


//  master
typedef struct _prgmMaster {
    fract32 (*output3)(void *);         //pointer to direct output 3
    fract32 (*output4)(void *);         //pointer to direct output 4
    fract32 direct;                     //direct 3/4 input level

    fract32 grp1;                       //group A to mix level
    fract32 grp2;                       //group B to mix level
    fract32 output;                     //master output level
} prgmMaster;

typedef struct _prgmMaster *PrgmMasterptr;


#endif
