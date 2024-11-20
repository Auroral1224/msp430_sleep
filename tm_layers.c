/* Copyright 2022 Sipeed Technology Co., Ltd. All Rights Reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
// It is default O0 implement
#include "tinymaix.h"
#include "float.h"
#include "math.h"

#if TM_OPT_LEVEL == TM_OPT0

#if TM_ARCH==TM_ARCH_CPU
    #include "arch_cpu.h"
#elif TM_ARCH==TM_ARCH_ARM_SIMD
    #include "arch_arm_simd.h"
#elif TM_ARCH==TM_ARCH_ARM_NEON
    #include "arch_arm_neon.h"
#elif TM_ARCH==TM_ARCH_ARM_MVEI
    #include "arch_arm_mvei.h"
#elif TM_ARCH==TM_ARCH_RV32P
    #include "arch_rv32p.h"
#elif TM_ARCH==TM_ARCH_RV64V
    #include "arch_rv64v.h"
#elif TM_ARCH==TM_ARCH_CSKYV2
    #include "arch_cskyv2.h"
#elif TM_ARCH==TM_ARCH_X86_SSE2
    #include "arch_x86_sse2.h"
#else
    #error "UNSUPPORT ARCH!"
#endif


TM_PERF_REG(t_sbuf);TM_PERF_REG(t_dotp);TM_PERF_REG(t_post); 
TM_PERF_REG(t_valid); TM_PERF_REG(t_pad); 
TM_PERF_REG(t_conv); TM_PERF_REG(t_pwconv); TM_PERF_REG(t_dwconv); 

///*************************** TML_CONV2D **********************************/
//static uint32_t k_oft[TM_MAX_KSIZE];
//static mtype_t sbuf[TM_MAX_KCSIZE];
//#if (TM_MDL_TYPE==TM_MDL_FP32) || (TM_MDL_TYPE==TM_MDL_FP16)
//#define SUMSCALE NULL
//#define OUTSCALE outscale
//
//#elif (TM_MDL_TYPE==TM_MDL_INT8) || (TM_MDL_TYPE==TM_MDL_INT16)
//
//#if TM_FASTSCALE
//    static int32_t sumscale[TM_MAX_CSIZE];
//    #define OUTSCALE outscale
//#else
//    static float sumscale[TM_MAX_CSIZE];
//    #define OUTSCALE outscale_inv
//#endif
//#define SUMSCALE (sumscale + c)
//#endif
//
////for valid or kernel in valid part, use fast method
//tm_err_t TM_WEAK tml_conv2d_dwconv2d(tm_mat_t* in, tm_mat_t* out, wtype_t* w, btype_t* b, \
//	uint8_t kw, uint8_t kh, uint8_t sx, uint8_t sy, uint8_t dx, uint8_t dy, uint16_t act, \
//	uint8_t pad_top, uint8_t pad_bottom, uint8_t pad_left, uint8_t pad_right, uint32_t dmul, \
//    sctype_t* ws, sctype_t in_s, zptype_t in_zp, sctype_t out_s, zptype_t out_zp) //kernel: (cho, chi, h, w)
//{   TM_PERF_INIT(t_sbuf);TM_PERF_INIT(t_dotp);TM_PERF_INIT(t_post);
//    TM_PERF_INIT(t_valid);TM_PERF_INIT(t_pad);
//    TM_PERF_INIT(t_conv); TM_PERF_INIT(t_pwconv); TM_PERF_INIT(t_dwconv);
//    uint8_t pad_flag = (pad_top != 0 ||pad_bottom != 0 ||pad_left != 0 ||pad_right != 0);
//    if(dx!=1 || dy!= 1) return TM_ERR_TODO;
//    if(act >= TM_ACT_MAXCNT) return TM_ERR_UNSUPPORT;
//    uint16_t maxk = kw*kh;
//    if(maxk>TM_MAX_KSIZE) return TM_ERR_KSIZE;
//    if(maxk==1 && (pad_flag||dmul)) return TM_ERR_UNSUPPORT;   //assume no pad or dwconv when pwconv
//    uint16_t chi  = in->c;
//    uint16_t cho  = out->c;
//    sumtype_t sum = 0;
//    mtype_t* outp = out->data;
//
//#if (TM_MDL_TYPE == TM_MDL_INT8) || (TM_MDL_TYPE == TM_MDL_INT16)
//#if TM_FASTSCALE
//	int32_t outscale = (1<<TM_FASTSCALE_SHIFT)/out_s;
//	for(uint16_t c=0; c<out->c;c++) sumscale[c]=1.0/ws[c]/in_s;
//#else
//
//	sctype_t outscale = out_s;
//#ifdef USE_FIXED_POINT
//	sctype_t outscale_inv = _IQtoF(_IQdiv(_IQ(1.0f), _IQ(outscale)));
//	for(uint16_t c=0; c<out->c;c++) sumscale[c] = _IQtoF(_IQmpy(_IQ(ws[c]), _IQ(in_s)));
//#else
//	sctype_t outscale_inv = 1.f / outscale;
//	for(uint16_t c=0; c<out->c;c++) sumscale[c]=ws[c]*in_s;
//#endif
//
//#endif
//#else
//	sctype_t outscale = out_s;
//#endif
//
//    if(maxk==1){ TM_PERF_START(t_pwconv);   //pointwise conv
//        #define BATCH_SIZE 2
//        sumtype_t sums[BATCH_SIZE];
//        for (uint16_t y = 0; y < out->h; y++) {
//            for (uint16_t x = 0; x < out->w; x++) {
//                mtype_t* sptr = (mtype_t*)TM_MATP(in, sy*y, sx*x, 0);
//                wtype_t* kptr = (wtype_t*)w;
//                uint16_t c = 0;
//                for(; c<out->c-BATCH_SIZE+1; ){
//                    for(uint16_t bat = 0; bat < BATCH_SIZE; bat+=2)
//                        tm_dot_prod_pack2(sptr, kptr + chi*bat, chi, sums + bat);
//                    tm_postprocess_sum(BATCH_SIZE, sums, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp);
//                    c += BATCH_SIZE;
//                    outp += BATCH_SIZE;
//                    kptr += chi*BATCH_SIZE;//*2;
//                }
//                for(; c<out->c; c++){
//                    tm_dot_prod(sptr, kptr, chi, &sum); //size=maxk*chi //pw maxk==1
//                    tm_postprocess_sum(1, &sum, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp); outp++;
//                    kptr += chi;
//                }
//            }
//        }
//        TM_PERF_ADD(t_pwconv);
//        printf("pwconv\n");
//        return TM_OK;
//    }
//
//
//    if(dmul) {TM_PERF_START(t_dwconv);} else {TM_PERF_START(t_conv);};
//    int32_t oft = 0;
//    int32_t idx = 0;
//    for(int32_t y=0; y<kh; y++){    //gen k_oft table
//        for(int32_t x=0; x<kw; x++){
//            k_oft[idx] = oft;
//            idx += 1;
//            oft += chi;
//        }
//        oft += (in->w - kw)*chi;
//    }
//    chi  = dmul ? 1 : in->c; // dmul>=1 indicate depthwise; dummy chi for dwconv compatible
//    int32_t slow_flag = 0; //same pad part is slow
//    for (int32_t y = 0; y < out->h; y++) {
//        int32_t src_y0 = sy*y - pad_top;
//        for (int32_t x = 0; x < out->w; x++) {
//            int32_t src_x0 = sx*x - pad_left;
//            sumtype_t sum;
//            slow_flag = ((src_y0<0)+(src_x0<0)+(src_y0+kh>in->h)+(src_x0+kw>in->w));
//            //TM_PERF_START(t_sbuf);
//            if(!slow_flag) {TM_PERF_START(t_valid); //valid or same valid part
//                mtype_t* sptr_base = (mtype_t*)TM_MATP(in, src_y0, src_x0, 0); //?c/dmul:0
//                mtype_t* sptr = sptr_base; //= (mtype_t*)TM_MATP(in, src_y0, src_x0, 0); //sbuf 不变
//                uint32_t sidx=0;    //sbuf:cho,chi,maxk //dw:chi==1;
//                for (int32_t cc = 0; cc < (dmul?cho:chi); cc++) {
//                    for (int32_t k = 0; k < maxk; k++) {
//                        sbuf[sidx+k] = sptr[k_oft[k]];
//                    }
//                    sidx += maxk;
//                    sptr = sptr_base + (dmul?(cc+1)/dmul:(cc+1));
//                }
//            } else {  TM_PERF_START(t_pad);       //same pad part
//                int32_t _ky0 = src_y0<0 ? -src_y0 : 0;
//                int32_t _kx0 = src_x0<0 ? -src_x0 : 0;
//                int32_t _ky1 = in->h-src_y0>kh ? kh : in->h-src_y0;
//                int32_t _kx1 = in->w-src_x0>kw ? kw : in->w-src_x0;
//                uint32_t sidx=0;    //sbuf:cho,chi,maxk //dw:chi==1;
////                uint32_t s_step = (_ky1-_ky0)*(_kx1-_kx0);
//                mtype_t* sptr_base = (mtype_t*)TM_MATP(in, src_y0, src_x0, 0);
//                mtype_t* sptr = sptr_base;
//            #if TM_MDL_TYPE == TM_MDL_INT8
//                memset(sbuf, in_zp, dmul?cho*maxk:chi*maxk);    //do padding
//            #elif (TM_MDL_TYPE == TM_MDL_FP32)||(TM_MDL_TYPE == TM_MDL_FP16)||(TM_MDL_TYPE == TM_MDL_FP8_143)||(TM_MDL_TYPE == TM_MDL_FP8_152)
//                memset(sbuf, 0, (dmul?cho*maxk:chi*maxk)*sizeof(mtype_t));
//            #else
//            #error "unsupport mdl type"
//            #endif
//                for (int32_t cc = 0; cc < (dmul?cho:chi); cc++) {
//                    for(int32_t _ky=_ky0; _ky<_ky1; _ky++){
//                        for(int32_t _kx=_kx0; _kx<_kx1; _kx++){
//                            int32_t k = _ky*kw + _kx;
//                            sbuf[sidx+k] = sptr[k_oft[k]];
//                        }
//                    }
//                    sidx += maxk;
//                    sptr = sptr_base + (dmul?(cc+1)/dmul:(cc+1));
//                }
//            }
//            //TM_PERF_ADD(t_sbuf);
//            mtype_t* sptr = sbuf;    //sbuf prepare ok~
//            if(maxk*chi==9 && dmul){ //simple opt for 3x3 dwconv
//                for(int32_t c=0; c<out->c; c++){
//                    wtype_t* kptr = (wtype_t*)w + c*chi*maxk;//TM_PERF_START(t_dotp);
//                    tm_dot_prod_3x3x1(sptr, kptr, &sum);//TM_PERF_ADD(t_dotp);TM_PERF_START(t_post);
//                    tm_postprocess_sum(1, &sum, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp); outp++;//TM_PERF_ADD(t_post);
//                    sptr += maxk; //dwconv need move step
//                }
//            }else {
//                for(int32_t c=0; c<out->c; c++){
//                    wtype_t* kptr = (wtype_t*)w + c*chi*maxk;//TM_PERF_START(t_dotp);
//                    tm_dot_prod(sptr, kptr, maxk*chi, &sum);//TM_PERF_ADD(t_dotp);TM_PERF_START(t_post);
//                    tm_postprocess_sum(1, &sum, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp); outp++;//TM_PERF_ADD(t_post);
//                    if(dmul) sptr += maxk; //dwconv need move step
//                }
//            }
//            if(!slow_flag) {TM_PERF_ADD(t_valid);} else {TM_PERF_ADD(t_pad);}
//        }
//    }
//    if(dmul) {TM_PERF_ADD(t_dwconv);} else {TM_PERF_ADD(t_conv);};
///*
//    if(dmul) {TM_PERF_START(t_dwconv);} else {TM_PERF_START(t_conv);};
//    uint32_t oft = 0;
//    uint16_t idx = 0;
//    for(uint8_t y=0; y<kh; y++){    //gen k_oft table
//        for(uint8_t x=0; x<kw; x++){
//            k_oft[idx] = oft;
//            idx += 1;
//            oft += chi;
//        }
//        oft += (in->w - kw)*chi;
//    }
//    chi  = dmul ? 1 : in->c; // dmul>=1 indicate depthwise; dummy chi for dwconv compatible
//    uint8_t slow_flag = 0; //same pad part is slow
//    for (uint16_t y = 0; y < out->h; y++) {
//    	int32_t src_y0 = sy*y - pad_top;
//        for (uint16_t x = 0; x < out->w; x++) {
//        	int32_t src_x0 = sx*x - pad_left;
//            sumtype_t sum;
//            slow_flag = ((src_y0<0)+(src_x0<0)+(src_y0+kh>in->h)+(src_x0+kw>in->w));
//            //TM_PERF_START(t_sbuf);
//            if(!slow_flag) {TM_PERF_START(t_valid); //valid or same valid part
//                mtype_t* sptr_base = (mtype_t*)TM_MATP(in, src_y0, src_x0, 0); //?c/dmul:0
//                mtype_t* sptr = sptr_base; //= (mtype_t*)TM_MATP(in, src_y0, src_x0, 0); //sbuf 荳榊序
//                uint32_t sidx=0;    //sbuf:cho,chi,maxk //dw:chi==1;
//                for (uint16_t cc = 0; cc < (dmul?cho:chi); cc++) {
//                    for (uint16_t k = 0; k < maxk; k++) {
//                        sbuf[sidx+k] = sptr[k_oft[k]];
//                    }
//                    sidx += maxk;
//                    sptr = sptr_base + (dmul?(cc+1)/dmul:(cc+1));
//                }
//            } else {  TM_PERF_START(t_pad);       //same pad part
//             // FIXME: padding part
//				int32_t _ky0 = src_y0<0 ? -src_y0 : 0;
//				int32_t _kx0 = src_x0<0 ? -src_x0 : 0;
//				int32_t _ky1 = in->h-src_y0>kh ? kh : in->h-src_y0;
//				int32_t _kx1 = in->w-src_x0>kw ? kw : in->w-src_x0;
////				printf("%d, %d, %d, %d\n", _ky0, _kx0, _ky1, _kx1);
//                uint32_t sidx=0;    //sbuf:cho,chi,maxk //dw:chi==1;
////                uint32_t s_step = (_ky1-_ky0)*(_kx1-_kx0);
//                mtype_t* sptr_base = (mtype_t*)TM_MATP(in, src_y0, src_x0, 0);
//                mtype_t* sptr = sptr_base;
//            #if TM_MDL_TYPE == TM_MDL_INT8
//                memset(sbuf, (int8_t)in_zp, dmul?cho*maxk:chi*maxk);    //do padding
//            #elif (TM_MDL_TYPE == TM_MDL_FP32)||(TM_MDL_TYPE == TM_MDL_FP16)||(TM_MDL_TYPE == TM_MDL_FP8_143)||(TM_MDL_TYPE == TM_MDL_FP8_152)
//                memset(sbuf, 0, (dmul?cho*maxk:chi*maxk)*sizeof(mtype_t));
//            #else
//            #error "unsupport mdl type"
//            #endif
//                for (uint16_t cc = 0; cc < (dmul?cho:chi); cc++) {
//                    for(int32_t _ky=_ky0; _ky<_ky1; _ky++){
//                        for(int32_t _kx=_kx0; _kx<_kx1; _kx++){
//                        	int64_t k = _ky*kw + _kx;
//                            sbuf[sidx+k] = sptr[k_oft[k]];
////                            printf("%d ", sbuf[sidx+k]);
//                        }
//                    }
////                    printf("\n");
//                    sidx += maxk;
//                    sptr = sptr_base + (dmul?(cc+1)/dmul:(cc+1));
////                    printf("%x\n", sptr);
//                }
//            }
//
//
//            //TM_PERF_ADD(t_sbuf);
//            mtype_t* sptr = sbuf;    //sbuf prepare ok~
//            if(maxk*chi==9 && dmul){ //simple opt for 3x3 dwconv
//				for(uint16_t c=0; c<out->c; c++){
//                    wtype_t* kptr = (wtype_t*)w + c*chi*maxk;//TM_PERF_START(t_dotp);
//                    tm_dot_prod_3x3x1(sptr, kptr, &sum);//TM_PERF_ADD(t_dotp);TM_PERF_START(t_post);
//                    tm_postprocess_sum(1, &sum, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp); outp++;//TM_PERF_ADD(t_post);
//                    sptr += maxk; //dwconv need move step
//                }
//            }else {
//                for(uint16_t c=0; c<out->c; c++){
//                    wtype_t* kptr = (wtype_t*)w + c*chi*maxk;//TM_PERF_START(t_dotp);
//                    tm_dot_prod(sptr, kptr, maxk*chi, &sum);//TM_PERF_ADD(t_dotp);TM_PERF_START(t_post);
//                    tm_postprocess_sum(1, &sum, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp); outp++;//TM_PERF_ADD(t_post);
//                    if(dmul) sptr += maxk; //dwconv need move step
//                }
//            }
//            if(!slow_flag) {TM_PERF_ADD(t_valid);} else {TM_PERF_ADD(t_pad);}
//        }
//    }
//    if(dmul) {TM_PERF_ADD(t_dwconv);} else {TM_PERF_ADD(t_conv);};
//*/
//    return TM_OK;
//}
/*************************** TML_CONV2D **********************************/
TM_STATIC uint32_t k_oft[TM_MAX_KSIZE];
TM_STATIC mtype_t sbuf[TM_MAX_KCSIZE];
#if (TM_MDL_TYPE==TM_MDL_FP32) || (TM_MDL_TYPE==TM_MDL_FP16) 
#define SUMSCALE NULL
#define OUTSCALE outscale

