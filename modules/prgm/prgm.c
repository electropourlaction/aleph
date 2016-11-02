//prgm.c, aleph-bfin
#include "prgm.h"

//  data types
typedef struct _prgmData {
    ModuleData super;
    ParamData mParamData[eParamNumParams];
    
    //audio buffer
    volatile fract32 sampleBuffer[BUFFER_SIZE];

} prgmData;

ModuleData *gModuleData;

//  pointer to SDRAM (all external memory)
prgmData *data;

//  samples|buffers
sampleBuffer onchipbuffer[N_CHN];
s32 samples[32];
u32 offset;

//  sequencer
s32 pp[N_CHN];              //  step ping pong buffer

//  cv|trig
fract32 cv[N_CHN+1];        //  cv outputs (6 cv channels, 1 cv aux)
fract32 trig = 0;           //  trig bus

//  channels
prgmChn *chn[N_CHN];        //  mix channels

//  virtual outputs
fract32 vout[N_CHN] = { 0, 0, 0, 0, 0, 0 };

//  aux
fract32 aux = 0;

//  master
prgmMaster *master;         //  master section


//  function declarations
//  memory allocation
static void init_channel_parameters(prgmChn *c, u8 n);
static void init_master_parameters(void);

//set head offsets
static void set_start(prgmChn *c, u32 offset);
static void set_loop(prgmChn *c, u32 offset);

//  inputs
static fract32 (*set_input(u8 n))();
static fract32 in_noinput(void);
static fract32 in_in0(void);
static fract32 in_in1(void);
static fract32 in_vout0(void);
static fract32 in_vrev0(void);
static fract32 in_vpad0(void);
static fract32 in_vout1(void);
static fract32 in_vrev1(void);
static fract32 in_vpad1(void);
static fract32 in_vout2(void);
static fract32 in_vrev2(void);
static fract32 in_vpad2(void);
static fract32 in_vout3(void);
static fract32 in_vrev3(void);
static fract32 in_vpad3(void);
static fract32 in_vout4(void);
static fract32 in_vrev4(void);
static fract32 in_vpad4(void);
static fract32 in_vout5(void);
static fract32 in_vrev5(void);
static fract32 in_vpad5(void);
static fract32 in_aux(void);
static fract32 in_arev(void);

//  cv
static fract32 (*set_cvprocess(u8 n))();
static fract32 cv_off(prgmChn *c);
static fract32 cv_imp(prgmChn *c);
static fract32 cv_lin(prgmChn *c);
static fract32 cv_pre(prgmChn *c);
static fract32 cv_bpf(prgmChn *c);
static fract32 cv_dly(prgmChn *c);
static fract32 cv_lfo(prgmChn *c);
static fract32 cv_env(prgmChn *c);
static fract32 cv_gate(prgmChn *c);
static fract32 cv_rpt(prgmChn *c);

//  audio
static fract32 (*set_process(u8 n))();
static fract32 pf_off(prgmChn *c);
static fract32 pf_imp(prgmChn *c);
static fract32 pf_lin(prgmChn *c);
static fract32 pf_tape(prgmChn *c);
static fract32 pf_deck(prgmChn *c);
static fract32 pf_pre(prgmChn *c);
static fract32 pf_bpf(prgmChn *c);
static fract32 pf_dly(prgmChn *c);
static fract32 pf_rpt(prgmChn *c);
static fract32 pf_mute(prgmChn *c);
static fract32 pf_holdmute(prgmChn *c);
static fract32 pf_unmute(prgmChn *c);

//  outputs
static fract32 (*set_output(u8 n))();
static fract32 out_off(void);
static fract32 out_vout0(void);
static fract32 out_vout1(void);
static fract32 out_vout2(void);
static fract32 out_vout3(void);
static fract32 out_vout4(void);
static fract32 out_vout5(void);
static fract32 out_aux(void);
static void route_mix(s32 t, s32 r);


//MDMA DEBUG
void bfin_debug (u8 reg)
{
    switch (reg)
    {
        case 0:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & DMAEN;
            break;
        case 1:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & WDSIZE_8;
            break;
        case 2:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & WDSIZE_16;
            break;
        case 3:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & WDSIZE_32;
            break;
        case 4:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & DMA2D;
            break;
        case 5:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & DI_SEL;
            break;
        case 6:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & DI_EN;
            break;
        case 7:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & NDSIZE;
            break;
        case 8:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & FLOW_STOP;
            break;
        case 9:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & FLOW_AUTO;
            break;
        case 10:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & FLOW_ARRAY;
            break;
        case 11:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & FLOW_SMALL;
            break;
        case 12:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & FLOW_LARGE;
            break;
        case 13:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & DMA_DONE;
            break;
        case 14:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & DMA_ERR;
            break;
        case 15:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & DFETCH;
            break;
        case 16:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CONFIG & DMA_RUN;
            break;
        case 17:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_NEXT_DESC_PTR;
            break;
        case 18:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_START_ADDR;
            break;
        case 19:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_X_COUNT;
            break;
        case 20:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_Y_COUNT;
            break;
        case 21:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_X_MODIFY;
            break;
        case 22:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_Y_MODIFY;
            break;
        case 23:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CURR_DESC_PTR;
            break;
        case 24:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CURR_ADDR;
            break;
        case 25:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CURR_X_COUNT;
            break;
        case 26:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_CURR_Y_COUNT;
            break;
        case 27:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_IRQ_STATUS;
            break;
        case 28:
            gModuleData->paramData[eParamDebug].value = MDMA_S0_PERIPHERAL_MAP;
            break;
            
        default:
            break;
    }
}


