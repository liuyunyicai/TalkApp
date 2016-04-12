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
 *  Module name              : adpcm.h
 *
 *  Last update              :
 *
 *  Description              :
 *
 *         the ADPCM audio codec interface header file.
 *
 ****************************************************************************/
#ifndef _ADPCM_H_
#define _ADPCM_H_


/*****************************************************************************
 *                       macro defination
 ****************************************************************************/
#define AUDIO_NOT_SUPPORTED    0
#define AUDIO_U_LAW            1
#define AUDIO_PCM_8_BIT        2
#define AUDIO_PCM_16_BIT       3
#define AUDIO_ADPCM            0x17
#define AUDIO_A_LAW            0x18


/*****************************************************************************
 *                    data type defination
 ****************************************************************************/

typedef struct
{
    int				valprev;	/* Previous output value */
    int  			index;		/* Index into stepsize table */
} New_Adpcm_State_Struct_t;


/*****************************************************************************
 *                  extern function declaration
 ****************************************************************************/

int  ADPCM_enc_init (void **handle, int mode);
int  ADPCM_enc_frame(void *handle, short *input, unsigned char *output, int len);
int  ADPCM_enc_free (void *handle);

int  ADPCM_dec_init (void **handle, int mode);
int  ADPCM_dec_frame(void *handle, unsigned char *input, short *output, int len);
int  ADPCM_dec_free (void *handle);

#endif /* _ADPCM_H_ */