#elif (TM_MDL_TYPE==TM_MDL_INT8) || (TM_MDL_TYPE==TM_MDL_INT16) 

#if TM_FASTSCALE
    TM_STATIC int32_t sumscale[TM_MAX_CSIZE];
    #define OUTSCALE outscale
#else
    TM_STATIC float sumscale[TM_MAX_CSIZE];
    #define OUTSCALE outscale_inv
#endif
#define SUMSCALE (sumscale + c)
#endif
 
//for valid or kernel in valid part, use fast method
tm_err_t TM_WEAK tml_conv2d_dwconv2d(tm_mat_t* in, tm_mat_t* out, wtype_t* w, btype_t* b, \
    uint8_t kw, uint8_t kh, uint8_t sx, uint8_t sy, uint8_t dx, uint8_t dy, uint16_t act, \
    uint8_t pad_top, uint8_t pad_bottom, uint8_t pad_left, uint8_t pad_right, uint32_t dmul, \
    sctype_t* ws, sctype_t in_s, zptype_t in_zp, sctype_t out_s, zptype_t out_zp) //kernel: (cho, chi, h, w)
{   TM_PERF_INIT(t_sbuf);TM_PERF_INIT(t_dotp);TM_PERF_INIT(t_post);
    TM_PERF_INIT(t_valid);TM_PERF_INIT(t_pad);
    TM_PERF_INIT(t_conv); TM_PERF_INIT(t_pwconv); TM_PERF_INIT(t_dwconv);
    int32_t pad_flag = (pad_top != 0 ||pad_bottom != 0 ||pad_left != 0 ||pad_right != 0);
    if(dx!=1 || dy!= 1) return TM_ERR_TODO;
    if(act >= TM_ACT_MAXCNT) return TM_ERR_UNSUPPORT;
    int32_t maxk = kw*kh;
    if(maxk>TM_MAX_KSIZE) return TM_ERR_KSIZE;
    if(maxk==1 && (pad_flag||dmul)) return TM_ERR_UNSUPPORT;   //assume no pad or dwconv when pwconv
    int32_t chi  = in->c;
    int32_t cho  = out->c;
    sumtype_t sum = 0;
    mtype_t* outp = out->data;

#if (TM_MDL_TYPE == TM_MDL_INT8) || (TM_MDL_TYPE == TM_MDL_INT16)
#if TM_FASTSCALE
	int32_t outscale = (1<<TM_FASTSCALE_SHIFT)/out_s;
	for(int32_t c=0; c<out->c;c++) sumscale[c]=1.0/ws[c]/in_s;
#else
	sctype_t outscale = out_s;
    sctype_t outscale_inv = 1.f / outscale;
	for(int32_t c=0; c<out->c;c++) sumscale[c]=ws[c]*in_s;
#endif
#else
	sctype_t outscale = out_s;
#endif

    if(maxk==1){ TM_PERF_START(t_pwconv);   //pointwise conv
        #define BATCH_SIZE 2
        sumtype_t sums[BATCH_SIZE];
        for (int32_t y = 0; y < out->h; y++) {
            for (int32_t x = 0; x < out->w; x++) {
                mtype_t* sptr = (mtype_t*)TM_MATP(in, sy*y, sx*x, 0);
                wtype_t* kptr = (wtype_t*)w;
                int32_t c = 0;
                for(; c<out->c-BATCH_SIZE+1; ){
                    for(int32_t bat = 0; bat < BATCH_SIZE; bat+=2)
                        tm_dot_prod_pack2(sptr, kptr + chi*bat, chi, sums + bat);
                    tm_postprocess_sum(BATCH_SIZE, sums, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp);
                    c += BATCH_SIZE;
                    outp += BATCH_SIZE;
                    kptr += chi*BATCH_SIZE;//*2;
                }
                for(; c<out->c; c++){
                    tm_dot_prod(sptr, kptr, chi, &sum); //size=maxk*chi //pw maxk==1
                    tm_postprocess_sum(1, &sum, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp); outp++;
                    kptr += chi;
                }
            }
        }
        TM_PERF_ADD(t_pwconv);
        return TM_OK;
    }

    if(dmul) {TM_PERF_START(t_dwconv);} else {TM_PERF_START(t_conv);};
    int32_t oft = 0;
    int32_t idx = 0;
    for(int32_t y=0; y<kh; y++){    //gen k_oft table
        for(int32_t x=0; x<kw; x++){
            k_oft[idx] = oft;
            idx += 1;
            oft += chi;
        }
        oft += (in->w - kw)*chi;
    }
    chi  = dmul ? 1 : in->c; // dmul>=1 indicate depthwise; dummy chi for dwconv compatible
    int32_t slow_flag = 0; //same pad part is slow
    for (int32_t y = 0; y < out->h; y++) {
        int32_t src_y0 = sy*y - pad_top;
        for (int32_t x = 0; x < out->w; x++) {
            int32_t src_x0 = sx*x - pad_left;
            sumtype_t sum;
            slow_flag = ((src_y0<0)+(src_x0<0)+(src_y0+kh>in->h)+(src_x0+kw>in->w));
            //TM_PERF_START(t_sbuf);
            if(!slow_flag) {TM_PERF_START(t_valid); //valid or same valid part
                mtype_t* sptr_base = (mtype_t*)TM_MATP(in, src_y0, src_x0, 0); //?c/dmul:0
                mtype_t* sptr = sptr_base; //= (mtype_t*)TM_MATP(in, src_y0, src_x0, 0); //sbuf ??
                uint32_t sidx=0;    //sbuf:cho,chi,maxk //dw:chi==1;
                for (int32_t cc = 0; cc < (dmul?cho:chi); cc++) {
                    for (int32_t k = 0; k < maxk; k++) {
                        sbuf[sidx+k] = sptr[k_oft[k]];
                    }
                    sidx += maxk;
                    sptr = sptr_base + (dmul?(cc+1)/dmul:(cc+1));
                }
            } else {  TM_PERF_START(t_pad);       //same pad part
                int32_t _ky0 = src_y0<0 ? -src_y0 : 0;
                int32_t _kx0 = src_x0<0 ? -src_x0 : 0;
                int32_t _ky1 = in->h-src_y0>kh ? kh : in->h-src_y0;
                int32_t _kx1 = in->w-src_x0>kw ? kw : in->w-src_x0;
                uint32_t sidx=0;    //sbuf:cho,chi,maxk //dw:chi==1;
                uint32_t s_step = (_ky1-_ky0)*(_kx1-_kx0);
                mtype_t* sptr_base = (mtype_t*)TM_MATP(in, src_y0, src_x0, 0);
                mtype_t* sptr = sptr_base;
            #if TM_MDL_TYPE == TM_MDL_INT8
                memset(sbuf, in_zp, dmul?cho*maxk:chi*maxk);    //do padding
            #elif (TM_MDL_TYPE == TM_MDL_FP32)||(TM_MDL_TYPE == TM_MDL_FP16)||(TM_MDL_TYPE == TM_MDL_FP8_143)||(TM_MDL_TYPE == TM_MDL_FP8_152)
                memset(sbuf, 0, (dmul?cho*maxk:chi*maxk)*sizeof(mtype_t));
            #else
            #error "unsupport mdl type"
            #endif
                for (int32_t cc = 0; cc < (dmul?cho:chi); cc++) {
                    for(int32_t _ky=_ky0; _ky<_ky1; _ky++){
                        for(int32_t _kx=_kx0; _kx<_kx1; _kx++){
                            int32_t k = _ky*kw + _kx;
                            sbuf[sidx+k] = sptr[k_oft[k]];
                        }
                    }
                    sidx += maxk;
                    sptr = sptr_base + (dmul?(cc+1)/dmul:(cc+1));
                }
            }
            //TM_PERF_ADD(t_sbuf);
            mtype_t* sptr = sbuf;    //sbuf prepare ok~
            if(maxk*chi==9 && dmul){ //simple opt for 3x3 dwconv
                for(int32_t c=0; c<out->c; c++){
                    wtype_t* kptr = (wtype_t*)w + c*chi*maxk;//TM_PERF_START(t_dotp);
                    tm_dot_prod_3x3x1(sptr, kptr, &sum);//TM_PERF_ADD(t_dotp);TM_PERF_START(t_post);
                    tm_postprocess_sum(1, &sum, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp); outp++;//TM_PERF_ADD(t_post);
                    sptr += maxk; //dwconv need move step
                }
            }else {
                for(int32_t c=0; c<out->c; c++){
                    wtype_t* kptr = (wtype_t*)w + c*chi*maxk;//TM_PERF_START(t_dotp);
                    tm_dot_prod(sptr, kptr, maxk*chi, &sum);//TM_PERF_ADD(t_dotp);TM_PERF_START(t_post);
                    tm_postprocess_sum(1, &sum, b + c, act, outp, SUMSCALE, OUTSCALE, out_zp); outp++;//TM_PERF_ADD(t_post);
                    if(dmul) sptr += maxk; //dwconv need move step
                }
            }
            if(!slow_flag) {TM_PERF_ADD(t_valid);} else {TM_PERF_ADD(t_pad);}
        }
    }
    if(dmul) {TM_PERF_ADD(t_dwconv);} else {TM_PERF_ADD(t_conv);};
    return TM_OK;
}