//function definitions
void init_channel_parameters(prgmChn *c, u8 n) {
    c->input = set_input(0);

    c->cvprocess = set_cvprocess(0);
    c->cvframe = 0;
    //  buffers are initialised in module_init!

    c->loop = 0;
    c->x = 0;
    c->y = 0;
    c->c0 = 0;
    c->c1 = 0;
    c->c2 = 0;
    c->c3 = 0;
    
    c->trig = 0;
    c->retrig = 0;
    c->send_retrig = 0;
    
    c->flag = 0;
    c->process = set_process(0);
    
    c->a = 0;
    c->amod = n;
    c->amodl = 0;
    c->b = 0;
    c->c = 0;
    c->d = 0;
    
    c->aux = 0;
    c->auxmod = n;
    c->auxmodl = 0;
    
    c->auxcv = 0;
    
    c->snd = FR32_MAX;
    c->sndmod = n;
    c->sndmodl = 0;
    
    c->to_mix = 1;
    c->to_grp1 = 0;
    c->to_grp2 = 0;
    
    c->deck_input = 0;
    
    c->mute = 0;
    c->auxmute = 0;
    c->hold = 0;
    c->tmp = 0;
    
    filter_1p_lo_init(&(c->pSlew), 0xf);
    filter_1p_lo_set_slew(&(c->pSlew), 0x5f000000); //parameter slew
    filter_1p_lo_init(&(c->auxSlew), 0xf);
    filter_1p_lo_set_slew(&(c->auxSlew), 0x7f000000); //aux slew
    filter_1p_lo_init(&(c->sndSlew), 0xf);
    filter_1p_lo_set_slew(&(c->sndSlew), 0x7f000000); //send slew
}

void init_master_parameters() {
    master->output3 = set_output(0);
    master->output4 = set_output(0);
    master->direct = FR32_MAX;
    
    master->grp1 = FR32_MAX;
    master->grp2 = FR32_MAX;
    master->output = FR32_MAX;
}


//  inputs
fract32 (*set_input(u8 n))() {
    static fract32 (*inputs[])() =
    {
        in_noinput,
        in_aux,
        in_arev,
        in_in0,
        in_in1,
        in_vout0,
        in_vrev0,
        in_vpad0,
        in_vout1,
        in_vrev1,
        in_vpad1,
        in_vout2,
        in_vrev2,
        in_vpad2,
        in_vout3,
        in_vrev3,
        in_vpad3,
        in_vout4,
        in_vrev4,
        in_vpad4,
        in_vout5,
        in_vrev5,
        in_vpad5,
    };
    
    return (n < 1 || n > N_INPUTS) ? *inputs[0] : *inputs[n];
}

//  cv processes
static fract32 (*set_cvprocess(u8 n))() {
    static fract32 (*process[])() =
    {
        cv_off,             //0
        
        cv_imp,             //1
        cv_lin,             //2
        cv_off,             //3 tape
        cv_off,             //4 deck
        
        cv_pre,             //5
        cv_bpf,             //6
        cv_dly,             //7
        
        cv_lfo,             //8
        cv_env,             //9
        cv_gate,            //10
        
        cv_rpt,             //11
        cv_off,             //12 mute
        cv_off,             //13 holdmute
        cv_off,             //14 unmute
    };
    
    return (n < 1 || n > 15) ? *process[0] : *process[n];
}


//  audio processes
fract32 (*set_process(u8 n))() {
    static fract32 (*process[])() =
    {
        pf_off,             //0
        
        pf_imp,             //1
        pf_lin,             //2
        pf_tape,            //3
        pf_deck,            //4
        
        pf_pre,             //5
        pf_bpf,             //6
        pf_dly,             //7
        
        pf_off,             //8 lfo
        pf_off,             //9 env
        pf_off,             //10 gate

        pf_rpt,             //11
        pf_mute,            //12
        pf_holdmute,        //13
        pf_unmute,          //14
    };
    
    return (n < 1 || n > 15) ? *process[0] : *process[n];
}


//  direct outs
fract32 (*set_output(u8 n))() {
    static fract32 (*outputs[])() =
    {
        out_off,
        out_vout0,
        out_vout1,
        out_vout2,
        out_vout3,
        out_vout4,
        out_vout5,
        out_aux,
    };
    
    return (n < 1 || n > N_DIROUTS) ? *outputs[0] : *outputs[n];
}



static inline void param_setup(u32 id, ParamValue v) {
    gModuleData->paramData[id].value = v;
    //    module_set_param(id, v);
}

void module_load_sample(s32 sample) {
}

