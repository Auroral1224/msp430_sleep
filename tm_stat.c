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

#include "tinymaix.h"

#if TM_ENABLE_STAT
static const char* mdl_type_str[6] = {
    "int8",   
    "int16",     
    "fp32",    
    "fp16",  
    "fp8 1.4.3", 
    "fp8 1.5.2", 
};

static const char* tml_str_tbl[TML_MAXCNT] = {
    "Conv2D",   /*TML_CONV2D  = 0,*/
    "GAP",      /*TML_GAP     = 1,*/
    "FC",       /*TML_FC      = 2,*/
    "Softmax",  /*TML_SOFTMAX = 3,*/
    "Reshape",  /*TML_RESHAPE = 4,*/
    "DWConv2D", /*TML_DWCONV2D= 5,*/
    "ADD",      /*TML_ADD     = 6,*/
};

static const int tml_headsize_tbl[TML_MAXCNT] = {
    sizeof(tml_conv2d_dw_t),
    sizeof(tml_gap_t),
    sizeof(tml_fc_t),
    sizeof(tml_softmax_t),
    sizeof(tml_reshape_t),
    sizeof(tml_conv2d_dw_t),
    sizeof(tml_add_t),
};

//tm_err_t tm_stat(tm_mdlbin_t* b)
//{
//    printf("================================ model stat ================================\n");
//    printf("mdl_type=%d (%s))\n", b->mdl_type, mdl_type_str[b->mdl_type]);
//    printf("out_deq=%d \n", b->out_deq);
//    printf("input_cnt=%d, output_cnt=%d, layer_cnt=%d\n", b->input_cnt, b->output_cnt, b->layer_cnt);
//    uint16_t* idim = b->in_dims;
//    printf("input %ddims: (%d, %d, %d)\n", idim[0],idim[1],idim[2],idim[3]);
//    uint16_t* odim = b->out_dims;
//    printf("output %ddims: (%d, %d, %d)\n", odim[0],odim[1],odim[2],odim[3]);
//    //printf("model param bin addr: 0x%x\n", (uint32_t)(b->layers_body));
//    printf("main buf size %ld; sub buf size %ld\n", \
//        b->buf_size,b->sub_size);
//
//    printf("//Note: PARAM is layer param size, include align padding\r\n\r\n");
//    printf("Idx\tLayer\t         outshape\tinoft\toutoft\tPARAM\tMEMOUT OPS\n");
//    printf("---\tInput    \t%3d,%3d,%3d\t-   \t0    \t0 \t%lld \t0\n",\
//        idim[1],idim[2],idim[3], (uint64_t)(idim[1]*idim[2]*idim[3]*sizeof(mtype_t)));
//    //      000  Input    -     224,224,3  0x40001234 0x40004000 100000 500000 200000
//    //printf("000  Input    -     %3d,%3d,%d  0x%08x   0x%08x     %6d %6d %6d\n",)
//    uint32_t sum_param = 0;
//    uint32_t sum_ops   = 0;
//    uint8_t*layer_body  = (uint8_t*)b->layers_body;
//    uint8_t layer_i;
//    for(layer_i = 0; layer_i < b->layer_cnt; layer_i++){
//        tml_head_t* h = (tml_head_t*)(layer_body);
//        TM_DBG("body oft = %ld\n", (uint32_t)((size_t)h - (size_t)(b)));
//        TM_DBG("type=%d, is_out=%d, size=%ld, in_oft=%ld, out_oft=%ld, in_dims=[%d,%d,%d,%d], out_dims=[%d,%d,%d,%d], in_s=%.3f, in_zp=%ld, out_s=%.3f, out_zp=%ld\n",\
//                h->type,h->is_out,h->size,h->in_oft,h->out_oft,\
//                h->in_dims[0],h->in_dims[1],h->in_dims[2],h->in_dims[3],\
//                h->out_dims[0],h->out_dims[1],h->out_dims[2],h->out_dims[3],\
//                h->in_s,(int32_t)(h->in_zp),h->out_s,(int32_t)(h->out_zp));
//        if(h->type < TML_MAXCNT) {
//            uint32_t memout = h->out_dims[1]*h->out_dims[2]*h->out_dims[3];
//            sum_param += (h->size - tml_headsize_tbl[h->type]);
//            uint32_t ops = 0;
//            switch(h->type){
//            case TML_CONV2D: {
//                tml_conv2d_dw_t* l = (tml_conv2d_dw_t*)(layer_body);
//                ops = memout*(l->kernel_w)*(l->kernel_h)*(h->in_dims[3]);   //MAC as ops
//                TM_DBG("Conv2d: kw=%d, kh=%d, sw=%d, sh=%d, dw=%d, dh=%d, act=%d, pad=[%d,%d,%d,%d], dmul=%ld, ws_oft=%ld, w_oft=%ld, b_oft=%ld\n",\
//                    l->kernel_w, l->kernel_h, l->stride_w, l->stride_h, l->dilation_w, l->dilation_h, \
//                    l->act, l->pad[0], l->pad[1], l->pad[2], l->pad[3], l->depth_mul, \
//                    l->ws_oft, l->w_oft, l->b_oft);
//                break;}
//            case TML_GAP:
//                ops = (h->in_dims[1])*(h->in_dims[2])*(h->in_dims[3]);  //SUM as ops
//                break;
//            case TML_FC: {
//                tml_fc_t* l = (tml_fc_t*)(layer_body);
//                ops = (h->out_dims[3])*(h->in_dims[3]);         //MAC as ops
//                TM_DBG("FC: ws_oft=%ld, w_oft=%ld, b_oft=%ld\n",\
//                    l->ws_oft, l->w_oft, l->b_oft);
//                break;}
//            case TML_SOFTMAX:
//                ops = 6*(h->out_dims[3]);                       //mixed
//                break;
//            case TML_DWCONV2D: {
//                tml_conv2d_dw_t* l = (tml_conv2d_dw_t*)(layer_body);
//                ops = memout*(l->kernel_w)*(l->kernel_h)*1;   //MAC as ops
//                TM_DBG("DWConv2d: kw=%d, kh=%d, sw=%d, sh=%d, dw=%d, dh=%d, act=%d, pad=[%d,%d,%d,%d], dmul=%ld, ws_oft=%ld, w_oft=%ld, b_oft=%ld\n",\
//                    l->kernel_w, l->kernel_h, l->stride_w, l->stride_h, l->dilation_w, l->dilation_h, \
//                    l->act, l->pad[0], l->pad[1], l->pad[2], l->pad[3], l->depth_mul,\
//                    l->ws_oft, l->w_oft, l->b_oft);
//                break;}
//            default:
//                ops = 0;
//                break;
//            }
//            sum_ops += ops;
//            printf("%03d\t%s      \t%3d,%3d,%3d\t%ld\t%ld\t%ld\t%lld\t", layer_i, tml_str_tbl[h->type], \
//                h->out_dims[1], h->out_dims[2], h->out_dims[3], \
//                h->in_oft, h->out_oft, h->size - tml_headsize_tbl[h->type], \
//                (uint64_t)(memout*sizeof(mtype_t)));
//            printf("%ld\r\n", ops);
//        } else {
//            return TM_ERR_LAYERTYPE;
//        }
//        if ((size_t)(h) < (size_t)(b)) {
//            printf("error: address calculate overflow.\n");
//            return TM_ERR;
//        }
//        layer_body += (h->size);
//    }
//    printf("\r\nTotal param ~%.1f KB, OPS ~%.2f MOPS, buffer %.1f KB\r\n\r\n", \
//        sum_param/1024.0, sum_ops/1000000.0, (b->buf_size + b->sub_size)/1024.0);
//    return TM_OK;
//}
tm_err_t tm_stat(tm_mdlbin_t* b)
{   
    printf("================================ model stat ================================\n");
    printf("mdl_type=%d (%s))\n", b->mdl_type, mdl_type_str[b->mdl_type]);
    printf("out_deq=%d \n", b->out_deq);
    printf("input_cnt=%d, output_cnt=%d, layer_cnt=%d\n", b->input_cnt, b->output_cnt, b->layer_cnt);
    uint16_t* idim = b->in_dims;
    printf("input %ddims: (%d, %d, %d)\n", idim[0],idim[1],idim[2],idim[3]);
    uint16_t* odim = b->out_dims;
    printf("output %ddims: (%d, %d, %d)\n", odim[0],odim[1],odim[2],odim[3]);
    printf("main buf size %ld; sub buf size %ld\n", b->buf_size, b->sub_size);

    printf("//Note: PARAM is layer param size, include align padding\r\n\r\n");
    printf("Idx\tLayer\t         outshape\tinoft\toutoft\tPARAM\tMEMOUT\tMAC\n");
    printf("---\tInput    \t%3d,%3d,%3d\t-   \t0    \t0 \t%llu \t0\n",
        idim[1],idim[2],idim[3], (uint64_t)(idim[1])*idim[2]*idim[3]*sizeof(mtype_t));

    uint64_t sum_param = 0;
    uint64_t sum_ops   = 0;
    uint8_t* layer_body = (uint8_t*)b->layers_body;
    uint8_t layer_i;
    for(layer_i = 0; layer_i < b->layer_cnt; layer_i++){
        tml_head_t* h = (tml_head_t*)(layer_body);
        TM_DBG("body oft = %ld\n", (uint32_t)((size_t)h - (size_t)(b)));
        TM_DBG("type=%d, is_out=%d, size=%ld, in_oft=%ld, out_oft=%ld, in_dims=[%d,%d,%d,%d], out_dims=[%d,%d,%d,%d], in_s=%.3f, in_zp=%ld, out_s=%.3f, out_zp=%ld\n",
                h->type,h->is_out,h->size,h->in_oft,h->out_oft,
                h->in_dims[0],h->in_dims[1],h->in_dims[2],h->in_dims[3],
                h->out_dims[0],h->out_dims[1],h->out_dims[2],h->out_dims[3],
                h->in_s,(int32_t)(h->in_zp),h->out_s,(int32_t)(h->out_zp));
        if(h->type < TML_MAXCNT) {
            uint64_t memout = (uint64_t)h->out_dims[1] * h->out_dims[2] * h->out_dims[3];
            sum_param += (h->size - tml_headsize_tbl[h->type]);
            uint64_t ops = 0;
            switch(h->type){
            case TML_CONV2D: {
                tml_conv2d_dw_t* l = (tml_conv2d_dw_t*)(layer_body);
                ops = memout * l->kernel_w * l->kernel_h * h->in_dims[3];
                TM_DBG("Conv2d: kw=%d, kh=%d, sw=%d, sh=%d, dw=%d, dh=%d, act=%d, pad=[%d,%d,%d,%d], dmul=%ld, ws_oft=%ld, w_oft=%ld, b_oft=%ld\n",
                    l->kernel_w, l->kernel_h, l->stride_w, l->stride_h, l->dilation_w, l->dilation_h,
                    l->act, l->pad[0], l->pad[1], l->pad[2], l->pad[3], l->depth_mul,
                    l->ws_oft, l->w_oft, l->b_oft);
                break;}
            case TML_GAP:
                ops = (uint64_t)h->in_dims[1] * h->in_dims[2] * h->in_dims[3];
                break;
            case TML_FC: {
                tml_fc_t* l = (tml_fc_t*)(layer_body);
                ops = (uint64_t)h->out_dims[3] * h->in_dims[3];
                TM_DBG("FC: ws_oft=%ld, w_oft=%ld, b_oft=%ld\n",
                    l->ws_oft, l->w_oft, l->b_oft);
                break;}
            case TML_SOFTMAX:
                ops = 6 * (uint64_t)h->out_dims[3];
                break;
            case TML_DWCONV2D: {
                tml_conv2d_dw_t* l = (tml_conv2d_dw_t*)(layer_body);
                ops = memout * l->kernel_w * l->kernel_h;
                TM_DBG("DWConv2d: kw=%d, kh=%d, sw=%d, sh=%d, dw=%d, dh=%d, act=%d, pad=[%d,%d,%d,%d], dmul=%ld, ws_oft=%ld, w_oft=%ld, b_oft=%ld\n",
                    l->kernel_w, l->kernel_h, l->stride_w, l->stride_h, l->dilation_w, l->dilation_h,
                    l->act, l->pad[0], l->pad[1], l->pad[2], l->pad[3], l->depth_mul,
                    l->ws_oft, l->w_oft, l->b_oft);
                break;}
            default:
                ops = 0;
                break;
            }
            sum_ops += ops;
            printf("%03d\t%-10s\t%3d,%3d,%3d\t%ld\t%ld\t%ld\t%llu\t%llu\n", layer_i, tml_str_tbl[h->type],
                h->out_dims[1], h->out_dims[2], h->out_dims[3],
                h->in_oft, h->out_oft, h->size - tml_headsize_tbl[h->type],
                (uint64_t)(memout*sizeof(mtype_t)), ops);
        } else {
            return TM_ERR_LAYERTYPE;
        }
        if ((size_t)(h) < (size_t)(b)) {
            printf("error: address calculate overflow.\n");
            return TM_ERR;
        }
        layer_body += (h->size);
    }
    printf("\nTotal param ~%.1f KB, MAC ~%.2fM, OPS ~%.2f M, buffer %.1f KB\n\n",
        sum_param/1024.0,sum_ops/1000000.0, sum_ops*2/1000000.0, (b->buf_size + b->sub_size)/1024.0);
    return TM_OK;
}


#endif