/*************************** TML_GAP **********************************/
tm_err_t TM_WEAK tml_gap(tm_mat_t* in, tm_mat_t* out, sctype_t in_s, zptype_t in_zp, sctype_t out_s, zptype_t out_zp)
{   TM_DBGT_INIT();
    mtype_t* data;
    for(int c=0; c <out->c; c++){
        sumtype_t sum = 0;
        data = in->data + c;
        for(int y=0; y <in->h; y++){
            for(int x=0; x <in->w; x++){
                sum  += ((sumtype_t)(*data));
                data += out->c;
            }
        }
    #if TM_MDL_TYPE == TM_MDL_INT8 || TM_MDL_TYPE == TM_MDL_INT16
        out->data[c] = (mtype_t)((sum/((in->h)*(in->w))-in_zp)*in_s/out_s + out_zp); //requant
    #elif TM_MDL_TYPE == TM_MDL_FP32 || TM_MDL_TYPE == TM_MDL_FP16
        out->data[c] = (mtype_t)(sum/((in->h)*(in->w)));
    //#else //#elif TM_MDL_TYPE == TM_MDL_FP8_143 || TM_MDL_TYPE == TM_MDL_FP8_152
    #endif
    }
    return TM_OK;
}

/*************************** TML_FC **********************************/
tm_err_t TM_WEAK tml_fc(tm_mat_t* in, tm_mat_t* out,  wtype_t* w, btype_t* b, \
    sctype_t* ws, sctype_t in_s, zptype_t in_zp, sctype_t out_s, zptype_t out_zp)
{   TM_DBGT_INIT();
    mtype_t* data = in->data;
    for(uint16_t c=0; c <out->c; c++){
        sumtype_t sum = 0;
        tm_dot_prod(data, w+c*in->c, in->c, &sum);
        sum += b[c];    //fuse with zp
    #if TM_MDL_TYPE == TM_MDL_INT8 || TM_MDL_TYPE == TM_MDL_INT16

#ifdef USE_FIXED_POINT
        out->data[c] = (mtype_t)(_IQtoF(_IQdiv(_IQmpy(_IQmpyI32(_IQ(in_s), sum), _IQ(ws[0])), _IQ(out_s))) + out_zp);
#else
        out->data[c] = (mtype_t)(sum*in_s*ws[0]/out_s + out_zp); //requant
#endif

    #else
        out->data[c] = (mtype_t)(sum);
    #endif
    }
    return TM_OK;
}