void module_init(void) {
    data = (prgmData*)SDRAM_ADDRESS;
    gModuleData = &(data->super);
    strcpy(gModuleData->name, "aleph-prgm");
    gModuleData->paramData = data->mParamData;
    gModuleData->numParams = eParamNumParams;
    
    u32 i, n;
    
    //  init channels
    for(n=0; n<N_CHN; n++)
    {
        chn[n] = (PrgmChnptr)malloc(sizeof(prgmChn));
        init_channel_parameters(chn[n], n);
    }
    
    //  init master
    master = (PrgmMasterptr)malloc(sizeof(prgmMaster));
    init_master_parameters();
    
    //  init channel buffers
    for(n=0; n<N_CHN; n++)
    {
        buffer_init(&(onchipbuffer[n]), data->sampleBuffer, BUFFER_SIZE);
    }
    
    //  init channel playheads
    for(n=0; n<N_CHN; n++)
    {
        for (i=0; i<N_HEADS; i++)
        {
            buffer_head_init(&(chn[n])->head[i], &(onchipbuffer[n]));
        }
    }
    
    //  zero sample buffer
    for(i=0; i<BUFFER_SIZE; i++)
    {
        data->sampleBuffer[i] = 0x00000000;
    }
    
    for(n=0; n<32; n++)
    {
        samples[n] = 0;
    }
    
    //  init static variables
    offset = 0;
    trig = 0;
    
    for(n=0; n<N_CHN+1; n++)
    {
        cv[n] = 0x00000000;
    }
    
    //  init parameters
    for(i=0; i<module_get_num_params(); i++) param_setup(i, 0);
}


void module_deinit(void) {}


extern u32 module_get_num_params(void) {
    return eParamNumParams;
}


//  process frame
void module_process_frame(void) {
    u8 i;
    fract32 cvtmp, tmp, mod, a, b;
    
    cvtmp = tmp = mod = 0x00000000;

    //  cv | auxcv
    for(i=0;i<N_CHN;i++)
    {
        if(!chn[i]->flag)
        {
            cv[i] = 0x00000000;
        }
        else
        {
            //  cv outputs
            cv[i] = chn[i]->cvprocess(chn[i]);
            
            //  mix aux
            //  w aux modulation
            if (chn[i]->auxmodl != 0)
            {
                mod = add_fr1x32(chn[i]->aux, mult_fr1x32x32(cv[chn[i]->auxmod], chn[i]->auxmodl));
                if (mod < 0) mod = 0;
                
                //  aux
                tmp = add_fr1x32(tmp, mult_fr1x32x32(vout[i], mod));
                
                //  auxcv
                cvtmp = add_fr1x32(cvtmp, mult_fr1x32x32(cv[i], chn[i]->auxcv));
            }
            //  w/o aux modulation
            else
            {
                //  aux
                tmp = add_fr1x32(tmp, mult_fr1x32x32(vout[i], chn[i]->aux));
                
                //  auxcv
                cvtmp = add_fr1x32(cvtmp, mult_fr1x32x32(cv[i], chn[i]->auxcv));
            }
        }
    }

    aux = tmp;
    cv[6] = cvtmp;
    
    //  process audio
    a = b = tmp = mod = 0x00000000;
    
    for(i=0;i<N_CHN;i++)
    {
        fract32 v = 0x00000000;
        //  skip inactive channels
        if(!chn[i]->flag)
        {
            vout[i] = 0x00000000;
        }
        else
        {
            //  calculate active channels
            vout[i] = 0x00000000;
            v = vout[i] = chn[i]->process(chn[i]);
        }

        //  mix
        if(chn[i]->to_mix)
        {
            if (chn[i]->sndmodl != 0)
            {
                mod = add_fr1x32(chn[i]->snd, mult_fr1x32x32(cv[chn[i]->sndmod], chn[i]->sndmodl));
                if (mod < 0) mod = 0;
                tmp = add_fr1x32(tmp, mult_fr1x32x32(v, mod));
            }
            else tmp = add_fr1x32(tmp, mult_fr1x32x32(v, chn[i]->snd));

        }                                                                                                                                                                 
        //  group 1
        if(chn[i]->to_grp1)
        {
            if (chn[i]->sndmodl != 0)
            {
                mod = add_fr1x32(chn[i]->snd, mult_fr1x32x32(cv[chn[i]->sndmod], chn[i]->sndmodl));
                if (mod < 0) mod = 0;
                a = add_fr1x32(a, mult_fr1x32x32(v, mod));
            }
            else a = add_fr1x32(a, mult_fr1x32x32(v, chn[i]->snd));
        }
        //  group 2
        if(chn[i]->to_grp2)
        {
            if (chn[i]->sndmodl != 0)
            {
                mod = add_fr1x32(chn[i]->snd, mult_fr1x32x32(cv[chn[i]->sndmod], chn[i]->sndmodl));
                if (mod < 0) mod = 0;
                b = add_fr1x32(b, mult_fr1x32x32(v, mod));
            }
            else b = add_fr1x32(b, mult_fr1x32x32(v, chn[i]->snd));
        }
    }

    //  add groups to mix
    tmp = add_fr1x32(tmp, mult_fr1x32x32(a, master->grp1));
    tmp = add_fr1x32(tmp, mult_fr1x32x32(b, master->grp2));

    //if clip detection s64 clip, else s32 tmp... ON MIX!
    //TRY S64 CLIP_TMP HERE TO DETECT CLIPPING AT OUTPUT MIX...
    /*
     if (clip > FR32_MAX) return 0;
     */
    
    //  add external inputs to mix, output mix
//    if (in[2]) out[0] = add_fr1x32(mult_fr1x32x32(tmp, master->output), mult_fr1x32x32(in[2], master->direct));
    out[0] = out[1] = mult_fr1x32x32(tmp, master->output);
    
//    if (in[3]) out[1] = add_fr1x32(mult_fr1x32x32(tmp, master->output), mult_fr1x32x32(in[3], master->direct));
//    out[1] = mult_fr1x32x32(tmp, master->output);
    
    //  direct outputs
    out[2] = master->output3(master);
    out[3] = master->output4(master);    
}

