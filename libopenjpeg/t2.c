/*
 * Copyright (c) 2002-2007, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2007, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux and Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opj_includes.h"

/** @defgroup T2 T2 - Implementation of a tier-2 coding */
/*@{*/

/** @name Local static functions */
/*@{*/

static void t2_putcommacode(opj_bio_t *bio, int n);
static int t2_getcommacode(opj_bio_t *bio);
/**
Variable length code for signalling delta Zil (truncation point)
@param bio Bit Input/Output component
@param n delta Zil
*/
static void t2_putnumpasses(opj_bio_t *bio, int n);
static int t2_getnumpasses(opj_bio_t *bio);
/**
Encode a packet of a tile to a destination buffer
@param tile Tile for which to write the packets
@param tcp Tile coding parameters
@param pi Packet identity
@param dest Destination buffer
@param len Length of the destination buffer
@param cstr_info Codestream information structure 
@param tileno Number of the tile encoded
@return 
*/
static int t2_encode_packet(opj_tcd_tile_t *tile, opj_tcp_t *tcp, opj_pi_iterator_t *pi, unsigned char *dest, int len, opj_codestream_info_t *cstr_info, int tileno);

/**
Encode a packet of a tile to a destination buffer
@param tileno Number of the tile encoded
@param tile Tile for which to write the packets
@param tcp Tile coding parameters
@param pi Packet identity
@param dest Destination buffer
@param p_data_written   FIXME DOC
@param len Length of the destination buffer
@param cstr_info Codestream information structure
@return
*/
static opj_bool t2_encode_packet_v2(
                                                         OPJ_UINT32 tileno,
                                                         opj_tcd_tile_v2_t *tile,
                                                         opj_tcp_v2_t *tcp,
                                                         opj_pi_iterator_t *pi,
                                                         OPJ_BYTE *dest,
                                                         OPJ_UINT32 * p_data_written,
                                                         OPJ_UINT32 len,
                                                         opj_codestream_info_t *cstr_info);

/**
@param cblk
@param index
@param cblksty
@param first
*/
static opj_bool t2_init_seg(opj_tcd_cblk_dec_t* cblk, int index, int cblksty, int first);
/**
Decode a packet of a tile from a source buffer
@param t2 T2 handle
@param src Source buffer
@param len Length of the source buffer
@param tile Tile for which to write the packets
@param tcp Tile coding parameters
@param pi Packet identity
@param pack_info Packet information
@return 
*/
static int t2_decode_packet(opj_t2_t* t2, unsigned char *src, int len, opj_tcd_tile_t *tile, 
                                                                                                                opj_tcp_t *tcp, opj_pi_iterator_t *pi, opj_packet_info_t *pack_info);


/**
Decode a packet of a tile from a source buffer
@param t2 T2 handle
@param tile Tile for which to write the packets
@param tcp Tile coding parameters
@param pi Packet identity
@param src Source buffer
@param data_read   FIXME DOC
@param max_length  FIXME DOC
@param pack_info Packet information

@return  FIXME DOC
*/
static opj_bool t2_decode_packet_v2(
                             opj_t2_v2_t* t2,
                             opj_tcd_tile_v2_t *tile,
                             opj_tcp_v2_t *tcp,
                             opj_pi_iterator_t *pi,
                             OPJ_BYTE *src,
                             OPJ_UINT32 * data_read,
                             OPJ_UINT32 max_length,
                             opj_packet_info_t *pack_info);

static opj_bool t2_skip_packet(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                             opj_tcp_v2_t *p_tcp,
                                                         opj_pi_iterator_t *p_pi,
                                                         OPJ_BYTE *p_src,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *p_pack_info);

static opj_bool t2_read_packet_header(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                             opj_tcp_v2_t *p_tcp,
                                                         opj_pi_iterator_t *p_pi,
                                                         opj_bool * p_is_data_present,
                                                         OPJ_BYTE *p_src_data,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *p_pack_info);

static opj_bool t2_read_packet_data(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                                                         opj_pi_iterator_t *p_pi,
                                                         OPJ_BYTE *p_src_data,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *pack_info);

static opj_bool t2_skip_packet_data(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                                                         opj_pi_iterator_t *p_pi,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *pack_info);

/**
@param cblk
@param index
@param cblksty
@param first
*/
static opj_bool t2_init_seg_v2( opj_tcd_cblk_dec_v2_t* cblk,
                                                                OPJ_UINT32 index,
                                                                OPJ_UINT32 cblksty,
                                                                OPJ_UINT32 first);

/*@}*/

/*@}*/

/* ----------------------------------------------------------------------- */

/* #define RESTART 0x04 */

static void t2_putcommacode(opj_bio_t *bio, int n) {
        while (--n >= 0) {
                bio_write(bio, 1, 1);
        }
        bio_write(bio, 0, 1);
}

static int t2_getcommacode(opj_bio_t *bio) {
        int n;
        for (n = 0; bio_read(bio, 1); n++) {
                ;
        }
        return n;
}

static void t2_putnumpasses(opj_bio_t *bio, int n) {
        if (n == 1) {
                bio_write(bio, 0, 1);
        } else if (n == 2) {
                bio_write(bio, 2, 2);
        } else if (n <= 5) {
                bio_write(bio, 0xc | (n - 3), 4);
        } else if (n <= 36) {
                bio_write(bio, 0x1e0 | (n - 6), 9);
        } else if (n <= 164) {
                bio_write(bio, 0xff80 | (n - 37), 16);
        }
}

static int t2_getnumpasses(opj_bio_t *bio) {
        int n;
        if (!bio_read(bio, 1))
                return 1;
        if (!bio_read(bio, 1))
                return 2;
        if ((n = bio_read(bio, 2)) != 3)
                return (3 + n);
        if ((n = bio_read(bio, 5)) != 31)
                return (6 + n);
        return (37 + bio_read(bio, 7));
}

static int t2_encode_packet(opj_tcd_tile_t * tile, opj_tcp_t * tcp, opj_pi_iterator_t *pi, unsigned char *dest, int length, opj_codestream_info_t *cstr_info, int tileno) {
        int bandno, cblkno;
        unsigned char *c = dest;

        int compno = pi->compno;        /* component value */
        int resno  = pi->resno;         /* resolution level value */
        int precno = pi->precno;        /* precinct value */
        int layno  = pi->layno;         /* quality layer value */

        opj_tcd_tilecomp_t *tilec = &tile->comps[compno];
        opj_tcd_resolution_t *res = &tilec->resolutions[resno];
        
        opj_bio_t *bio = NULL;  /* BIO component */
        
        /* <SOP 0xff91> */
        if (tcp->csty & J2K_CP_CSTY_SOP) {
                c[0] = 255;
                c[1] = 145;
                c[2] = 0;
                c[3] = 4;
                c[4] = (unsigned char)((tile->packno % 65536) / 256);
                c[5] = (unsigned char)((tile->packno % 65536) % 256);
                c += 6;
        }
        /* </SOP> */
        
        if (!layno) {
                for (bandno = 0; bandno < res->numbands; bandno++) {
                        opj_tcd_band_t *band = &res->bands[bandno];
                        opj_tcd_precinct_t *prc = &band->precincts[precno];
                        tgt_reset(prc->incltree);
                        tgt_reset(prc->imsbtree);
                        for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
                                opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
                                cblk->numpasses = 0;
                                tgt_setvalue(prc->imsbtree, cblkno, band->numbps - cblk->numbps);
                        }
                }
        }
        
        bio = bio_create();
        bio_init_enc(bio, c, length);
        bio_write(bio, 1, 1);           /* Empty header bit */
        
        /* Writing Packet header */
        for (bandno = 0; bandno < res->numbands; bandno++) {
                opj_tcd_band_t *band = &res->bands[bandno];
                opj_tcd_precinct_t *prc = &band->precincts[precno];
                for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
                        opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
                        opj_tcd_layer_t *layer = &cblk->layers[layno];
                        if (!cblk->numpasses && layer->numpasses) {
                                tgt_setvalue(prc->incltree, cblkno, layno);
                        }
                }
                for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
                        opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
                        opj_tcd_layer_t *layer = &cblk->layers[layno];
                        int increment = 0;
                        int nump = 0;
                        int len = 0, passno;
                        /* cblk inclusion bits */
                        if (!cblk->numpasses) {
                                tgt_encode(bio, prc->incltree, cblkno, layno + 1);
                        } else {
                                bio_write(bio, layer->numpasses != 0, 1);
                        }
                        /* if cblk not included, go to the next cblk  */
                        if (!layer->numpasses) {
                                continue;
                        }
                        /* if first instance of cblk --> zero bit-planes information */
                        if (!cblk->numpasses) {
                                cblk->numlenbits = 3;
                                tgt_encode(bio, prc->imsbtree, cblkno, 999);
                        }
                        /* number of coding passes included */
                        t2_putnumpasses(bio, layer->numpasses);
                        
                        /* computation of the increase of the length indicator and insertion in the header     */
                        for (passno = cblk->numpasses; passno < cblk->numpasses + layer->numpasses; passno++) {
                                opj_tcd_pass_t *pass = &cblk->passes[passno];
                                nump++;
                                len += pass->len;
                                if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
                                        increment = int_max(increment, int_floorlog2(len) + 1 - (cblk->numlenbits + int_floorlog2(nump)));
                                        len = 0;
                                        nump = 0;
                                }
                        }
                        t2_putcommacode(bio, increment);

                        /* computation of the new Length indicator */
                        cblk->numlenbits += increment;

                        /* insertion of the codeword segment length */
                        for (passno = cblk->numpasses; passno < cblk->numpasses + layer->numpasses; passno++) {
                                opj_tcd_pass_t *pass = &cblk->passes[passno];
                                nump++;
                                len += pass->len;
                                if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
                                        bio_write(bio, len, cblk->numlenbits + int_floorlog2(nump));
                                        len = 0;
                                        nump = 0;
                                }
                        }
                }
        }

        if (bio_flush(bio)) {
                bio_destroy(bio);
                return -999;            /* modified to eliminate longjmp !! */
        }

        c += bio_numbytes(bio);
        bio_destroy(bio);
        
        /* <EPH 0xff92> */
        if (tcp->csty & J2K_CP_CSTY_EPH) {
                c[0] = 255;
                c[1] = 146;
                c += 2;
        }
        /* </EPH> */

        /* << INDEX */
        /* End of packet header position. Currently only represents the distance to start of packet
        // Will be updated later by incrementing with packet start value */
        if(cstr_info && cstr_info->index_write) {
                opj_packet_info_t *info_PK = &cstr_info->tile[tileno].packet[cstr_info->packno];
                info_PK->end_ph_pos = (int)(c - dest);
        }
        /* INDEX >> */
        
        /* Writing the packet body */
        
        for (bandno = 0; bandno < res->numbands; bandno++) {
                opj_tcd_band_t *band = &res->bands[bandno];
                opj_tcd_precinct_t *prc = &band->precincts[precno];
                for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
                        opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
                        opj_tcd_layer_t *layer = &cblk->layers[layno];
                        if (!layer->numpasses) {
                                continue;
                        }
                        if (c + layer->len > dest + length) {
                                return -999;
                        }
                        
                        memcpy(c, layer->data, layer->len);
                        cblk->numpasses += layer->numpasses;
                        c += layer->len;
                        /* << INDEX */ 
                        if(cstr_info && cstr_info->index_write) {
                                opj_packet_info_t *info_PK = &cstr_info->tile[tileno].packet[cstr_info->packno];
                                info_PK->disto += layer->disto;
                                if (cstr_info->D_max < info_PK->disto) {
                                        cstr_info->D_max = info_PK->disto;
                                }
                        }
                        /* INDEX >> */
                }
        }
        
        return (c - dest);
}

