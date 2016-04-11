/*****************************************************************************
 * Copyright (c) 2004-2009 by GuangZhou JinPeng Co. LTD.
 *
 * +------------------------------------------------------------------+
 * | This software is furnished under a license and may only be used  |
 * | and copied in accordance with the terms and conditions of  such  |
 * | a license and with the inclusion of this copyright notice. This  |
 * | software or any other copies of this software may not be provided|
 * | or otherwise made available to any other person.  The ownership  |
 * | and title of this software is not transferred.                   |
 * |                                                                  |
 * | The information in this software is subject  to change without   |
 * | any  prior notice and should not be construed as a commitment by |
 * | GuangZhou JinPeng Co. LTD.                                       |
 * |                                                                  |
 * | this code and information is provided "as is" without any        |
 * | warranty of any kind, either expressed or implied, including but |
 * | not limited to the implied warranties of merchantability and/or  |
 * | fitness for any particular purpose.                              |
 * +------------------------------------------------------------------+
 *
 *  Module name              : adpcm.c
 *
 *  Last update              :
 *
 *  Description              :
 *
 *         the ADPCM audio codec implementation file.
 *
 ****************************************************************************/
/****************************************************************************
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version 1.2, 18-Dec-92.
**
** Change log:
** - Fixed a stupid bug, where the delta was computed as
**   stepsize*code/4 in stead of stepsize*(code+0.5)/4.
** - There was an off-by-one error causing it to pick
**   an incorrect delta once in a blue moon.
** - The NODIVMUL define has been removed. Computations are now always done
**   using shifts, adds and subtracts. It turned out that, because the standard
**   is defined using shift/add/subtract, you needed bits of fixup code
**   (because the div/mul simulation using shift/add/sub made some rounding
**   errors that real div/mul don't make) and all together the resultant code
**   ran slower than just using the shifts all the time.
** - Changed some of the variable names to be more meaningful.
****************************************************************************/
/****************************************************************************
 *                      including file declaration
 ****************************************************************************/
#include <stdlib.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

#include "adpcm.h"


/****************************************************************************
 *                      data type defination
 ****************************************************************************/
typedef	int	(*func_encode)( void*, void*, int, void* );
typedef	int	(*func_decode)( void*, void*, int, void* );

typedef struct audio_codec
{
	New_Adpcm_State_Struct_t	state;
	int					mode;
	func_encode			encode;		/* encode fuction */
	func_decode			decode;     /* decode fuction */
} audio_codec;


/****************************************************************************
 *                         local variable declaration
 ****************************************************************************/
/* Intel ADPCM step variation table */
static int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};


/****************************************************************************
 *                      function declaration
 ****************************************************************************/
int ulaw2linear(int u_val);


/****************************************************************************
 *                         function defination
 ****************************************************************************/