//mix routing
static void route_mix(s32 n, s32 r) {
    //  mix
    if (r == 0)
    {
        chn[n]->to_mix = 1;
        chn[n]->to_grp1 = 0;
        chn[n]->to_grp2 = 0;
    }
    //  group 1
    else if (r == 1)
    {
        chn[n]->to_mix = 0;
        chn[n]->to_grp1 = 1;
        chn[n]->to_grp2 = 0;
    }
    //  group 1
    else if (r == 2)
    {
        chn[n]->to_mix = 0;
        chn[n]->to_grp1 = 0;
        chn[n]->to_grp2 = 1;
    }
    //  group 1 + 2
    else if (r == 3)
    {
        chn[n]->to_mix = 0;
        chn[n]->to_grp1 = 1;
        chn[n]->to_grp2 = 1;
    }
    //  mix + group 1
    else if (r == 4)
    {
        chn[n]->to_mix = 1;
        chn[n]->to_grp1 = 1;
        chn[n]->to_grp2 = 0;
    }
    // mix + group 2
    else if (r == 5)
    {
        chn[n]->to_mix = 1;
        chn[n]->to_grp1 = 0;
        chn[n]->to_grp2 = 1;
    }
    else ;
}


void module_clock_in(void) {
    u8 n;
    s32 pptmp;
    
    //  refresh step value
    for (n = 0; n < N_CHN; n++)
    {
        pptmp = pp[n];
        
        if (pptmp)
        {
            //  imp
            if (chn[n]->flag == 1)
            {
                chn[n]->trig = 1;
            }
            //  lin
            if (chn[n]->flag == 2)
            {
                //  sample
                set_loop(chn[n], samples[pptmp + 1]);
                set_start(chn[n], samples[pptmp]);
                chn[n]->trig = 1;
            }
            //  tape
            if (chn[n]->flag == 11)
            {
                /*
                //  command
                if (((pptmp >> 24) & 0xff) == start)
                {
//                    chn[s]->c = v;
                    env_tcd_set_c(&(chn[s]->envAmp), 1);
                }
                else if (ppval byte 0 == stop)
                {
                    env_tcd_set_c(&(chn[s]->envAmp), 0);
                }
                else if (ppval byte 0 == reverse)
                {
                    env_tcd_set_c(&(chn[s]->envAmp), 2);
                }
                //  speed
                else
                {
                    env_tcd_set_a(&(chn[s]->envAmp), byte123 val);
                }
                */
            }
            //  deck
            if (chn[n]->flag == 12)
            {
                /*
                //  command
                cmd_0[output] = (pptmp >> 24) & 0xff;
                cmd_1[output] = (pptmp >> 16) & 0xff;
                cmd_2[output] = (pptmp >> 8) & 0xff;
                cmd_3[output] = pptmp & 0xff;
                
                uart_write(output);
                */
            }
            else ;
            /*
            {
             CV TRACK?!??!
                cv[n] = pp_val[n];
            }
            */
            
            pp[n] = 0;
        }
    }
}


void module_send_retrig(void) {
    u8 n;

    for (n = 0; n < N_CHN; n++)
    {
        if (chn[n]->retrig) chn[n]->trig = 1;
    }
}


//  parameters
void module_set_param(u32 idx, ParamValue v) {};

void param_set_sampleoffset(ParamValue v) {
    offset = v;
}

void param_set_samplevalue(ParamValue v) {
    data->sampleBuffer[offset] = v;
}

void set_start(prgmChn *c, u32 offset) {
    u8 i;
    
    for (i=0; i<N_HEADS; i++)
    {
        c->head[i].start = offset;
    }
}

void set_loop(prgmChn *c, u32 offset) {
    u8 i;
    
    for (i=0; i<N_HEADS; i++)
    {
        c->head[i].end = offset;
    }
}