/*************************** TML_SOFTMAX **********************************/
tm_err_t TM_WEAK tml_softmax(tm_mat_t* in, tm_mat_t* out, sctype_t in_s, zptype_t in_zp, sctype_t out_s, zptype_t out_zp)
{   TM_DBGT_INIT(); //note we have float size output buf even in INT8/INT16 mode
    mtype_t* din = in->data;
    float*  dout = (float*)(out->data);
    float   dmax =  -FLT_MAX;
    for(int c=0; c <in->c; c++){
    #if TM_MDL_TYPE == TM_MDL_INT8 || TM_MDL_TYPE == TM_MDL_INT16
        dout[c] = (float)((sumtype_t)din[c] - in_zp)*in_s;
    #else
        dout[c] = din[c];
    #endif
        if(dout[c] > dmax) dmax = dout[c];
    }
    float sum = 0;
    for(uint16_t c=0; c <in->c; c++){
        dout[c] -= dmax;
        dout[c] = (float)tm_exp(dout[c]);
        sum     += dout[c];
        dout[c] -= 0.000001;  //prevent 1.0 value (cause 256 overflow)
    }
    for(uint16_t c=0; c <in->c; c++){  //int8/int16 <= fp32, so it is ok
    #if TM_MDL_TYPE == TM_MDL_INT8 || TM_MDL_TYPE == TM_MDL_INT16
        out->data[c] = (mtype_t)(dout[c]/sum/out_s + out_zp); //requant
    #else
        out->data[c] = (mtype_t)(dout[c]/sum);
    #endif
    }
    return TM_OK;
}