static opj_bool t2_init_seg(opj_tcd_cblk_dec_t* cblk, int index, int cblksty, int first) {
        opj_tcd_seg_t* seg;
        opj_tcd_seg_t* new_segs = (opj_tcd_seg_t*) opj_realloc(cblk->segs, (index + 1) * sizeof(opj_tcd_seg_t));
        if (!new_segs) {
                /* opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory to init segment #%d\n", index); */
                /* TODO: tell cblk has no segment (in order to update the range of valid indices)*/
                cblk->segs = NULL;
                return OPJ_FALSE;
        }
        cblk->segs = new_segs;
        seg = &cblk->segs[index];
        seg->data = NULL;
        seg->dataindex = 0;
        seg->numpasses = 0;
        seg->len = 0;
        if (cblksty & J2K_CCP_CBLKSTY_TERMALL) {
                seg->maxpasses = 1;
        }
        else if (cblksty & J2K_CCP_CBLKSTY_LAZY) {
                if (first) {
                        seg->maxpasses = 10;
                } else {
                        seg->maxpasses = (((seg - 1)->maxpasses == 1) || ((seg - 1)->maxpasses == 10)) ? 2 : 1;
                }
        } else {
                seg->maxpasses = 109;
        }
        return OPJ_TRUE;
}