void module_set_sqparam(u32 s, u32 idx, ParamValue v) {

    switch(idx) {
        case eParamDummy:
            break;

        case eParamOffset:
            param_set_sampleoffset(v);
            break;
        case eParamSample:
            param_set_samplevalue(v);
            break;
        case eParamSampleLoopPoint:
            samples[s] = v;
            break;

        case eParamPP:
            pp[s] = v;
            break;
            
        case eParamTrig:
            chn[s]->trig = 1;
            break;
        case eParamRetrig:
            chn[s]->retrig = v;
            break;
        case eParamSendRetrig:
            chn[s]->send_retrig = v;
            break;
            
        case eParamInput:
            chn[s]->input = set_input(v);
            break;
            
        case eParamFlag:
            chn[s]->flag = v;
            chn[s]->process = set_process(v);
            chn[s]->cvprocess = set_cvprocess(v);
            break;
            
        case eParamMuteFlag:
            chn[s]->mute = v;
            break;
        case eParamMute:
            chn[s]->process = set_process(12);
            break;            
        case eParamUnMute:
            chn[s]->process = set_process(14);
            break;
            
        case eParamOffsetA:
            set_start(chn[s], v);
            break;
            
        case eParamOffsetB:
            set_loop(chn[s], v);
            break;
            
        case eParamA:
            chn[s]->a = v;
            break;
        case eParamAModSrc:
            chn[s]->amod = v;
            break;
        case eParamAModAmount:
            chn[s]->amodl = v;
            break;
        case eParamB:
            chn[s]->b = v;
            break;
        case eParamC:
            chn[s]->c = v;
            break;
        case eParamD:
            chn[s]->d = v;
            break;
            
        case eParamRouteSnd:
            route_mix(s, v);
            break;
        case eParamSndLevel:
            chn[s]->snd = v;
            filter_1p_lo_in(&(chn[s]->sndSlew), v);
            break;
        case eParamSndModSrc:
            chn[s]->sndmod = v;
        case eParamSndModAmount:
            chn[s]->sndmodl = v;
            break;
            
        case eParamAuxLevel:
            chn[s]->aux = v;
            break;
        case eParamAuxModSrc:
            chn[s]->auxmod = v;
            break;
        case eParamAuxModAmount:
            chn[s]->auxmodl = v;
            break;
            
        case eParamAuxCvLevel:
            chn[s]->auxcv = v;
            break;
            
        case eParamDirectOut3:
            master->output3 = set_output(v);
            break;
        case eParamDirectOut4:
            master->output4 = set_output(v);
            break;

        case eParamGroup1:
            master->grp1 = v;
            break;
        case eParamGroup2:
            master->grp2 = v;
            break;
        case eParamMaster:
            master->output = v;
            break;
            
        case eParamStartupTime:
            chn[s]->b = v;
            break;

        case eParamDebug:
            bfin_debug(v);
            break;
            
            
        default:
            break;
    }
}

//  inputs
fract32 in_noinput(void) {
    return 0;
}

fract32 in_in0(void) {
    return in[0];
}

fract32 in_in1(void) {
    return in[1];
}

fract32 in_vout0(void) {
    return vout[0];
}

fract32 in_vrev0(void) {
    return negate_fr1x32(vout[0]);
}

fract32 in_vpad0(void) {
    return mult_fr1x32x32(vout[0], PAD);
}

fract32 in_vout1(void) {
    return vout[1];
}

fract32 in_vrev1(void) {
    return negate_fr1x32(vout[1]);
}

fract32 in_vpad1(void) {
    return mult_fr1x32x32(vout[1], PAD);
}

fract32 in_vout2(void) {
    return vout[2];
}

fract32 in_vrev2(void) {
    return negate_fr1x32(vout[2]);
}

fract32 in_vpad2(void) {
    return mult_fr1x32x32(vout[2], PAD);
}

fract32 in_vout3(void) {
    return vout[3];
}

fract32 in_vrev3(void) {
    return negate_fr1x32(vout[3]);
}

fract32 in_vpad3(void) {
    return mult_fr1x32x32(vout[3], PAD);
}

fract32 in_vout4(void) {
    return vout[4];
}

fract32 in_vrev4(void) {
    return negate_fr1x32(vout[4]);
}

fract32 in_vpad4(void) {
    return mult_fr1x32x32(vout[4], PAD);
}

fract32 in_vout5(void) {
    return vout[5];
}

fract32 in_vrev5(void) {
    return negate_fr1x32(vout[5]);
}

fract32 in_vpad5(void) {
    return mult_fr1x32x32(vout[5], PAD);
}


fract32 in_aux(void) {
    return aux;
}

fract32 in_arev(void) {
    return negate_fr1x32(aux);
}


//  cv
fract32 cv_off(prgmChn *c) {
    return 0;
}

fract32 cv_imp(prgmChn *c) {
    fract32 tmp;
    fract32 mod = mult_fr1x32x32(cv[c->amod], c->amodl);
    //  mode parameter: impulse length
    //  send retrig: on gate end
    //  return gate

    //  impulse start
    if (c->trig)
    {
        c->trig = 0;
        
        //  set impulse length w scaled modulation
        if ((tmp = add_fr1x32(c->a, (mod >> 16))) > 0xf)
        {
            c->x = tmp;
            c->y = tmp >> 1;
        }
        else
        {
            c->x = c->y = 0xf;
        }
        return c->cvframe = 0;
    }
    else
    {
        //  gate on
        if (c->x > c->y)
        {
            c->x -= 1;

            return c->cvframe = FR32_MAX;
        }
        //  gate off
        else if (c->x > 0xf)
        {
            c->x -= 1;

            return c->cvframe = 0;
        }
        //  end of impulse
        else if (c->x > 0x7)
        {
            c->x = 0;
            
            if (c->send_retrig) module_send_retrig();
            return c->cvframe = 0;
        }
        //  rest
        else
        {
            return c->cvframe = 0;
        }
    }
}

fract32 cv_lin(prgmChn *c) {
    //  send retrig: on sample start
    //  return sample
    
    if (c->trig)
    {
        c->trig = 0;
        
        //  set playback head position
        c->head[0].idx = c->head[0].start;
        if (c->send_retrig) module_send_retrig();
        
        //  return first sample
        return c->cvframe = buffer_head_play(&(c->head[0]));
    }
    else
    {
        //  end of sample
        if (c->head[0].idx > c->head[0].end)
        {
            return 0;
        }
        //  playback sample
        else
        {
            c->head[0].idx += 1;
            return c->cvframe = buffer_head_play(&(c->head[0]));
        }
    }
}