/* 16bitsPerSample: short version */
int adpcm_coder16( short *indata, unsigned char *outdata, int len, New_Adpcm_State_Struct_t *state )
{
    int val;			/* Current input sample value */
    unsigned int delta;	/* Current adpcm output value */
    int diff;			/* Difference between val and valprev */
    int step;	        /* Stepsize */
    int valpred;		/* Predicted output value */
    int vpdiff;         /* Current change to valpred */
    int index;			/* Current step change index */
    unsigned int outputbuffer = 0;/* place to keep previous 4-bit value */
    int count = 0;      /* the number of bytes encoded */

    ((New_Adpcm_State_Struct_t *)outdata)->valprev = state->valprev;
    ((New_Adpcm_State_Struct_t *)outdata)->index = state->index;
    outdata += sizeof(New_Adpcm_State_Struct_t);


    valpred = state->valprev;
    index = state->index;
    step = stepsizeTable[index];

    while (len > 0 ) {
        /* Step 1 - compute difference with previous value */
        val = *indata++;
        diff = val - valpred;
        if( diff < 0 )
        {
            delta = 8;
            diff = 0-diff;
        }
        else
        {
            delta = 0;
        }

        /* Step 2 - Divide and clamp */
        /* Note:
        ** This code *approximately* computes:
        **    delta = diff*4/step;
        **    vpdiff = (delta+0.5)*step/4;
        ** but in shift step bits are dropped. The net result of this is
        ** that even if you have fast mul/div hardware you cannot put it to
        ** good use since the fixup would be too expensive.
        */
        vpdiff = (step >> 3);

        if ( diff >= step ) {
            delta |= 4;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if ( diff >= step  ) {
            delta |= 2;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if ( diff >= step ) {
            delta |= 1;
            vpdiff += step;
        }

        /* Phil Frisbie combined steps 3 and 4 */
        /* Step 3 - Update previous value */
        /* Step 4 - Clamp previous value to 16 bits */
        if ( (delta&8) != 0 )
        {
            valpred -= vpdiff;
            if ( valpred < -32768 )
                valpred = -32768;
        }
        else
        {
            valpred += vpdiff;
            if ( valpred > 32767 )
                valpred = 32767;
        }

        /* Step 5 - Assemble value, update index and step values */
        index += indexTable[delta];
        if ( index < 0 ) index = 0;
        else if ( index > 88 ) index = 88;
        step = stepsizeTable[index];

        /* Step 6 - Output value */
        outputbuffer = (delta << 4);

        /* Step 1 - compute difference with previous value */
        val = *indata++;
        diff = val - valpred;
        if(diff < 0)
        {
            delta = 8;
            diff = (-diff);
        }
        else
        {
            delta = 0;
        }

        /* Step 2 - Divide and clamp */
        /* Note:
        ** This code *approximately* computes:
        **    delta = diff*4/step;
        **    vpdiff = (delta+0.5)*step/4;
        ** but in shift step bits are dropped. The net result of this is
        ** that even if you have fast mul/div hardware you cannot put it to
        ** good use since the fixup would be too expensive.
        */
        vpdiff = (step >> 3);

        if ( diff >= step ) {
            delta |= 4;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if ( diff >= step  ) {
            delta |= 2;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if ( diff >= step ) {
            delta |= 1;
            vpdiff += step;
        }

        /* Phil Frisbie combined steps 3 and 4 */
        /* Step 3 - Update previous value */
        /* Step 4 - Clamp previous value to 16 bits */
        if ( (delta&8) != 0 )
        {
            valpred -= vpdiff;
            if ( valpred < -32768 )
                valpred = -32768;
        }
        else
        {
            valpred += vpdiff;
            if ( valpred > 32767 )
                valpred = 32767;
        }

        /* Step 5 - Assemble value, update index and step values */
        index += indexTable[delta];
        if ( index < 0 ) index = 0;
        else if ( index > 88 ) index = 88;
        step = stepsizeTable[index];

        /* Step 6 - Output value */
        *outdata++ = (unsigned char)(delta | outputbuffer);
        count++;
        len -= 2;
    }

    state->valprev = (short)valpred;
    state->index = index;

	/* outpur buffer have include Adpcm State struct */
    return count+sizeof(New_Adpcm_State_Struct_t);
}

int adpcm_decoder16( unsigned char *indata, short *outdata, int len, New_Adpcm_State_Struct_t *state )
{
    unsigned int delta;	/* Current adpcm output value */
    int step;	        /* Stepsize */
    int valpred;		/* Predicted value */
    int vpdiff;         /* Current change to valpred */
    int index;			/* Current step change index */
    unsigned int inputbuffer = 0;/* place to keep next 4-bit value */
    int count = 0;

	valpred = state->valprev;
    index = state->index;
    if ( index < 0 ) index = 0;
    else if ( index > 88 ) index = 88;
    step = stepsizeTable[index];

#ifdef WIN32
    _RPT2(0, "adpcm dec: %d %d\n", valpred, index );
#endif

    /* Loop unrolling by Phil Frisbie */
    /* This assumes there are ALWAYS an even number of samples */
    while ( len-- > 0 ) {

        /* Step 1 - get the delta value */
        inputbuffer = (unsigned int)*indata++;
        delta = (inputbuffer >> 4);

        /* Step 2 - Find new index value (for later) */
        index += indexTable[delta];
        if ( index < 0 ) index = 0;
        else if ( index > 88 ) index = 88;


        /* Phil Frisbie combined steps 3, 4, and 5 */
        /* Step 3 - Separate sign and magnitude */
        /* Step 4 - Compute difference and new predicted value */
        /* Step 5 - clamp output value */
        /*
        ** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
        ** in adpcm_coder.
        */
        vpdiff = step >> 3;
        if ( (delta & 4) != 0 ) vpdiff += step;
        if ( (delta & 2) != 0 ) vpdiff += step>>1;
        if ( (delta & 1) != 0 ) vpdiff += step>>2;

        if ( (delta & 8) != 0 )
        {
            valpred -= vpdiff;
            if ( valpred < -32768 )
                valpred = -32768;
        }
        else
        {
            valpred += vpdiff;
            if ( valpred > 32767 )
                valpred = 32767;
        }

        /* Step 6 - Update step value */
        step = stepsizeTable[index];

        /* Step 7 - Output value */
        *outdata++ = (short)valpred;

        /* Step 1 - get the delta value */
        delta = inputbuffer & 0xf;

        /* Step 2 - Find new index value (for later) */
        index += indexTable[delta];
        if ( index < 0 ) index = 0;
        else if ( index > 88 ) index = 88;


        /* Phil Frisbie combined steps 3, 4, and 5 */
        /* Step 3 - Separate sign and magnitude */
        /* Step 4 - Compute difference and new predicted value */
        /* Step 5 - clamp output value */
        /*
        ** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
        ** in adpcm_coder.
        */
        vpdiff = step >> 3;
        if ( (delta & 4) != 0 ) vpdiff += step;
        if ( (delta & 2) != 0 ) vpdiff += step>>1;
        if ( (delta & 1) != 0 ) vpdiff += step>>2;

        if ( (delta & 8) != 0 )
        {
            valpred -= vpdiff;
            if ( valpred < -32768 )
                valpred = -32768;
        }
        else
        {
            valpred += vpdiff;
            if ( valpred > 32767 )
                valpred = 32767;
        }

        /* Step 6 - Update step value */
        step = stepsizeTable[index];

        /* Step 7 - Output value */
        *outdata++ = (short)valpred;
        count += 2;
    }

    /*state->valprev = (short)valpred;
    state->index = index;*/

    return count;
}

#define USE_ULDECODE_BUFFER 0

/* 16bitsPerSample: short version */
int adpcm_coder16ul( const unsigned char *indata, unsigned char *outdata, int len, New_Adpcm_State_Struct_t *state )
{
    int val;			/* Current input sample value */
    unsigned int delta;	/* Current adpcm output value */
    int diff;			/* Difference between val and valprev */
    int step;	        /* Stepsize */
    int valpred;		/* Predicted output value */
    int vpdiff;         /* Current change to valpred */
    int index;			/* Current step change index */
    unsigned int outputbuffer = 0;/* place to keep previous 4-bit value */
    int count = 0;      /* the number of bytes encoded */

#if USE_ULDECODE_BUFFER
	short* temp = (short*)malloc( len*sizeof(short) );
	ulawDecode( indata, temp, len );
	indata = (const unsigned char *)temp;
#endif

	((New_Adpcm_State_Struct_t *)outdata)->valprev = state->valprev;
	((New_Adpcm_State_Struct_t *)outdata)->index = state->index;
	outdata += sizeof(New_Adpcm_State_Struct_t);

    valpred = state->valprev;
    index = (int)state->index;
    step = stepsizeTable[index];

#ifdef WIN32
    _RPT2(0, "adpcm enc: %d %d\n", valpred, index );
#endif

	while (len > 0 )
	{
        /* Step 1 - compute difference with previous value */
	#if USE_ULDECODE_BUFFER
        val = *((short *)indata)++;
	#else
        val = ulaw2linear(*indata++);
	#endif
        diff = val - valpred;
        if( diff < 0 )
        {
            delta = 8;
            diff = 0-diff;
        }
        else
        {
            delta = 0;
        }

        /* Step 2 - Divide and clamp */
        /* Note:
        ** This code *approximately* computes:
        **    delta = diff*4/step;
        **    vpdiff = (delta+0.5)*step/4;
        ** but in shift step bits are dropped. The net result of this is
        ** that even if you have fast mul/div hardware you cannot put it to
        ** good use since the fixup would be too expensive.
        */
        vpdiff = (step >> 3);

        if ( diff >= step ) {
            delta |= 4;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if ( diff >= step  ) {
            delta |= 2;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if ( diff >= step ) {
            delta |= 1;
            vpdiff += step;
        }

        /* Phil Frisbie combined steps 3 and 4 */
        /* Step 3 - Update previous value */
        /* Step 4 - Clamp previous value to 16 bits */
        if ( (delta&8) != 0 )
        {
            valpred -= vpdiff;
            if ( valpred < -32768 )
                valpred = -32768;
        }
        else
        {
            valpred += vpdiff;
            if ( valpred > 32767 )
                valpred = 32767;
        }

        /* Step 5 - Assemble value, update index and step values */
        index += indexTable[delta];
        if ( index < 0 ) index = 0;
        else if ( index > 88 ) index = 88;
        step = stepsizeTable[index];

        /* Step 6 - Output value */
        outputbuffer = (delta << 4);

        /* Step 1 - compute difference with previous value */
	#if USE_ULDECODE_BUFFER
        val = *((short *)indata)++;
	#else
        val = ulaw2linear(*indata++);
	#endif
        diff = val - valpred;
        if(diff < 0)
        {
            delta = 8;
            diff = (-diff);
        }
        else
        {
            delta = 0;
        }

        /* Step 2 - Divide and clamp */
        /* Note:
        ** This code *approximately* computes:
        **    delta = diff*4/step;
        **    vpdiff = (delta+0.5)*step/4;
        ** but in shift step bits are dropped. The net result of this is
        ** that even if you have fast mul/div hardware you cannot put it to
        ** good use since the fixup would be too expensive.
        */
        vpdiff = (step >> 3);

        if ( diff >= step ) {
            delta |= 4;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if ( diff >= step  ) {
            delta |= 2;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if ( diff >= step ) {
            delta |= 1;
            vpdiff += step;
        }

        /* Phil Frisbie combined steps 3 and 4 */
        /* Step 3 - Update previous value */
        /* Step 4 - Clamp previous value to 16 bits */
        if ( (delta&8) != 0 )
        {
            valpred -= vpdiff;
            if ( valpred < -32768 )
                valpred = -32768;
        }
        else
        {
            valpred += vpdiff;
            if ( valpred > 32767 )
                valpred = 32767;
        }

        /* Step 5 - Assemble value, update index and step values */
        index += indexTable[delta];
        if ( index < 0 ) index = 0;
        else if ( index > 88 ) index = 88;
        step = stepsizeTable[index];

        /* Step 6 - Output value */
        *outdata++ = (unsigned char)(delta | outputbuffer);
        count++;
        len -= 2;
    }

    state->valprev = (short)valpred;
    state->index = (char)index;

#if USE_ULDECODE_BUFFER
	free( temp );
#endif

	/* outpur buffer have include Adpcm State struct */
    return count+sizeof(New_Adpcm_State_Struct_t);
}

#define	BIAS		(0x84)		/* Bias for linear code. */
#define CLIP            8159
#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
int ulaw2linear( int	u_val)
{
	int t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= (u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

int audio_codec_init( void **handle, int mode )
{
	struct audio_codec *p_codec = (struct audio_codec *)malloc( sizeof( struct audio_codec) );
	if( !p_codec )
		return -1;

    p_codec->state.valprev = 0;
    p_codec->state.index = 0;
	p_codec->mode = mode;
	p_codec->decode = (func_decode)adpcm_decoder16;
	if( mode == AUDIO_U_LAW )
		p_codec->encode = (func_encode)adpcm_coder16ul;
	else
		p_codec->encode = (func_encode)adpcm_coder16;

	*handle = p_codec;

	return 0;
}

/*
 * encode 40ms: 320 samples (mono) x 8bits => 4bits x 320 = 160 Bytes
 */
int audio_enc_frame( void *handle, short *input, unsigned char *output, int len )
{
	return ((struct audio_codec*)handle)->encode( input, output, len, handle );
}

/*
 * decode 40ms: 4 bits x 320 = 160 Bytes: 320 samples x 8 bits
 */
int audio_dec_frame( void *handle, unsigned char *input, short *output, int len )
{
	int nSize = sizeof(New_Adpcm_State_Struct_t);
    return ((struct audio_codec*)handle)->decode( input+nSize, output, len-nSize, input );
}

/*******************************************************
Desc:
  free audio codec
*******************************************************/
int audio_codec_free( void *handle )
{
	struct audio_codec  *p_codec = (struct audio_codec *)handle;
	if( !p_codec )
		return -1;

	free( p_codec );

	return 0;
}



/************************************************************
 *  ADPCM ¶ÔÍâ½Ó¿Úº¯Êý
 ***********************************************************/
 
/*******************************************************
Desc:
  init audio encoder
Parms:
  mode(in): current support AUDIO_PCM_16_BIT & AUDIO_U_LAW only
  handle(out): return a handle of codec states for Encode
Return:
  -1: error
   0: success
*******************************************************/
int
ADPCM_enc_init( void **handle, int mode)
{
    return audio_codec_init( handle, mode );
}


/*******************************************************
Desc:
  encode a frame audio data
Parms:
  handle(in): a handle return by audio_codec_init()
  input(in): input audio sample data
  output(out): output encoed data, the first 8byte (sizeof Adpcm_State_Struct_t) is Adpcm State.
  len(in): input sample number
Return:
  output data size
*******************************************************/

int
ADPCM_enc_frame ( void *handle, short *input, unsigned char *output, int samples )
{
    return audio_enc_frame( handle, input, output, samples );
}

/*******************************************************
Desc:
  free audio encoder
Parms:
  handle(in): a handle return by ADPCM_enc_init()
Return:
  -1: error
   0: success
*******************************************************/

int
ADPCM_enc_free (void *handle)
{
    return audio_codec_free( handle );
}


/*******************************************************
Desc:
  init audio decoder
Parms:
  mode(in): current support AUDIO_PCM_16_BIT & AUDIO_U_LAW only
  handle(out): return a handle of codec states for Decode
Return:
  -1: error
   0: success
*******************************************************/
int
ADPCM_dec_init( void **handle, int mode)
{
    return audio_codec_init( handle, mode );
}

/*******************************************************
Desc:
  decode a frame audio data
Parms:
  handle(in): a handle return by audio_codec_init()
  input(in): input encoded audio data, the first 8byte (sizeof Adpcm_State_Struct_t) is Adpcm State.
  output(out): output decoed sample data
  len(in): input sample number
Return:
  output data size
*******************************************************/
int
ADPCM_dec_frame( void *handle, unsigned char *input, short *output, int len )
{
    return audio_dec_frame( handle, input, output, len );
}


/*******************************************************
Desc:
  free audio decoder
Parms:
  handle(in): a handle return by ADPCM_dec_init()
Return:
  -1: error
   0: success
*******************************************************/

int
ADPCM_dec_free(void *handle)
{
    return audio_codec_free(handle);
}