/*************************** TML_RESHAPE **********************************/
tm_err_t TM_WEAK tml_reshape(tm_mat_t* in, tm_mat_t* out, sctype_t in_s, zptype_t in_zp, sctype_t out_s, zptype_t out_zp)
{   
    //in fact do nothing... out shape
    return TM_OK;
}


tm_err_t TM_WEAK tml_add(tm_mat_t* in0, tm_mat_t* in1, tm_mat_t* out, \
    sctype_t in_s0, zptype_t in_zp0, sctype_t in_s1, zptype_t in_zp1, sctype_t out_s, zptype_t out_zp)
{   //TODO: check in0 shape == in1 shape 
    //It is simple and experimental implement for ADD, could be more way faster
    mtype_t* d0 = in0->data;
    mtype_t* d1 = in1->data;
    mtype_t* res = out->data; 
    int size = in0->h*in0->w*in0->c;
    TM_PRINTF("s0=%.3f,zp0=%ld; s1=%.3f,zp1=%ld\r\n", in_s0, in_zp0, in_s1, in_zp1);
#if TM_MDL_TYPE == TM_MDL_FP16 || TM_MDL_TYPE == TM_MDL_FP32 || TM_MDL_TYPE == TM_MDL_INT8
    int i;
    for(i=0; i+4<=size; ){
        res[i] = TM_QUANT(TM_DEQUANT(d0[i],in_s0,in_zp0)+TM_DEQUANT(d1[i],in_s1,in_zp1), out_s, out_zp); i++;
        res[i] = TM_QUANT(TM_DEQUANT(d0[i],in_s0,in_zp0)+TM_DEQUANT(d1[i],in_s1,in_zp1), out_s, out_zp); i++;
        res[i] = TM_QUANT(TM_DEQUANT(d0[i],in_s0,in_zp0)+TM_DEQUANT(d1[i],in_s1,in_zp1), out_s, out_zp); i++;
        res[i] = TM_QUANT(TM_DEQUANT(d0[i],in_s0,in_zp0)+TM_DEQUANT(d1[i],in_s1,in_zp1), out_s, out_zp); i++;
    }
    for(; i<size; i++){
        res[i] = TM_QUANT(TM_DEQUANT(d0[i],in_s0,in_zp0)+TM_DEQUANT(d1[i],in_s1,in_zp1), out_s, out_zp);
    }
#else
    #error "ADD not support this data type yet"
#endif
    return TM_OK;
}

#endif