fract32 cv_pre(prgmChn *c) {
    //  mode parameter: threshold level
    //  send retrig: on env start
    //  return env

    return c->cvframe = c->input(c);
}

fract32 cv_bpf(prgmChn *c) {
    //  static cv source
    
    return c->a;
}

fract32 cv_dly(prgmChn *c) {
    //  mode parameter: lfo rate
    //  send retrig: on lfo loop
    //  return lfo

    c->x += c->d;
    return c->cvframe = abs_fr1x32(mult_fr1x32x32(FR32_MAX, negate_fr1x32(c->x)));
}

fract32 cv_rpt(prgmChn *c) {
    //  mode parameter: lfo rate
    //  send retrig: on lfo loop
    //  return lfo
        
    c->x += c->d;
    return c->cvframe = abs_fr1x32(mult_fr1x32x32(FR32_MAX, negate_fr1x32(c->x)));
}

fract32 cv_lfo(prgmChn *c) {
    //  mode parameter: lfo rate
    //  send retrig: on lfo loop
    //  return lfo
    
    if (c->trig)
    {
        c->trig = 0;
        c->x = 0;
        return 0;
    }
    else
    {
        fract32 mod = mult_fr1x32x32(cv[c->amod], c->amodl);
        
        c->x += add_fr1x32(c->a, mod);
        c->cvframe = abs_fr1x32(mult_fr1x32x32(FR32_MAX, negate_fr1x32(c->x)));
        if (c->send_retrig)
        {
            if (c->cvframe < c->a) module_send_retrig();
        }
        return c->cvframe;
    }
}

fract32 cv_env(prgmChn *c) {
    //  return env with shape modulation

    return 0;
}

fract32 cv_gate(prgmChn *c) {
    //  return gate with level modulation
    
    return 0;
    /*
    filter_1p_lo_in(&(c->pSlew), c->a);
    
    if (filter_1p_sync(&(c->pSlew)))
    {
        return c->a;
    }
    else
    {
        return filter_1p_lo_next(&(c->pSlew));
    }
    */
}

fract32 cv_mute(prgmChn *c) {
    //  no cv output
    return 0;
}

fract32 cv_holdmute(prgmChn *c) {
    //  no cv output
    return 0;
}

fract32 cv_unmute(prgmChn *c) {
    //  no cv output
    return 0;
}


//  audio
fract32 pf_off(prgmChn *c) {
    return 0;
}

fract32 pf_imp(prgmChn *c) {
    //  return impulse
    return c->cvframe;
}

fract32 pf_lin(prgmChn *c) {
    fract32 mod = mult_fr1x32x32(cv[c->amod], c->amodl);

    //  return sample w level modulation
    return add_fr1x32(mult_fr1x32x32(c->cvframe, c->a), mult_fr1x32x32(c->cvframe, mod));
}