static int t2_decode_packet(opj_t2_t* t2, unsigned char *src, int len, opj_tcd_tile_t *tile, 
                opj_tcp_t *tcp, opj_pi_iterator_t *pi, opj_packet_info_t *pack_info) {
        int bandno, cblkno;
        unsigned char *c = src;

        opj_cp_t *cp = t2->cp;

        int compno = pi->compno;        /* component value */
        int resno  = pi->resno;         /* resolution level value */
        int precno = pi->precno;        /* precinct value */
        int layno  = pi->layno;         /* quality layer value */

        opj_tcd_resolution_t* res = &tile->comps[compno].resolutions[resno];

        unsigned char *hd = NULL;
        int present;
        
        opj_bio_t *bio = NULL;  /* BIO component */
        
        if (layno == 0) {
                for (bandno = 0; bandno < res->numbands; bandno++) {
                        opj_tcd_band_t *band = &res->bands[bandno];
                        opj_tcd_precinct_t *prc = &band->precincts[precno];
                        
                        if ((band->x1-band->x0 == 0)||(band->y1-band->y0 == 0)) continue;
                        
                        tgt_reset(prc->incltree);
                        tgt_reset(prc->imsbtree);
                        for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
                                opj_tcd_cblk_dec_t* cblk = &prc->cblks.dec[cblkno];
                                cblk->numsegs = 0;
                        }
                }
        }
        
        /* SOP markers */
        
        if (tcp->csty & J2K_CP_CSTY_SOP) {
                if ((*c) != 0xff || (*(c + 1) != 0x91)) {
                        opj_event_msg(t2->cinfo, EVT_WARNING, "Expected SOP marker\n");
                } else {
                        c += 6;
                }
                
                /** TODO : check the Nsop value */
        }
        
        /* 
        When the marker PPT/PPM is used the packet header are store in PPT/PPM marker
        This part deal with this caracteristic
        step 1: Read packet header in the saved structure
        step 2: Return to codestream for decoding 
        */

        bio = bio_create();
        
        if (cp->ppm == 1) {             /* PPM */
                hd = cp->ppm_data;
                bio_init_dec(bio, hd, cp->ppm_len);
        } else if (tcp->ppt == 1) {     /* PPT */
                hd = tcp->ppt_data;
                bio_init_dec(bio, hd, tcp->ppt_len);
        } else {                        /* Normal Case */
                hd = c;
                bio_init_dec(bio, hd, src+len-hd);
        }
        
        present = bio_read(bio, 1);
        
        if (!present) {
                bio_inalign(bio);
                hd += bio_numbytes(bio);
                bio_destroy(bio);
                
                /* EPH markers */
                
                if (tcp->csty & J2K_CP_CSTY_EPH) {
                        if ((*hd) != 0xff || (*(hd + 1) != 0x92)) {
                                printf("Error : expected EPH marker\n");
                        } else {
                                hd += 2;
                        }
                }

                /* << INDEX */
                /* End of packet header position. Currently only represents the distance to start of packet
                // Will be updated later by incrementing with packet start value*/
                if(pack_info) {
                        pack_info->end_ph_pos = (int)(c - src);
                }
                /* INDEX >> */
                
                if (cp->ppm == 1) {             /* PPM case */
                        cp->ppm_len += cp->ppm_data-hd;
                        cp->ppm_data = hd;
                        return (c - src);
                }
                if (tcp->ppt == 1) {    /* PPT case */
                        tcp->ppt_len+=tcp->ppt_data-hd;
                        tcp->ppt_data = hd;
                        return (c - src);
                }
                
                return (hd - src);
        }
        
        for (bandno = 0; bandno < res->numbands; bandno++) {
                opj_tcd_band_t *band = &res->bands[bandno];
                opj_tcd_precinct_t *prc = &band->precincts[precno];
                
                if ((band->x1-band->x0 == 0)||(band->y1-band->y0 == 0)) continue;
                
                for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
                        int included, increment, n, segno;
                        opj_tcd_cblk_dec_t* cblk = &prc->cblks.dec[cblkno];
                        /* if cblk not yet included before --> inclusion tagtree */
                        if (!cblk->numsegs) {
                                included = tgt_decode(bio, prc->incltree, cblkno, layno + 1);
                                /* else one bit */
                        } else {
                                included = bio_read(bio, 1);
                        }
                        /* if cblk not included */
                        if (!included) {
                                cblk->numnewpasses = 0;
                                continue;
                        }
                        /* if cblk not yet included --> zero-bitplane tagtree */
                        if (!cblk->numsegs) {
                                int i, numimsbs;
                                for (i = 0; !tgt_decode(bio, prc->imsbtree, cblkno, i); i++) {
                                        ;
                                }
                                numimsbs = i - 1;
                                cblk->numbps = band->numbps - numimsbs;
                                cblk->numlenbits = 3;
                        }
                        /* number of coding passes */
                        cblk->numnewpasses = t2_getnumpasses(bio);
                        increment = t2_getcommacode(bio);
                        /* length indicator increment */
                        cblk->numlenbits += increment;
                        segno = 0;
                        if (!cblk->numsegs) {
                                if (OPJ_FALSE == t2_init_seg(cblk, segno, tcp->tccps[compno].cblksty, 1)) {
                                        /* TODO: LH: shall we destroy bio here ?*/
                                        opj_event_msg(t2->cinfo, EVT_WARNING, "Not enough memory to init segment #%d\n", segno);
                                        return -999;
                                }
                        } else {
                                segno = cblk->numsegs - 1;
                                if (cblk->segs[segno].numpasses == cblk->segs[segno].maxpasses) {
                                        ++segno;
                                        if (OPJ_FALSE == t2_init_seg(cblk, segno, tcp->tccps[compno].cblksty, 0)) {
                                                /* TODO: LH: shall we destroy bio here ?*/
                                                opj_event_msg(t2->cinfo, EVT_WARNING, "Not enough memory to init segment #%d\n", segno);
                                                return -999;
                                        }
                                }
                        }
                        n = cblk->numnewpasses;
                        
                        do {
                                cblk->segs[segno].numnewpasses = int_min(cblk->segs[segno].maxpasses - cblk->segs[segno].numpasses, n);
                                cblk->segs[segno].newlen = bio_read(bio, cblk->numlenbits + int_floorlog2(cblk->segs[segno].numnewpasses));
                                n -= cblk->segs[segno].numnewpasses;
                                if (n > 0) {
                                        ++segno;
                                        if (OPJ_FALSE == t2_init_seg(cblk, segno, tcp->tccps[compno].cblksty, 0)) {
                                                /* TODO: LH: shall we destroy bio here ? */
                                                opj_event_msg(t2->cinfo, EVT_WARNING, "Not enough memory to init segment #%d\n", segno);
                                                return -999;
                                        }
                                }
                        } while (n > 0);
                }
        }
        
        if (bio_inalign(bio)) {
                bio_destroy(bio);
                return -999;
        }
        
        hd += bio_numbytes(bio);
        bio_destroy(bio);
        
        /* EPH markers */
        if (tcp->csty & J2K_CP_CSTY_EPH) {
                if ((*hd) != 0xff || (*(hd + 1) != 0x92)) {
                        opj_event_msg(t2->cinfo, EVT_ERROR, "Expected EPH marker\n");
                        return -999;
                } else {
                        hd += 2;
                }
        }

        /* << INDEX */
        /* End of packet header position. Currently only represents the distance to start of packet
        // Will be updated later by incrementing with packet start value*/
        if(pack_info) {
                pack_info->end_ph_pos = (int)(hd - src);
        }
        /* INDEX >> */
        
        if (cp->ppm==1) {
                cp->ppm_len+=cp->ppm_data-hd;
                cp->ppm_data = hd;
        } else if (tcp->ppt == 1) {
                tcp->ppt_len+=tcp->ppt_data-hd;
                tcp->ppt_data = hd;
        } else {
                c=hd;
        }
        
        for (bandno = 0; bandno < res->numbands; bandno++) {
                opj_tcd_band_t *band = &res->bands[bandno];
                opj_tcd_precinct_t *prc = &band->precincts[precno];
                
                if ((band->x1-band->x0 == 0)||(band->y1-band->y0 == 0)) continue;
                
                for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
                        opj_tcd_cblk_dec_t* cblk = &prc->cblks.dec[cblkno];
                        opj_tcd_seg_t *seg = NULL;
                        if (!cblk->numnewpasses)
                                continue;
                        if (!cblk->numsegs) {
                                seg = &cblk->segs[0];
                                cblk->numsegs++;
                                cblk->len = 0;
                        } else {
                                seg = &cblk->segs[cblk->numsegs - 1];
                                if (seg->numpasses == seg->maxpasses) {
                                        seg++;
                                        cblk->numsegs++;
                                }
                        }
                        
                        do {
                                unsigned char * new_data;
                                if (c + seg->newlen > src + len) {
                                        return -999;
                                }

#ifdef USE_JPWL
                        /* we need here a j2k handle to verify if making a check to
                        the validity of cblocks parameters is selected from user (-W) */

                                /* let's check that we are not exceeding */
                                if ((cblk->len + seg->newlen) > 8192) {
                                        opj_event_msg(t2->cinfo, EVT_WARNING,
                                                "JPWL: segment too long (%d) for codeblock %d (p=%d, b=%d, r=%d, c=%d)\n",
                                                seg->newlen, cblkno, precno, bandno, resno, compno);
                                        if (!JPWL_ASSUME) {
                                                opj_event_msg(t2->cinfo, EVT_ERROR, "JPWL: giving up\n");
                                                return -999;
                                        }
                                        seg->newlen = 8192 - cblk->len;
                                        opj_event_msg(t2->cinfo, EVT_WARNING, "      - truncating segment to %d\n", seg->newlen);
                                        break;
                                };

#endif /* USE_JPWL */
                                
                                new_data = (unsigned char*) opj_realloc(cblk->data, (cblk->len + seg->newlen) * sizeof(unsigned char));
                                if (! new_data) {
                                        opj_event_msg(t2->cinfo, EVT_ERROR, "JPWL: Not enough memory for codeblock data %d (p=%d, b=%d, r=%d, c=%d)\n",
                                                        seg->newlen, cblkno, precno, bandno, resno, compno);
                                        cblk->data = 0;
                                        cblk->len  = 0; /* TODO: LH: other things to reset ?*/
                                        opj_free(cblk->data);
                                        return -999;
                                }
                                cblk->data = new_data;
                                memcpy(cblk->data + cblk->len, c, seg->newlen);
                                if (seg->numpasses == 0) {
                                        seg->data = &cblk->data;
                                        seg->dataindex = cblk->len;
                                }
                                c += seg->newlen;
                                cblk->len += seg->newlen;
                                seg->len += seg->newlen;
                                seg->numpasses += seg->numnewpasses;
                                cblk->numnewpasses -= seg->numnewpasses;
                                if (cblk->numnewpasses > 0) {
                                        seg++;
                                        cblk->numsegs++;
                                }
                        } while (cblk->numnewpasses > 0);
                }
        }
        
        return (c - src);
}

/* ----------------------------------------------------------------------- */

