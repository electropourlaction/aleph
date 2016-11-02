/* 
env_tcd_buffer.h
*/

#ifndef _ALEPH_BUFFER_H_
#define _ALEPH_BUFFER_H_

#include "fix.h"
#include "fix32.h"
#include "types.h"

#define BUFFER_SIZE 0x01e84800
#define AUX_SIZE 0xffff
#define AUX_SIZE_1 (AUX_SIZE - 1)
//#define BUFFER_SIZE 0xf000

//SDRAM mono audio buffer
typedef struct _sampleBuffer {
    volatile fract32 *data;             //pointer to data
    u32 samples;                        //count of samples
} sampleBuffer;

//buffer head
typedef struct _bufferHead {
    sampleBuffer *buf;                  //pointer to buffer
    u32 idx;                            //current idx
    u32 start;                          //start point
    u32 end;                            //end|loop point
} bufferHead;

//MDMA buffer
typedef struct _mdmaBuffer {
    volatile fract32 *start;            //pointer to start address
    u32 x_count;
    u32 x_modify;
    u32 y_count;
    u32 y_modify;
} mdmaBuffer;


//init buffer at pre-allocated memory
extern void buffer_init(sampleBuffer *buf, volatile fract32 *data, u32 samples);

//init head
extern void buffer_head_init(bufferHead *head, sampleBuffer *buf);

//play sample at position
extern s32 buffer_head_play(bufferHead *head);

//mix
extern void buffer_head_mix(bufferHead *head, fract32 s, fract32 preLevel);

//record and return sample at idx
extern s32 buffer_head_rec(bufferHead *head, s32 sample);

#endif