//tape
fract32 pf_tape(prgmChn *c) {
    if (c->c)
    {
        fract32 tmp = add_fr1x32(c->a, mult_fr1x32x32(cv[c->amod], c->amodl));
        
        //  reverse
        if (tmp < 0)
        {
            if (c->trig)
            {
                c->trig = 0;
                c->x = 0;
                
                //  perform pre-calculations, return first sample
                fract32 loop = sub_fr1x32(c->head[0].end, c->head[0].start);
                fract32 y1n = sub_fr1x32(c->head[0].start, loop);
                fract32 y0 = c->head[0].start;
                fract32 y1 = c->head[0].end;
                fract32 y2 = add_fr1x32(c->head[0].end, loop);
                fract32 e1 = add_fr1x32(y1n, y2), o1 = sub_fr1x32(y2, y1n);
                fract32 e2 = add_fr1x32(y0, y1), o2 = sub_fr1x32(y1, y0);
                c->c0 = add_fr1x32(mult_fr1x32x32(FR32_MAX1_48, e1), mult_fr1x32x32(FR32_MAX23_48, e2));
                c->c1 = add_fr1x32(mult_fr1x32x32(FR32_MAX1_8, o1), mult_fr1x32x32(FR32_MAX5_8, o2));
                c->c2 = mult_fr1x32x32(FR32_MAX1_4, sub_fr1x32(e1, e2));
                c->c3 = sub_fr1x32(mult_fr1x32x32(FR32_MAX1_6, o1), mult_fr1x32x32(FR32_MAX1_2, o2));
                c->head[0].idx = y1;
                return buffer_head_play(&(c->head[0]));
            }
            else
            {
                //  return interpolated sample
                c->x -= tmp;
                fract32 z = sub_fr1x32(FR32_MAX1_2, c->x);
                c->head[0].idx = add_fr1x32(c->c0, mult_fr1x32x32(z, add_fr1x32(c->c1, mult_fr1x32x32(z, add_fr1x32(c->c2, mult_fr1x32x32(z, c->c3))))));
                if (c->head[0].idx < c->head[0].start + 0xf)
                {
                    c->x = 0;
                    c->head[0].idx = c->head[0].end;
                    if (c->send_retrig) module_send_retrig();
                }
                return buffer_head_play(&(c->head[0]));
            }
        }
        //  forward
        else
        {
            if (c->trig)
            {
                c->trig = 0;
                c->x = 0;
                
                //  perform pre-calculations, return first sample
                fract32 loop = sub_fr1x32(c->head[0].end, c->head[0].start);
                fract32 y1n = sub_fr1x32(c->head[0].start, loop);
                fract32 y0 = c->head[0].start;
                fract32 y1 = c->head[0].end;
                fract32 y2 = add_fr1x32(c->head[0].end, loop);
                fract32 e1 = add_fr1x32(y1n, y2), o1 = sub_fr1x32(y2, y1n);
                fract32 e2 = add_fr1x32(y0, y1), o2 = sub_fr1x32(y1, y0);
                c->c0 = add_fr1x32(mult_fr1x32x32(FR32_MAX1_48, e1), mult_fr1x32x32(FR32_MAX23_48, e2));
                c->c1 = add_fr1x32(mult_fr1x32x32(FR32_MAX1_8, o1), mult_fr1x32x32(FR32_MAX5_8, o2));
                c->c2 = mult_fr1x32x32(FR32_MAX1_4, sub_fr1x32(e1, e2));
                c->c3 = sub_fr1x32(mult_fr1x32x32(FR32_MAX1_6, o1), mult_fr1x32x32(FR32_MAX1_2, o2));
                c->head[0].idx = y0;
                return buffer_head_play(&(c->head[0]));
            }
            else
            {
                //  return interpolated sample
                c->x += tmp;
                fract32 z = sub_fr1x32(c->x, FR32_MAX1_2);
                c->head[0].idx = add_fr1x32(c->c0, mult_fr1x32x32(z, add_fr1x32(c->c1, mult_fr1x32x32(z, add_fr1x32(c->c2, mult_fr1x32x32(z, c->c3))))));
                if (c->head[0].idx > c->head[0].end - 0xf)
                {
                    c->x = 0;
                    c->head[0].idx = c->head[0].start;
                    if (c->send_retrig) module_send_retrig();
                }
                return buffer_head_play(&(c->head[0]));
            }
        }
    }
    else return 0;
}

/*
fract32 pf_tape(prgmChn *c) {
    if (c->c)
    {
        if (c->trig)
        {
            c->trig = 0;
            c->x = 0;
            
            //  perform pre-calculations, return first sample
            fract32 loop = sub_fr1x32(c->head[0].end, c->head[0].start);
            fract32 y1n = sub_fr1x32(c->head[0].start, loop);
            fract32 y0 = c->head[0].start;
            fract32 y1 = c->head[0].end;
            fract32 y2 = add_fr1x32(c->head[0].end, loop);
            fract32 e1 = add_fr1x32(y1n, y2), o1 = sub_fr1x32(y2, y1n);
            fract32 e2 = add_fr1x32(y0, y1), o2 = sub_fr1x32(y1, y0);
            c->c0 = add_fr1x32(mult_fr1x32x32(FR32_MAX1_48, e1), mult_fr1x32x32(FR32_MAX23_48, e2));
            c->c1 = add_fr1x32(mult_fr1x32x32(FR32_MAX1_8, o1), mult_fr1x32x32(FR32_MAX5_8, o2));
            c->c2 = mult_fr1x32x32(FR32_MAX1_4, sub_fr1x32(e1, e2));
            c->c3 = sub_fr1x32(mult_fr1x32x32(FR32_MAX1_6, o1), mult_fr1x32x32(FR32_MAX1_2, o2));
            c->head[0].idx = y0;
            return buffer_head_play(&(c->head[0]));
        }
        else
        {
            //  return interpolated sample
            c->x += tmp;
            fract32 z = sub_fr1x32(c->x, FR32_MAX1_2);
            c->head[0].idx = add_fr1x32(c->c0, mult_fr1x32x32(z, add_fr1x32(c->c1, mult_fr1x32x32(z, add_fr1x32(c->c2, mult_fr1x32x32(z, c->c3))))));
            if (c->head[0].idx >= c->head[0].end - 0xf)
            {
                c->x = 0;
                c->head[0].idx = c->head[0].start;
                if (c->send_retrig) module_send_retrig();
            }
            return buffer_head_play(&(c->head[0]));
        }
    }
    else return 0;
}
*/