int t2_encode_packets(opj_t2_t* t2,int tileno, opj_tcd_tile_t *tile, int maxlayers, unsigned char *dest, int len, opj_codestream_info_t *cstr_info,int tpnum, int tppos,int pino, J2K_T2_MODE t2_mode, int cur_totnum_tp){
        unsigned char *c = dest;
        int e = 0;
        int compno;
        opj_pi_iterator_t *pi = NULL;
        int poc;
        opj_image_t *image = t2->image;
        opj_cp_t *cp = t2->cp;
        opj_tcp_t *tcp = &cp->tcps[tileno];
        int pocno = cp->cinema == CINEMA4K_24? 2: 1;
        int maxcomp = cp->max_comp_size > 0 ? image->numcomps : 1;
        
        pi = pi_initialise_encode(image, cp, tileno, t2_mode);
        if(!pi) {
                /* TODO: throw an error */
                return -999;
        }
        
        if(t2_mode == THRESH_CALC ){ /* Calculating threshold */
                for(compno = 0; compno < maxcomp; compno++ ){
                        for(poc = 0; poc < pocno ; poc++){
                                int comp_len = 0;
                                int tpnum = compno;
                                if (pi_create_encode(pi, cp,tileno,poc,tpnum,tppos,t2_mode,cur_totnum_tp)) {
                                        opj_event_msg(t2->cinfo, EVT_ERROR, "Error initializing Packet Iterator\n");
                                        pi_destroy(pi, cp, tileno);
                                        return -999;
                                }
                                while (pi_next(&pi[poc])) {
                                        if (pi[poc].layno < maxlayers) {
                                                e = t2_encode_packet(tile, &cp->tcps[tileno], &pi[poc], c, dest + len - c, cstr_info, tileno);
                                                comp_len = comp_len + e;
                                                if (e == -999) {
                                                        break;
                                                } else {
                                                        c += e;
                                                }
                                        }
                                }
                                if (e == -999) break;
                                if (cp->max_comp_size){
                                        if (comp_len > cp->max_comp_size){
                                                e = -999;
                                                break;
                                        }
                                }
                        }
                        if (e == -999)  break;
                }
        }else{  /* t2_mode == FINAL_PASS  */
                pi_create_encode(pi, cp,tileno,pino,tpnum,tppos,t2_mode,cur_totnum_tp);
                while (pi_next(&pi[pino])) {
                        if (pi[pino].layno < maxlayers) {
                                e = t2_encode_packet(tile, &cp->tcps[tileno], &pi[pino], c, dest + len - c, cstr_info, tileno);
                                if (e == -999) {
                                        break;
                                } else {
                                        c += e;
                                }
                                /* INDEX >> */
                                if(cstr_info) {
                                        if(cstr_info->index_write) {
                                                opj_tile_info_t *info_TL = &cstr_info->tile[tileno];
                                                opj_packet_info_t *info_PK = &info_TL->packet[cstr_info->packno];
                                                if (!cstr_info->packno) {
                                                        info_PK->start_pos = info_TL->end_header + 1;
                                                } else {
                                                        info_PK->start_pos = ((cp->tp_on | tcp->POC)&& info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[cstr_info->packno - 1].end_pos + 1;
                                                }
                                                info_PK->end_pos = info_PK->start_pos + e - 1;
                                                info_PK->end_ph_pos += info_PK->start_pos - 1;  /* End of packet header which now only represents the distance 
                                                                                                                                                                                                                                                // to start of packet is incremented by value of start of packet*/
                                        }
                                        
                                        cstr_info->packno++;
                                }
                                /* << INDEX */
                                tile->packno++;
                        }
                }
        }
        
        pi_destroy(pi, cp, tileno);
        
        if (e == -999) {
                return e;
        }
        
  return (c - dest);
}

opj_bool t2_encode_packets_v2(
                                           opj_t2_v2_t* p_t2,
                                           OPJ_UINT32 p_tile_no,
                                           opj_tcd_tile_v2_t *p_tile,
                                           OPJ_UINT32 p_maxlayers,
                                           OPJ_BYTE *p_dest,
                                           OPJ_UINT32 * p_data_written,
                                           OPJ_UINT32 p_max_len,
                                           opj_codestream_info_t *cstr_info,
                                           OPJ_UINT32 p_tp_num,
                                           OPJ_INT32 p_tp_pos,
                                           OPJ_UINT32 p_pino,
                                           J2K_T2_MODE p_t2_mode)
{
        OPJ_BYTE *l_current_data = p_dest;
        OPJ_UINT32 l_nb_bytes = 0;
        OPJ_UINT32 compno;
        OPJ_UINT32 poc;
        opj_pi_iterator_t *l_pi = 00;
        opj_pi_iterator_t *l_current_pi = 00;
        opj_image_t *l_image = p_t2->image;
        opj_cp_v2_t *l_cp = p_t2->cp;
        opj_tcp_v2_t *l_tcp = &l_cp->tcps[p_tile_no];
        OPJ_UINT32 pocno = l_cp->m_specific_param.m_enc.m_cinema == CINEMA4K_24? 2: 1;
        OPJ_UINT32 l_max_comp = l_cp->m_specific_param.m_enc.m_max_comp_size > 0 ? l_image->numcomps : 1;
        OPJ_UINT32 l_nb_pocs = l_tcp->numpocs + 1;

        l_pi = pi_initialise_encode_v2(l_image, l_cp, p_tile_no, p_t2_mode);
        if (!l_pi) {
                return OPJ_FALSE;
        }

        * p_data_written = 0;

        if (p_t2_mode == THRESH_CALC ){ /* Calculating threshold */
                l_current_pi = l_pi;

                for     (compno = 0; compno < l_max_comp; ++compno) {
                        OPJ_UINT32 l_comp_len = 0;
                        l_current_pi = l_pi;

                        for (poc = 0; poc < pocno ; ++poc) {
                                OPJ_UINT32 l_tp_num = compno;

                                pi_create_encode_v2(l_pi, l_cp,p_tile_no,poc,l_tp_num,p_tp_pos,p_t2_mode);

                                while (pi_next(l_current_pi)) {
                                        if (l_current_pi->layno < p_maxlayers) {
                                                l_nb_bytes = 0;

                                                if (! t2_encode_packet_v2(p_tile_no,p_tile, l_tcp, l_current_pi, l_current_data, &l_nb_bytes, p_max_len, cstr_info)) {
                                                        pi_destroy_v2(l_pi, l_nb_pocs);
                                                        return OPJ_FALSE;
                                                }

                                                l_comp_len += l_nb_bytes;
                                                l_current_data += l_nb_bytes;
                                                p_max_len -= l_nb_bytes;

                                                * p_data_written += l_nb_bytes;
                                        }
                                }

                                if (l_cp->m_specific_param.m_enc.m_max_comp_size) {
                                        if (l_comp_len > l_cp->m_specific_param.m_enc.m_max_comp_size) {
                                                pi_destroy_v2(l_pi, l_nb_pocs);
                                                return OPJ_FALSE;
                                        }
                                }

                                ++l_current_pi;
                        }
                }
        }
        else {  /* t2_mode == FINAL_PASS  */
                pi_create_encode_v2(l_pi, l_cp,p_tile_no,p_pino,p_tp_num,p_tp_pos,p_t2_mode);

                l_current_pi = &l_pi[p_pino];

                while (pi_next(l_current_pi)) {
                        if (l_current_pi->layno < p_maxlayers) {
                                l_nb_bytes=0;

                                if (! t2_encode_packet_v2(p_tile_no,p_tile, l_tcp, l_current_pi, l_current_data, &l_nb_bytes, p_max_len, cstr_info)) {
                                        pi_destroy_v2(l_pi, l_nb_pocs);
                                        return OPJ_FALSE;
                                }

                                l_current_data += l_nb_bytes;
                                p_max_len -= l_nb_bytes;

                                * p_data_written += l_nb_bytes;

                                /* INDEX >> */
                                if(cstr_info) {
                                        if(cstr_info->index_write) {
                                                opj_tile_info_t *info_TL = &cstr_info->tile[p_tile_no];
                                                opj_packet_info_t *info_PK = &info_TL->packet[cstr_info->packno];
                                                if (!cstr_info->packno) {
                                                        info_PK->start_pos = info_TL->end_header + 1;
                                                } else {
                                                        info_PK->start_pos = ((l_cp->m_specific_param.m_enc.m_tp_on | l_tcp->POC)&& info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[cstr_info->packno - 1].end_pos + 1;
                                                }
                                                info_PK->end_pos = info_PK->start_pos + l_nb_bytes - 1;
                                                info_PK->end_ph_pos += info_PK->start_pos - 1;  /* End of packet header which now only represents the distance
                                                                                                                                                                                                                                                   to start of packet is incremented by value of start of packet*/
                                        }

                                        cstr_info->packno++;
                                }
                                /* << INDEX */
                                ++p_tile->packno;
                        }
                }
        }

        pi_destroy_v2(l_pi, l_nb_pocs);

        return OPJ_TRUE;
}


int t2_decode_packets(opj_t2_t *t2, unsigned char *src, int len, int tileno, opj_tcd_tile_t *tile, opj_codestream_info_t *cstr_info) {
        unsigned char *c = src;
        opj_pi_iterator_t *pi;
        int pino, e = 0;
        int n = 0, curtp = 0;
        int tp_start_packno;

        opj_image_t *image = t2->image;
        opj_cp_t *cp = t2->cp;
        
        /* create a packet iterator */
        pi = pi_create_decode(image, cp, tileno);
        if(!pi) {
                /* TODO: throw an error */
                return -999;
        }

        tp_start_packno = 0;
        
        for (pino = 0; pino <= cp->tcps[tileno].numpocs; pino++) {
                while (pi_next(&pi[pino])) {
                        if ((cp->layer==0) || (cp->layer>=((pi[pino].layno)+1))) {
                                opj_packet_info_t *pack_info;
                                if (cstr_info)
                                        pack_info = &cstr_info->tile[tileno].packet[cstr_info->packno];
                                else
                                        pack_info = NULL;
                                e = t2_decode_packet(t2, c, src + len - c, tile, &cp->tcps[tileno], &pi[pino], pack_info);
                        } else {
                                e = 0;
                        }
                        if(e == -999) return -999;
                        /* progression in resolution */
                        image->comps[pi[pino].compno].resno_decoded =   
                                (e > 0) ? 
                                int_max(pi[pino].resno, image->comps[pi[pino].compno].resno_decoded) 
                                : image->comps[pi[pino].compno].resno_decoded;
                        n++;

                        /* INDEX >> */
                        if(cstr_info) {
                                opj_tile_info_t *info_TL = &cstr_info->tile[tileno];
                                opj_packet_info_t *info_PK = &info_TL->packet[cstr_info->packno];
                                if (!cstr_info->packno) {
                                        info_PK->start_pos = info_TL->end_header + 1;
                                } else if (info_TL->packet[cstr_info->packno-1].end_pos >= (int)cstr_info->tile[tileno].tp[curtp].tp_end_pos){ /* New tile part*/
                                        info_TL->tp[curtp].tp_numpacks = cstr_info->packno - tp_start_packno; /* Number of packets in previous tile-part*/
          info_TL->tp[curtp].tp_start_pack = tp_start_packno;
                                        tp_start_packno = cstr_info->packno;
                                        curtp++;
                                        info_PK->start_pos = cstr_info->tile[tileno].tp[curtp].tp_end_header+1;
                                } else {
                                        info_PK->start_pos = (cp->tp_on && info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[cstr_info->packno - 1].end_pos + 1;
                                }
                                info_PK->end_pos = info_PK->start_pos + e - 1;
                                info_PK->end_ph_pos += info_PK->start_pos - 1;  /* End of packet header which now only represents the distance 
                                                                                                                                                                                                                                // to start of packet is incremented by value of start of packet*/
                                cstr_info->packno++;
                        }
                        /* << INDEX */
                        
                        if (e == -999) {                /* ADD */
                                break;
                        } else {
                                c += e;
                        }                       
                }
        }
        /* INDEX >> */
        if(cstr_info) {
                cstr_info->tile[tileno].tp[curtp].tp_numpacks = cstr_info->packno - tp_start_packno; /* Number of packets in last tile-part*/
    cstr_info->tile[tileno].tp[curtp].tp_start_pack = tp_start_packno;
        }
        /* << INDEX */

        /* don't forget to release pi */
        pi_destroy(pi, cp, tileno);
        
        if (e == -999) {
                return e;
        }
        
        return (c - src);
}

opj_bool t2_decode_packets_v2(
                                                opj_t2_v2_t *p_t2,
                                                OPJ_UINT32 p_tile_no,
                                                struct opj_tcd_tile_v2 *p_tile,
                                                OPJ_BYTE *p_src,
                                                OPJ_UINT32 * p_data_read,
                                                OPJ_UINT32 p_max_len,
                                                opj_codestream_index_t *p_cstr_index)
{
        OPJ_BYTE *l_current_data = p_src;
        opj_pi_iterator_t *l_pi = 00;
        OPJ_UINT32 pino;
        opj_image_t *l_image = p_t2->image;
        opj_cp_v2_t *l_cp = p_t2->cp;
        opj_cp_v2_t *cp = p_t2->cp;
        opj_tcp_v2_t *l_tcp = &(p_t2->cp->tcps[p_tile_no]);
        OPJ_UINT32 l_nb_bytes_read;
        OPJ_UINT32 l_nb_pocs = l_tcp->numpocs + 1;
        opj_pi_iterator_t *l_current_pi = 00;
        OPJ_UINT32 curtp = 0;
        OPJ_UINT32 tp_start_packno;
        opj_packet_info_t *l_pack_info = 00;
        opj_image_comp_t* l_img_comp = 00;

#ifdef TODO_MSD
        if (p_cstr_index) {
                l_pack_info = p_cstr_index->tile_index[p_tile_no].packet;
        }
#endif

        /* create a packet iterator */
        l_pi = pi_create_decode_v2(l_image, l_cp, p_tile_no);
        if (!l_pi) {
                return OPJ_FALSE;
        }

        tp_start_packno = 0;
        l_current_pi = l_pi;

        for     (pino = 0; pino <= l_tcp->numpocs; ++pino) {

                /* if the resolution needed is to low, one dim of the tilec could be equal to zero
                 * and no packets are used to encode this resolution and
                 * l_current_pi->resno is always >= p_tile->comps[l_current_pi->compno].minimum_num_resolutions
                 * and no l_img_comp->resno_decoded are computed
                 */
                opj_bool* first_pass_failed = (opj_bool*)opj_malloc(l_image->numcomps * sizeof(opj_bool));
                memset(first_pass_failed, OPJ_TRUE, l_image->numcomps * sizeof(opj_bool));

                while (pi_next(l_current_pi)) {


                        if (l_tcp->num_layers_to_decode > l_current_pi->layno
                                        && l_current_pi->resno < p_tile->comps[l_current_pi->compno].minimum_num_resolutions) {
                                l_nb_bytes_read = 0;

                                first_pass_failed[l_current_pi->compno] = OPJ_FALSE;

                                if (! t2_decode_packet_v2(p_t2,p_tile,l_tcp,l_current_pi,l_current_data,&l_nb_bytes_read,p_max_len,l_pack_info)) {
                                        pi_destroy_v2(l_pi,l_nb_pocs);
                                        return OPJ_FALSE;
                                }

                                l_img_comp = &(l_image->comps[l_current_pi->compno]);
                                l_img_comp->resno_decoded = uint_max(l_current_pi->resno, l_img_comp->resno_decoded);
                        }
                        else {
                                l_nb_bytes_read = 0;
                                if (! t2_skip_packet(p_t2,p_tile,l_tcp,l_current_pi,l_current_data,&l_nb_bytes_read,p_max_len,l_pack_info)) {
                                        pi_destroy_v2(l_pi,l_nb_pocs);
                                        return OPJ_FALSE;
                                }
                        }

                        if (first_pass_failed[l_current_pi->compno]) {
                                l_img_comp = &(l_image->comps[l_current_pi->compno]);
                                if (l_img_comp->resno_decoded == 0)
                                        l_img_comp->resno_decoded = p_tile->comps[l_current_pi->compno].minimum_num_resolutions - 1;
                        }

                        l_current_data += l_nb_bytes_read;
                        p_max_len -= l_nb_bytes_read;

                        /* INDEX >> */
#ifdef TODO_MSD
                        if(p_cstr_info) {
                                opj_tile_info_v2_t *info_TL = &p_cstr_info->tile[p_tile_no];
                                opj_packet_info_t *info_PK = &info_TL->packet[p_cstr_info->packno];
                                if (!p_cstr_info->packno) {
                                        info_PK->start_pos = info_TL->end_header + 1;
                                } else if (info_TL->packet[p_cstr_info->packno-1].end_pos >= (OPJ_INT32)p_cstr_info->tile[p_tile_no].tp[curtp].tp_end_pos){ /* New tile part */
                                        info_TL->tp[curtp].tp_numpacks = p_cstr_info->packno - tp_start_packno; /* Number of packets in previous tile-part */
                                        tp_start_packno = p_cstr_info->packno;
                                        curtp++;
                                        info_PK->start_pos = p_cstr_info->tile[p_tile_no].tp[curtp].tp_end_header+1;
                                } else {
                                        info_PK->start_pos = (cp->m_specific_param.m_enc.m_tp_on && info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[p_cstr_info->packno - 1].end_pos + 1;
                                }
                                info_PK->end_pos = info_PK->start_pos + l_nb_bytes_read - 1;
                                info_PK->end_ph_pos += info_PK->start_pos - 1;  /* End of packet header which now only represents the distance */
                                ++p_cstr_info->packno;
                        }
#endif
                        /* << INDEX */
                }
                ++l_current_pi;

                opj_free(first_pass_failed);
        }
        /* INDEX >> */
#ifdef TODO_MSD
        if
                (p_cstr_info) {
                p_cstr_info->tile[p_tile_no].tp[curtp].tp_numpacks = p_cstr_info->packno - tp_start_packno; /* Number of packets in last tile-part */
        }
#endif
        /* << INDEX */

        /* don't forget to release pi */
        pi_destroy_v2(l_pi,l_nb_pocs);
        *p_data_read = l_current_data - p_src;
        return OPJ_TRUE;
}

/* ----------------------------------------------------------------------- */

opj_t2_t* t2_create(opj_common_ptr cinfo, opj_image_t *image, opj_cp_t *cp) {
        /* create the tcd structure */
        opj_t2_t *t2 = (opj_t2_t*)opj_malloc(sizeof(opj_t2_t));
        if(!t2) return NULL;
        t2->cinfo = cinfo;
        t2->image = image;
        t2->cp = cp;

        return t2;
}

/**
 * Creates a Tier 2 handle
 *
 * @param       p_image         Source or destination image
 * @param       p_cp            Image coding parameters.
 * @return              a new T2 handle if successful, NULL otherwise.
*/
opj_t2_v2_t* t2_create_v2(      opj_image_t *p_image,
                                                        opj_cp_v2_t *p_cp)
{
        /* create the tcd structure */
        opj_t2_v2_t *l_t2 = (opj_t2_v2_t*)opj_malloc(sizeof(opj_t2_v2_t));
        if (!l_t2) {
                return NULL;
        }
        memset(l_t2,0,sizeof(opj_t2_v2_t));

        l_t2->image = p_image;
        l_t2->cp = p_cp;

        return l_t2;
}

void t2_destroy(opj_t2_t *t2) {
        if(t2) {
                opj_free(t2);
        }
}

void t2_destroy_v2(opj_t2_v2_t *t2) {
        if(t2) {
                opj_free(t2);
        }
}


static opj_bool t2_decode_packet_v2(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                             opj_tcp_v2_t *p_tcp,
                                                         opj_pi_iterator_t *p_pi,
                                                         OPJ_BYTE *p_src,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *p_pack_info)
{
        opj_bool l_read_data;
        OPJ_UINT32 l_nb_bytes_read = 0;
        OPJ_UINT32 l_nb_total_bytes_read = 0;

        *p_data_read = 0;

        if (! t2_read_packet_header(p_t2,p_tile,p_tcp,p_pi,&l_read_data,p_src,&l_nb_bytes_read,p_max_length,p_pack_info)) {
                return OPJ_FALSE;
        }

        p_src += l_nb_bytes_read;
        l_nb_total_bytes_read += l_nb_bytes_read;
        p_max_length -= l_nb_bytes_read;

        /* we should read data for the packet */
        if (l_read_data) {
                l_nb_bytes_read = 0;

                if (! t2_read_packet_data(p_t2,p_tile,p_pi,p_src,&l_nb_bytes_read,p_max_length,p_pack_info)) {
                        return OPJ_FALSE;
                }

                l_nb_total_bytes_read += l_nb_bytes_read;
        }

        *p_data_read = l_nb_total_bytes_read;

        return OPJ_TRUE;
}

static opj_bool t2_encode_packet_v2(
                                                         OPJ_UINT32 tileno,
                                                         opj_tcd_tile_v2_t * tile,
                                                         opj_tcp_v2_t * tcp,
                                                         opj_pi_iterator_t *pi,
                                                         OPJ_BYTE *dest,
                                                         OPJ_UINT32 * p_data_written,
                                                         OPJ_UINT32 length,
                                                         opj_codestream_info_t *cstr_info)
{
        OPJ_UINT32 bandno, cblkno;
        OPJ_BYTE *c = dest;
        OPJ_UINT32 l_nb_bytes;
        OPJ_UINT32 compno = pi->compno; /* component value */
        OPJ_UINT32 resno  = pi->resno;          /* resolution level value */
        OPJ_UINT32 precno = pi->precno; /* precinct value */
        OPJ_UINT32 layno  = pi->layno;          /* quality layer value */
        OPJ_UINT32 l_nb_blocks;
        opj_tcd_band_v2_t *band = 00;
        opj_tcd_cblk_enc_v2_t* cblk = 00;
        opj_tcd_pass_v2_t *pass = 00;

        opj_tcd_tilecomp_v2_t *tilec = &tile->comps[compno];
        opj_tcd_resolution_v2_t *res = &tilec->resolutions[resno];

        opj_bio_t *bio = 00;    /* BIO component */

        /* <SOP 0xff91> */
        if (tcp->csty & J2K_CP_CSTY_SOP) {
                c[0] = 255;
                c[1] = 145;
                c[2] = 0;
                c[3] = 4;
                c[4] = (tile->packno % 65536) / 256;
                c[5] = (tile->packno % 65536) % 256;
                c += 6;
                length -= 6;
        }
        /* </SOP> */

        if (!layno) {
                band = res->bands;

                for(bandno = 0; bandno < res->numbands; ++bandno) {
                        opj_tcd_precinct_v2_t *prc = &band->precincts[precno];

                        tgt_reset(prc->incltree);
                        tgt_reset(prc->imsbtree);

                        l_nb_blocks = prc->cw * prc->ch;
                        for     (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
                                opj_tcd_cblk_enc_v2_t* cblk = &prc->cblks.enc[cblkno];

                                cblk->numpasses = 0;
                                tgt_setvalue(prc->imsbtree, cblkno, band->numbps - cblk->numbps);
                        }
                        ++band;
                }
        }

        bio = bio_create();
        bio_init_enc(bio, c, length);
        bio_write(bio, 1, 1);           /* Empty header bit */

        /* Writing Packet header */
        band = res->bands;
        for (bandno = 0; bandno < res->numbands; ++bandno)      {
                opj_tcd_precinct_v2_t *prc = &band->precincts[precno];

                l_nb_blocks = prc->cw * prc->ch;
                cblk = prc->cblks.enc;

                for (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
                        opj_tcd_layer_t *layer = &cblk->layers[layno];

                        if (!cblk->numpasses && layer->numpasses) {
                                tgt_setvalue(prc->incltree, cblkno, layno);
                        }

                        ++cblk;
                }

                cblk = prc->cblks.enc;
                for (cblkno = 0; cblkno < l_nb_blocks; cblkno++) {
                        opj_tcd_layer_t *layer = &cblk->layers[layno];
                        OPJ_UINT32 increment = 0;
                        OPJ_UINT32 nump = 0;
                        OPJ_UINT32 len = 0, passno;
                        OPJ_UINT32 l_nb_passes;

                        /* cblk inclusion bits */
                        if (!cblk->numpasses) {
                                tgt_encode(bio, prc->incltree, cblkno, layno + 1);
                        } else {
                                bio_write(bio, layer->numpasses != 0, 1);
                        }

                        /* if cblk not included, go to the next cblk  */
                        if (!layer->numpasses) {
                                ++cblk;
                                continue;
                        }

                        /* if first instance of cblk --> zero bit-planes information */
                        if (!cblk->numpasses) {
                                cblk->numlenbits = 3;
                                tgt_encode(bio, prc->imsbtree, cblkno, 999);
                        }

                        /* number of coding passes included */
                        t2_putnumpasses(bio, layer->numpasses);
                        l_nb_passes = cblk->numpasses + layer->numpasses;
                        pass = cblk->passes +  cblk->numpasses;

                        /* computation of the increase of the length indicator and insertion in the header     */
                        for (passno = cblk->numpasses; passno < l_nb_passes; ++passno) {
                                ++nump;
                                len += pass->len;

                                if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
                                        increment = int_max(increment, int_floorlog2(len) + 1 - (cblk->numlenbits + int_floorlog2(nump)));
                                        len = 0;
                                        nump = 0;
                                }

                                ++pass;
                        }
                        t2_putcommacode(bio, increment);

                        /* computation of the new Length indicator */
                        cblk->numlenbits += increment;

                        pass = cblk->passes +  cblk->numpasses;
                        /* insertion of the codeword segment length */
                        for (passno = cblk->numpasses; passno < l_nb_passes; ++passno) {
                                nump++;
                                len += pass->len;

                                if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
                                        bio_write(bio, len, cblk->numlenbits + int_floorlog2(nump));
                                        len = 0;
                                        nump = 0;
                                }
                                ++pass;
                        }

                        ++cblk;
                }

                ++band;
        }

        if (bio_flush(bio)) {
                bio_destroy(bio);
                return OPJ_FALSE;               /* modified to eliminate longjmp !! */
        }

        l_nb_bytes = bio_numbytes(bio);
        c += l_nb_bytes;
        length -= l_nb_bytes;

        bio_destroy(bio);

        /* <EPH 0xff92> */
        if (tcp->csty & J2K_CP_CSTY_EPH) {
                c[0] = 255;
                c[1] = 146;
                c += 2;
                length -= 2;
        }
        /* </EPH> */

        /* << INDEX */
        /* End of packet header position. Currently only represents the distance to start of packet
           Will be updated later by incrementing with packet start value*/
        if(cstr_info && cstr_info->index_write) {
                opj_packet_info_t *info_PK = &cstr_info->tile[tileno].packet[cstr_info->packno];
                info_PK->end_ph_pos = (OPJ_INT32)(c - dest);
        }
        /* INDEX >> */

        /* Writing the packet body */
        band = res->bands;
        for (bandno = 0; bandno < res->numbands; bandno++) {
                opj_tcd_precinct_v2_t *prc = &band->precincts[precno];

                l_nb_blocks = prc->cw * prc->ch;
                cblk = prc->cblks.enc;

                for (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
                        opj_tcd_layer_t *layer = &cblk->layers[layno];

                        if (!layer->numpasses) {
                                ++cblk;
                                continue;
                        }

                        if (layer->len > length) {
                                return OPJ_FALSE;
                        }

                        memcpy(c, layer->data, layer->len);
                        cblk->numpasses += layer->numpasses;
                        c += layer->len;
                        length -= layer->len;

                        /* << INDEX */
                        if(cstr_info && cstr_info->index_write) {
                                opj_packet_info_t *info_PK = &cstr_info->tile[tileno].packet[cstr_info->packno];
                                info_PK->disto += layer->disto;
                                if (cstr_info->D_max < info_PK->disto) {
                                        cstr_info->D_max = info_PK->disto;
                                }
                        }

                        ++cblk;
                        /* INDEX >> */
                }
                ++band;
        }

        * p_data_written += (c - dest);

        return OPJ_TRUE;
}

static opj_bool t2_skip_packet(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                             opj_tcp_v2_t *p_tcp,
                                                         opj_pi_iterator_t *p_pi,
                                                         OPJ_BYTE *p_src,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *p_pack_info)
{
        opj_bool l_read_data;
        OPJ_UINT32 l_nb_bytes_read = 0;
        OPJ_UINT32 l_nb_total_bytes_read = 0;

        *p_data_read = 0;

        if (! t2_read_packet_header(p_t2,p_tile,p_tcp,p_pi,&l_read_data,p_src,&l_nb_bytes_read,p_max_length,p_pack_info)) {
                return OPJ_FALSE;
        }

        p_src += l_nb_bytes_read;
        l_nb_total_bytes_read += l_nb_bytes_read;
        p_max_length -= l_nb_bytes_read;

        /* we should read data for the packet */
        if (l_read_data) {
                l_nb_bytes_read = 0;

                if (! t2_skip_packet_data(p_t2,p_tile,p_pi,&l_nb_bytes_read,p_max_length,p_pack_info)) {
                        return OPJ_FALSE;
                }

                l_nb_total_bytes_read += l_nb_bytes_read;
        }
        *p_data_read = l_nb_total_bytes_read;

        return OPJ_TRUE;
}



static opj_bool t2_read_packet_header(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                             opj_tcp_v2_t *p_tcp,
                                                         opj_pi_iterator_t *p_pi,
                                                         opj_bool * p_is_data_present,
                                                         OPJ_BYTE *p_src_data,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *p_pack_info)
{
        /* loop */
        OPJ_UINT32 bandno, cblkno;
        OPJ_UINT32 l_nb_code_blocks;
        OPJ_UINT32 l_remaining_length;
        OPJ_UINT32 l_header_length;
        OPJ_UINT32 * l_modified_length_ptr = 00;
        OPJ_BYTE *l_current_data = p_src_data;
        opj_cp_v2_t *l_cp = p_t2->cp;
        opj_bio_t *l_bio = 00;  /* BIO component */
        opj_tcd_band_v2_t *l_band = 00;
        opj_tcd_cblk_dec_v2_t* l_cblk = 00;
        opj_tcd_resolution_v2_t* l_res = &p_tile->comps[p_pi->compno].resolutions[p_pi->resno];

        OPJ_BYTE *l_header_data = 00;
        OPJ_BYTE **l_header_data_start = 00;

        OPJ_UINT32 l_present;

        if (p_pi->layno == 0) {
                l_band = l_res->bands;

                /* reset tagtrees */
                for (bandno = 0; bandno < l_res->numbands; ++bandno) {
                        opj_tcd_precinct_v2_t *l_prc = &l_band->precincts[p_pi->precno];

                        if ( ! ((l_band->x1-l_band->x0 == 0)||(l_band->y1-l_band->y0 == 0)) ) {
                                tgt_reset(l_prc->incltree);
                                tgt_reset(l_prc->imsbtree);
                                l_cblk = l_prc->cblks.dec;

                                l_nb_code_blocks = l_prc->cw * l_prc->ch;
                                for (cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
                                        l_cblk->numsegs = 0;
                                        l_cblk->real_num_segs = 0;
                                        ++l_cblk;
                                }
                        }

                        ++l_band;
                }
        }

        /* SOP markers */

        if (p_tcp->csty & J2K_CP_CSTY_SOP) {
                if ((*l_current_data) != 0xff || (*(l_current_data + 1) != 0x91)) {
                        /* TODO opj_event_msg(t2->cinfo->event_mgr, EVT_WARNING, "Expected SOP marker\n"); */
                } else {
                        l_current_data += 6;
                }

                /** TODO : check the Nsop value */
        }

        /*
        When the marker PPT/PPM is used the packet header are store in PPT/PPM marker
        This part deal with this caracteristic
        step 1: Read packet header in the saved structure
        step 2: Return to codestream for decoding
        */

        l_bio = bio_create();
        if (! l_bio) {
                return OPJ_FALSE;
        }

        if (l_cp->ppm == 1) { /* PPM */
                l_header_data_start = &l_cp->ppm_data;
                l_header_data = *l_header_data_start;
                l_modified_length_ptr = &(l_cp->ppm_len);

        }
        else if (p_tcp->ppt == 1) { /* PPT */
                l_header_data_start = &(p_tcp->ppt_data);
                l_header_data = *l_header_data_start;
                l_modified_length_ptr = &(p_tcp->ppt_len);
        }
        else {  /* Normal Case */
                l_header_data_start = &(l_current_data);
                l_header_data = *l_header_data_start;
                l_remaining_length = p_src_data+p_max_length-l_header_data;
                l_modified_length_ptr = &(l_remaining_length);
        }

        bio_init_dec(l_bio, l_header_data,*l_modified_length_ptr);

        l_present = bio_read(l_bio, 1);
        if (!l_present) {
                bio_inalign(l_bio);
                l_header_data += bio_numbytes(l_bio);
                bio_destroy(l_bio);

                /* EPH markers */
                if (p_tcp->csty & J2K_CP_CSTY_EPH) {
                        if ((*l_header_data) != 0xff || (*(l_header_data + 1) != 0x92)) {
                                printf("Error : expected EPH marker\n");
                        } else {
                                l_header_data += 2;
                        }
                }

                l_header_length = (l_header_data - *l_header_data_start);
                *l_modified_length_ptr -= l_header_length;
                *l_header_data_start += l_header_length;

                /* << INDEX */
                /* End of packet header position. Currently only represents the distance to start of packet
                   Will be updated later by incrementing with packet start value */
                if (p_pack_info) {
                        p_pack_info->end_ph_pos = (OPJ_INT32)(l_current_data - p_src_data);
                }
                /* INDEX >> */

                * p_is_data_present = OPJ_FALSE;
                *p_data_read = l_current_data - p_src_data;
                return OPJ_TRUE;
        }

        l_band = l_res->bands;
        for (bandno = 0; bandno < l_res->numbands; ++bandno) {
                opj_tcd_precinct_v2_t *l_prc = &(l_band->precincts[p_pi->precno]);

                if ((l_band->x1-l_band->x0 == 0)||(l_band->y1-l_band->y0 == 0)) {
                        ++l_band;
                        continue;
                }

                l_nb_code_blocks = l_prc->cw * l_prc->ch;
                l_cblk = l_prc->cblks.dec;
                for (cblkno = 0; cblkno < l_nb_code_blocks; cblkno++) {
                        OPJ_UINT32 l_included,l_increment, l_segno;
                        OPJ_INT32 n;

                        /* if cblk not yet included before --> inclusion tagtree */
                        if (!l_cblk->numsegs) {
                                l_included = tgt_decode(l_bio, l_prc->incltree, cblkno, p_pi->layno + 1);
                                /* else one bit */
                        }
                        else {
                                l_included = bio_read(l_bio, 1);
                        }

                        /* if cblk not included */
                        if (!l_included) {
                                l_cblk->numnewpasses = 0;
                                ++l_cblk;
                                continue;
                        }

                        /* if cblk not yet included --> zero-bitplane tagtree */
                        if (!l_cblk->numsegs) {
                                OPJ_UINT32 i = 0;

                                while (!tgt_decode(l_bio, l_prc->imsbtree, cblkno, i)) {
                                        ++i;
                                }

                                l_cblk->numbps = l_band->numbps + 1 - i;
                                l_cblk->numlenbits = 3;
                        }

                        /* number of coding passes */
                        l_cblk->numnewpasses = t2_getnumpasses(l_bio);
                        l_increment = t2_getcommacode(l_bio);

                        /* length indicator increment */
                        l_cblk->numlenbits += l_increment;
                        l_segno = 0;

                        if (!l_cblk->numsegs) {
                                if (! t2_init_seg_v2(l_cblk, l_segno, p_tcp->tccps[p_pi->compno].cblksty, 1)) {
                                        bio_destroy(l_bio);
                                        return OPJ_FALSE;
                                }
                        }
                        else {
                                l_segno = l_cblk->numsegs - 1;
                                if (l_cblk->segs[l_segno].numpasses == l_cblk->segs[l_segno].maxpasses) {
                                        ++l_segno;
                                        if (! t2_init_seg_v2(l_cblk, l_segno, p_tcp->tccps[p_pi->compno].cblksty, 0)) {
                                                bio_destroy(l_bio);
                                                return OPJ_FALSE;
                                        }
                                }
                        }
                        n = l_cblk->numnewpasses;

                        do {
                                l_cblk->segs[l_segno].numnewpasses = int_min(l_cblk->segs[l_segno].maxpasses - l_cblk->segs[l_segno].numpasses, n);
                                l_cblk->segs[l_segno].newlen = bio_read(l_bio, l_cblk->numlenbits + uint_floorlog2(l_cblk->segs[l_segno].numnewpasses));

                                n -= l_cblk->segs[l_segno].numnewpasses;
                                if (n > 0) {
                                        ++l_segno;

                                        if (! t2_init_seg_v2(l_cblk, l_segno, p_tcp->tccps[p_pi->compno].cblksty, 0)) {
                                                bio_destroy(l_bio);
                                                return OPJ_FALSE;
                                        }
                                }
                        } while (n > 0);

                        ++l_cblk;
                }

                ++l_band;
        }

        if (bio_inalign(l_bio)) {
                bio_destroy(l_bio);
                return OPJ_FALSE;
        }

        l_header_data += bio_numbytes(l_bio);
        bio_destroy(l_bio);

        /* EPH markers */
        if (p_tcp->csty & J2K_CP_CSTY_EPH) {
                if ((*l_header_data) != 0xff || (*(l_header_data + 1) != 0x92)) {
                        /* TODO opj_event_msg(t2->cinfo->event_mgr, EVT_ERROR, "Expected EPH marker\n"); */
                } else {
                        l_header_data += 2;
                }
        }

        l_header_length = (l_header_data - *l_header_data_start);
        *l_modified_length_ptr -= l_header_length;
        *l_header_data_start += l_header_length;

        /* << INDEX */
        /* End of packet header position. Currently only represents the distance to start of packet
         Will be updated later by incrementing with packet start value */
        if (p_pack_info) {
                p_pack_info->end_ph_pos = (OPJ_INT32)(l_current_data - p_src_data);
        }
        /* INDEX >> */

        *p_is_data_present = OPJ_TRUE;
        *p_data_read = l_current_data - p_src_data;

        return OPJ_TRUE;
}

static opj_bool t2_read_packet_data(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                                                         opj_pi_iterator_t *p_pi,
                                                         OPJ_BYTE *p_src_data,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *pack_info)
{
        OPJ_UINT32 bandno, cblkno;
        OPJ_UINT32 l_nb_code_blocks;
        OPJ_BYTE *l_current_data = p_src_data;
        opj_tcd_band_v2_t *l_band = 00;
        opj_tcd_cblk_dec_v2_t* l_cblk = 00;
        opj_tcd_resolution_v2_t* l_res = &p_tile->comps[p_pi->compno].resolutions[p_pi->resno];

        l_band = l_res->bands;
        for (bandno = 0; bandno < l_res->numbands; ++bandno) {
                opj_tcd_precinct_v2_t *l_prc = &l_band->precincts[p_pi->precno];

                if ((l_band->x1-l_band->x0 == 0)||(l_band->y1-l_band->y0 == 0)) {
                        ++l_band;
                        continue;
                }

                l_nb_code_blocks = l_prc->cw * l_prc->ch;
                l_cblk = l_prc->cblks.dec;

                for (cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
                        opj_tcd_seg_t *l_seg = 00;

                        if (!l_cblk->numnewpasses) {
                                /* nothing to do */
                                ++l_cblk;
                                continue;
                        }

                        if (!l_cblk->numsegs) {
                                l_seg = l_cblk->segs;
                                ++l_cblk->numsegs;
                                l_cblk->len = 0;
                        }
                        else {
                                l_seg = &l_cblk->segs[l_cblk->numsegs - 1];

                                if (l_seg->numpasses == l_seg->maxpasses) {
                                        ++l_seg;
                                        ++l_cblk->numsegs;
                                }
                        }

                        do {
                                if (l_current_data + l_seg->newlen > p_src_data + p_max_length) {
                                        return OPJ_FALSE;
                                }

#ifdef USE_JPWL
                        /* we need here a j2k handle to verify if making a check to
                        the validity of cblocks parameters is selected from user (-W) */

                                /* let's check that we are not exceeding */
                                if ((l_cblk->len + l_seg->newlen) > 8192) {
                                        opj_event_msg(p_t2->cinfo, EVT_WARNING,
                                                "JPWL: segment too long (%d) for codeblock %d (p=%d, b=%d, r=%d, c=%d)\n",
                                                l_seg->newlen, cblkno, p_pi->precno, bandno, p_pi->resno, p_pi->compno);
                                        if (!JPWL_ASSUME) {
                                                opj_event_msg(p_t2->cinfo, EVT_ERROR, "JPWL: giving up\n");
                                                return OPJ_FALSE;
                                        }
                                        l_seg->newlen = 8192 - l_cblk->len;
                                        opj_event_msg(p_t2->cinfo, EVT_WARNING, "      - truncating segment to %d\n", l_seg->newlen);
                                        break;
                                };

#endif /* USE_JPWL */

                                if ((l_cblk->len + l_seg->newlen) > 8192) {
                                        return OPJ_FALSE;
                                }
                               
                                memcpy(l_cblk->data + l_cblk->len, l_current_data, l_seg->newlen);

                                if (l_seg->numpasses == 0) {
                                        l_seg->data = &l_cblk->data;
                                        l_seg->dataindex = l_cblk->len;
                                }

                                l_current_data += l_seg->newlen;
                                l_seg->numpasses += l_seg->numnewpasses;
                                l_cblk->numnewpasses -= l_seg->numnewpasses;

                                l_seg->real_num_passes = l_seg->numpasses;
                                l_cblk->len += l_seg->newlen;
                                l_seg->len += l_seg->newlen;

                                if (l_cblk->numnewpasses > 0) {
                                        ++l_seg;
                                        ++l_cblk->numsegs;
                                }
                        } while (l_cblk->numnewpasses > 0);

                        l_cblk->real_num_segs = l_cblk->numsegs;
                        ++l_cblk;
                }

                ++l_band;
        }

        *(p_data_read) = l_current_data - p_src_data;

        return OPJ_TRUE;
}

static opj_bool t2_skip_packet_data(
                                                         opj_t2_v2_t* p_t2,
                                                         opj_tcd_tile_v2_t *p_tile,
                                                         opj_pi_iterator_t *p_pi,
                                                         OPJ_UINT32 * p_data_read,
                                                         OPJ_UINT32 p_max_length,
                                                         opj_packet_info_t *pack_info)
{
        OPJ_UINT32 bandno, cblkno;
        OPJ_UINT32 l_nb_code_blocks;
        opj_tcd_band_v2_t *l_band = 00;
        opj_tcd_cblk_dec_v2_t* l_cblk = 00;
        opj_tcd_resolution_v2_t* l_res = &p_tile->comps[p_pi->compno].resolutions[p_pi->resno];

        *p_data_read = 0;
        l_band = l_res->bands;

        for (bandno = 0; bandno < l_res->numbands; ++bandno) {
                opj_tcd_precinct_v2_t *l_prc = &l_band->precincts[p_pi->precno];

                if ((l_band->x1-l_band->x0 == 0)||(l_band->y1-l_band->y0 == 0)) {
                        ++l_band;
                        continue;
                }

                l_nb_code_blocks = l_prc->cw * l_prc->ch;
                l_cblk = l_prc->cblks.dec;

                for (cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
                        opj_tcd_seg_t *l_seg = 00;

                        if (!l_cblk->numnewpasses) {
                                /* nothing to do */
                                ++l_cblk;
                                continue;
                        }

                        if (!l_cblk->numsegs) {
                                l_seg = l_cblk->segs;
                                ++l_cblk->numsegs;
                                l_cblk->len = 0;
                        }
                        else {
                                l_seg = &l_cblk->segs[l_cblk->numsegs - 1];

                                if (l_seg->numpasses == l_seg->maxpasses) {
                                        ++l_seg;
                                        ++l_cblk->numsegs;
                                }
                        }

                        do {
                                if (* p_data_read + l_seg->newlen > p_max_length) {
                                        return OPJ_FALSE;
                                }

#ifdef USE_JPWL
                        /* we need here a j2k handle to verify if making a check to
                        the validity of cblocks parameters is selected from user (-W) */

                                /* let's check that we are not exceeding */
                                if ((l_cblk->len + l_seg->newlen) > 8192) {
                                        opj_event_msg(p_t2->cinfo, EVT_WARNING,
                                                "JPWL: segment too long (%d) for codeblock %d (p=%d, b=%d, r=%d, c=%d)\n",
                                                l_seg->newlen, cblkno, p_pi->precno, bandno, p_pi->resno, p_pi->compno);
                                        if (!JPWL_ASSUME) {
                                                opj_event_msg(p_t2->cinfo, EVT_ERROR, "JPWL: giving up\n");
                                                return -999;
                                        }
                                        l_seg->newlen = 8192 - l_cblk->len;
                                        opj_event_msg(p_t2->cinfo, EVT_WARNING, "      - truncating segment to %d\n", l_seg->newlen);
                                        break;
                                };

#endif /* USE_JPWL */
                                *(p_data_read) += l_seg->newlen;

                                l_seg->numpasses += l_seg->numnewpasses;
                                l_cblk->numnewpasses -= l_seg->numnewpasses;
                                if (l_cblk->numnewpasses > 0)
                                {
                                        ++l_seg;
                                        ++l_cblk->numsegs;
                                }
                        } while (l_cblk->numnewpasses > 0);

                        ++l_cblk;
                }

                ++l_band;
        }

        return OPJ_TRUE;
}


static opj_bool t2_init_seg_v2(opj_tcd_cblk_dec_v2_t* cblk, OPJ_UINT32 index, OPJ_UINT32 cblksty, OPJ_UINT32 first)
{
        opj_tcd_seg_t* seg = 00;
        OPJ_UINT32 l_nb_segs = index + 1;

        if (l_nb_segs > cblk->m_current_max_segs) {
                opj_tcd_seg_t* new_segs;
                cblk->m_current_max_segs += J2K_DEFAULT_NB_SEGS;

                new_segs = (opj_tcd_seg_t*) opj_realloc(cblk->segs, cblk->m_current_max_segs * sizeof(opj_tcd_seg_t));
                if(! new_segs) {
                        opj_free(cblk->segs);
                        cblk->segs = NULL;
                        cblk->m_current_max_segs = 0;
                        /* opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory to initialize segment %d\n", l_nb_segs); */
                        return OPJ_FALSE;
                }
                cblk->segs = new_segs;
        }

        seg = &cblk->segs[index];
        memset(seg,0,sizeof(opj_tcd_seg_t));

        if (cblksty & J2K_CCP_CBLKSTY_TERMALL) {
                seg->maxpasses = 1;
        }
        else if (cblksty & J2K_CCP_CBLKSTY_LAZY) {
                if (first) {
                        seg->maxpasses = 10;
                } else {
                        seg->maxpasses = (((seg - 1)->maxpasses == 1) || ((seg - 1)->maxpasses == 10)) ? 2 : 1;
                }
        } else {
                seg->maxpasses = 109;
        }

        return OPJ_TRUE;
}