fract32 pf_deck(prgmChn *c) {
    //  loop playback
    if (c->a == 0)
    {
        //  return first sample
        if (c->trig)
        {
            c->trig = 0;
            c->head[0].idx = c->head[0].start;
            return buffer_head_play(&(c->head[0]));
        }
        //  return next sample
        else
        {
            c->head[0].idx += 1;
            if (c->head[0].idx >= c->head[0].end)
            {
                c->head[0].idx = c->head[0].start;
                if (c->send_retrig) module_send_retrig();
            }
            return buffer_head_play(&(c->head[0]));
        }
    }
    //  record
    else if (c->a == 1)
    {
        //  startup time counter
        if (c->b > 0x1)
        {
            c->b -= 1;
            return 0;
        }
        //  start recording
        else
        {
            //  refresh index and loop point
            c->head[0].idx = c->loop;
            c->loop += 1;
            
            //  write to buffer
            if (c->loop > (c->head[0].start + AUX_SIZE_1))
            {
                //  setup playback mode
                c->a = 0;
                c->trig = 1;
                
                //  set new loop point
                c->head[0].end = c->loop;
                
                //  write to buffer with audio thru
                return buffer_head_rec(&(c->head[0]), c->input(c));
            }
            else
            {
                //  setup playback mode
                c->trig = 1;
                
                //  set new loop point
                c->head[0].end = c->loop;
                
                //  write to buffer with audio thru
                return buffer_head_rec(&(c->head[0]), c->input(c));
            }
        }
    }
    //  arm
    else if (c->a == 2)
    {
        //  prepare new loop point
        c->loop = c->head[0].start;
        
        //  return first sample
        if (c->trig)
        {
            c->trig = 0;
            c->head[0].idx = c->head[0].start;
            return buffer_head_play(&(c->head[0]));
        }
        //  return next sample
        else
        {
            c->head[0].idx += 1;
            if (c->head[0].idx >= c->head[0].end)
            {
                c->head[0].idx = c->head[0].start;
                if (c->send_retrig) module_send_retrig();
            }
            return buffer_head_play(&(c->head[0]));
        }
    }
    else return 0;
}

fract32 pf_pre(prgmChn *c) {
    //  return input
    return c->cvframe;
}

fract32 pf_bpf(prgmChn *c) {
    //  parameter a sets cutoff frequency with cv modulation
    //  parameter b sets feedback
    fract32 mod;

    filter_1p_lo_in(&(c->pSlew), c->a);
    
    if (filter_1p_sync(&(c->pSlew)))
    {
        mod = add_fr1x32(c->a, mult_fr1x32x32(cv[c->amod], c->amodl));
    }
    else
    {
        mod = add_fr1x32(filter_1p_lo_next(&(c->pSlew)), mult_fr1x32x32(cv[c->amod], c->amodl));
    }
    
    c->c0 = add_fr1x32(c->c0^2, mult_fr1x32x32(mod, c->c2));
    c->c1 = sub_fr1x32(sub_fr1x32(c->input(c), mult_fr1x32x32(c->b, c->c2)), c->c0^2);
    c->c2 = add_fr1x32(c->c2, mult_fr1x32x32(mod, c->c1^4));
   
    c->c0 = add_fr1x32(c->c0^2, mult_fr1x32x32(mod, c->c2));
    c->c1 = sub_fr1x32(sub_fr1x32(c->c2, mult_fr1x32x32(c->b, c->c2)), c->c0^2);
    return c->c2 = add_fr1x32(c->c2, mult_fr1x32x32(mod, c->c1^4));
}

fract32 pf_dly(prgmChn *c) {
    //  parameter a sets feedback level with cv modulation
    //  parameter b sets delay time
    
    fract32 mod = mult_fr1x32x32(cv[c->amod], c->amodl);
    
    //  set loop
    c->head[0].end = c->b;
    c->head[1].end = c->b;
    
    //advance the phasors
    c->head[0].idx += 1;
    c->head[1].idx += 1;
    if (c->head[1].idx >= c->head[1].end)
    {
        c->head[0].idx = c->head[0].start;
        c->head[1].idx = c->head[1].start;
    }
    
    //  return delay with modulated feedback level
    buffer_head_mix(&(c->head[0]), c->input(c), add_fr1x32(c->a, mod));
    return buffer_head_play(&(c->head[1]));
}

fract32 pf_rpt(prgmChn *c) {

    //advance the phasors
    c->head[0].idx += 1;
    c->head[1].idx += 1;
    
    if (c->head[1].idx >= c->head[1].end)
    {
        c->head[0].idx = c->head[0].start;
        c->head[1].idx = c->head[1].start;
    }
    
    return buffer_head_play(&(c->head[1]));
}

fract32 pf_mute(prgmChn *c) {
    fract32 tmp = 0; //c->frame;
    
    if (tmp > -0xf && tmp < 0xf)
    {
        c->hold = tmp;
        //  hold mute
        c->process = set_process(12);
        return tmp;
    }
    else return tmp;
}

fract32 pf_holdmute(prgmChn *c) {
    return c->hold;
}

fract32 pf_unmute(prgmChn *c) {
    //  frame refresh?!?!?
    fract32 tmp = 0; //c->frame; //envAmp.curve(&(c->envAmp), 0, 0);
    
    return tmp;
    /*
    filter_1p_lo_in(&(c->pSlew), tmp);
    
    if (filter_1p_sync(&(c->pSlew)))
    {
        c->process = set_process(c->flag);
        return tmp;
    }
    else
    {
        return c->hold;
        env_tcd_set_a(&(c->envAmp), filter_1p_lo_next(&(c->pSlew)));
        return c->envAmp.curve(&(c->envAmp), 0, 0);
    }
    */
}


//  outputs
fract32 out_off(void) {
    return 0;
}

fract32 out_vout0(void) {
    return vout[0];
}

fract32 out_vout1(void) {
    return vout[1];
}

fract32 out_vout2(void) {
    return vout[2];
}

fract32 out_vout3(void) {
    return vout[3];
}

fract32 out_vout4(void) {
    return vout[4];
}

fract32 out_vout5(void) {
    return vout[5];
}

fract32 out_aux(void) {
    return aux;
}
