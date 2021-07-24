/* #define LITTLE_ENDIAN * This should be #define'd already, if true. */
/* #define SHA1HANDSOFF * Copies data before messing with it. */

#define SHA1HANDSOFF

#include <stdio.h>
#include <string.h>

/* for uint32_t */
#include <stdint.h>
#include <stdlib.h>
#include "../h/gb_crypto.h"

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#if BYTE_ORDER == LITTLE_ENDIAN
#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) | (rol(block->l[i],8)&0x00FF00FF))
#elif BYTE_ORDER == BIG_ENDIAN
#define blk0(i) block->l[i]
#else
#error "Endianness not defined!"
#endif
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] ^block->l[(i+2)&15]^block->l[i&15],1));

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) temp=blk0(i); z+=( (w&(x^y)) ^y  )   +temp      +0x5A827999    +rol(v,5);   sprintf(g_gdb,"          x^y=%8x,w&(x^y)=%8x,(w&(x^y)) ^y=%8x rol(v,5)=%08x  rol(v,5)+0x5A827999=%08x blk0(i)=%08x rol(w,30)=%08x",x^y,w&(x^y),((w&(x^y)) ^y),rol(v,5),rol(v,5)+0x5A827999,temp,rol(w,30));w=rol(w,30);
#define R1(v,w,x,y,z,i) temp=blk(i);  z+=( (w&(x^y)) ^y  )   +temp      +0x5A827999    +rol(v,5);   sprintf(g_gdb,"          x^y=%8x,w&(x^y)=%8x,(w&(x^y)) ^y=%8x rol(v,5)=%08x  rol(v,5)+0x5A827999=%08x blk(i)=%08x  rol(w,30)=%08x",x^y,w&(x^y),((w&(x^y)) ^y),rol(v,5),rol(v,5)+0x5A827999,temp,rol(w,30));w=rol(w,30);
#define R2(v,w,x,y,z,i) temp=blk(i);  z+=(     w^x   ^y  )   +temp      +0x6ED9EBA1    +rol(v,5);   sprintf(g_gdb,"          x^y=%8x,w&(x^y)=%8x,(w&(x^y)) ^y=%8x rol(v,5)=%08x  rol(v,5)+0x6ED9EBA1=%08x blk(i)=%08x  rol(w,30)=%08x",x^y,w&(x^y),((w&(x^y)) ^y),rol(v,5),rol(v,5)+0x6ED9EBA1,temp,rol(w,30));w=rol(w,30);
#define R3(v,w,x,y,z,i) temp=blk(i);  z+=(((w|x)&y)|(w&x))   +temp      +0x8F1BBCDC    +rol(v,5);   sprintf(g_gdb,"          x^y=%8x,w&(x^y)=%8x,(w&(x^y)) ^y=%8x rol(v,5)=%08x  rol(v,5)+0x8F1BBCDC=%08x blk(i)=%08x  rol(w,30)=%08x",x^y,w&(x^y),((w&(x^y)) ^y),rol(v,5),rol(v,5)+0x8F1BBCDC,temp,rol(w,30));w=rol(w,30);
#define R4(v,w,x,y,z,i) temp=blk(i);  z+=(     w^x   ^y  )   +temp      +0xCA62C1D6    +rol(v,5);   sprintf(g_gdb,"          x^y=%8x,w&(x^y)=%8x,(w&(x^y)) ^y=%8x rol(v,5)=%08x  rol(v,5)+0xCA62C1D6=%08x blk(i)=%08x  rol(w,30)=%08x",x^y,w&(x^y),((w&(x^y)) ^y),rol(v,5),rol(v,5)+0xCA62C1D6,temp,rol(w,30));w=rol(w,30);


char g_gdb[200];
uint32_t temp;
uint32_t temp2;
uint32_t temp3;
uint32_t temp4;
uint32_t temp5;

int indentationLevel=0;
void gb_log(char *str,char filler)
{
	/*int i=0;
	int j=0;
	printf("\n");
	for(i=0;i<indentationLevel;i++)
	{
		for(j=0;j<2;j++)
		{
			printf("%c",filler);
		}
	}
	if(filler!=' ')
	{
		printf(">");
	}
	printf("%s",str);
	fflush(stdout);*/
}


void integers_permutation(uint32_t *integers)
{
	uint32_t n1,n2,n3,n4,n5;
	n1=integers[0];
	n2=integers[1];
	n3=integers[2];
	n4=integers[3];
	n5=integers[4];
	
	integers[0]=n5;
	integers[1]=n1;
	integers[2]=n2;
	integers[3]=n3;
	integers[4]=n4;
}

/* Hash a single 512-bit block. This is the core of the algorithm. */

void SHA1Transform(uint32_t state[5],const unsigned char buffer[64])
{  
    int i=0;
    char debugString[300];
    indentationLevel++;
    gb_log("SHA1Transform",'-');
    //uint32_t a, b, c, d, e;
    uint32_t integers[5];

    typedef union
    {
        unsigned char c[64];
        uint32_t l[16];
    } CHAR64LONG16;


    /*gb_log("   array as a pointer",' ');
	
	printf("\n\n*****INSIDE SHA1TRANSFORM: DATA IS *****\n");
	for(i=0;i<64;i++)
	{
		printf("[%02x '%c']",buffer[i],buffer[i]);
	}
	printf("\n\n******INSIDE SHA1TRANSFORM INITIAL STATE IS **** \n");
	printf("\n\nstate[0]=%08x state[1]=%08x state[2]=%08x state[3]=%08x state[4]=%08x",state[0],state[1],state[2],state[3],state[4]);
	fflush(stdout);*/
	
    CHAR64LONG16 block[1];      /* use array to appear as a pointer */

    memcpy(block, buffer, 64);

    /* Copy context->state[] to working vars */
    /*a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];*/
	
	integers[0]=state[0];
	integers[1]=state[1];
	integers[2]=state[2];
	integers[3]=state[3];
	integers[4]=state[4];
	
    /* 4 rounds of 20 operations each. Loop unrolled. */
	sprintf(debugString,"          START :       \e[31m [%010u,%010u,%010u,%010u,%010u] [%08x,%08x,%08x,%08x,%08x] \e[0m",integers[0],integers[1],integers[2],integers[3],integers[4],integers[0],integers[1],integers[2],integers[3],integers[4]);
    gb_log(debugString,' ');
	
    int indexColor=0;
	
	for(i=0;i<80;i++)
	{
		/*printf("\nINPUT:");
		for(jj=0;jj<64;jj++)
		{
			printf("'%c'(%02x)|",block->c[jj],block->c[jj]);
		}
		printf("\n");
		*/
		if(i<16)
		{
			indexColor=1;
			R0(integers[(4-(i%5)+1)%5],integers[(4-(i%5)+2)%5],integers[(4-(i%5)+3)%5],integers[(4-(i%5)+4)%5],integers[4-(i%5)], i);
		}
		else if(i<20)
		{
			indexColor=2;
			R1(integers[(4-(i%5)+1)%5],integers[(4-(i%5)+2)%5],integers[(4-(i%5)+3)%5],integers[(4-(i%5)+4)%5],integers[4-(i%5)], i);
		}
		else if(i<40)
		{
			indexColor=3;
			R2(integers[(4-(i%5)+1)%5],integers[(4-(i%5)+2)%5],integers[(4-(i%5)+3)%5],integers[(4-(i%5)+4)%5],integers[4-(i%5)], i);
		}
		else if(i<60)
		{
			indexColor=4;
			R3(integers[(4-(i%5)+1)%5],integers[(4-(i%5)+2)%5],integers[(4-(i%5)+3)%5],integers[(4-(i%5)+4)%5],integers[4-(i%5)], i);
		}
		else if(i<80)
		{
			indexColor=5;
			R4(integers[(4-(i%5)+1)%5],integers[(4-(i%5)+2)%5],integers[(4-(i%5)+3)%5],integers[(4-(i%5)+4)%5],integers[4-(i%5)], i);
		}
		sprintf(debugString,"          %d CHANGING N,M: \e[3%dm [%010u,%010u,%010u,%010u,%010u] [%08x,%08x,%08x,%08x,%08x] \e[0m",i+1,indexColor,integers[0],integers[1],integers[2],integers[3],integers[4],integers[0],integers[1],integers[2],integers[3],integers[4]);
		//log(debugString,' ');
	}
	
	/* Add the working vars back into context.state[] */
    state[0] += integers[0];
    state[1] += integers[1];
    state[2] += integers[2];
    state[3] += integers[3];
    state[4] += integers[4];
	sprintf(debugString,"          FINAL:        \e[31m [%010u,%010u,%010u,%010u,%010u] [%08x,%08x,%08x,%08x,%08x] \e[0m",state[0],state[1],state[2],state[3],state[4],state[0],state[1],state[2],state[3],state[4]);
    gb_log(debugString,' ');
	//printf("\n\n\n");
	//fflush(stdout);
    /* Wipe variables */
    //a = b = c = d = e = 0;
#ifdef SHA1HANDSOFF
    memset(block, '\0', sizeof(block));
#endif
    indentationLevel--;
}

/* SHA1Init - Initialize new context */

void SHA1Init(SHA1_CTX * context)
{
    indentationLevel++;
    gb_log("SHA1Init",'-');
    /* SHA1 initialization constants */
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
    indentationLevel--;
}


/* Run your data through this. */

void SHA1Update(SHA1_CTX * context,const unsigned char *data,uint32_t len)
{
    int k=0;
    indentationLevel++;
    gb_log("SHA1Update:",'-');
    //printf("data=");
    /*for(k=0;k<len;k++)
    {
	printf("'%c'[%02x]",data[k],data[k]);
    }
    if(len==1)
    {
	printf(" data[%d]='%c', len=%d",len-1, data[len-1],len);
    }*/
    uint32_t i=0;
    uint32_t j;

    j = context->count[0];
    if ((context->count[0] += (len << 3)) < j)
        context->count[1]++;
    context->count[1] += (len >> 29);
    j = (j >> 3) & 63;
    if ((j + len) > 63)
    {
	//printf("\e[33m,inside if, **********LEN=%d ********, j=%d,64-i=%d, *** i=%d, \e[0m",len, j,64-j, i);
        memcpy(&context->buffer[j], data, (i = 64 - j));
	//printf("\n\e[33m,After copying %d bytes of data\e[0m",i);
	if(len==8)
	{
		//printf("\n\t\t");
		for(k=56;k<56+len;k++)
		{
			//printf("\e[33m,buffer[%d]=%02x\e[0m",k,context->buffer[k]);
		}
	}
	else if(len==1)
	{
		//printf("buffer[%d]=%02x\e[0m",j,context->buffer[j]);
	}
        SHA1Transform(context->state, context->buffer);
        //printf("\e[33m,Checking other potential transforms in if. Variabile i=%d data='%s'\e[0m",i,data);
	for (; i + 63 < len; i += 64)
        {
            SHA1Transform(context->state, &data[i]);
	    //printf("\n");
        }
	//printf("\n\e[33m,Other potential transforms done. Variable i has become=%d\e[0m",i);
        j = 0;
    }
    else
    {
	//printf(",inside else ");
        i = 0;
    }
    memcpy(&context->buffer[j], &data[i], len - i);
    //printf(" buffer[%d]=data[%d] ('%c'[%d|%02x]) len-i=%d: context->count[0]=%d context->count[1]=%d, buffer[%d]_abs='%c'",j,i,data[i],data[i],data[i],len-i,context->count[0],context->count[1],j,context->buffer[j]);
    indentationLevel--;
}


/* Add padding and return the message digest. */
void SHA1Final(unsigned char digest[20],SHA1_CTX * context)
{
    indentationLevel++;
    gb_log("SHA1Final",'-');
    unsigned i;

    unsigned char finalcount[8];
    unsigned char c;


    for (i = 0; i < 8; i++)
    {
        finalcount[i] = (unsigned char) ((context->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);      /* Endian independent */
	//printf(" finalCount[%d]=%d",i,finalcount[i]);
    }

    gb_log("  \e[31mPADDING WITH SHA1UPDATE ARRIVING AT (512-64)bit=(64-8)bytes=56 bytes=>last element is data[55]\e[0m",' ');
    gb_log("  first byte padding is 128 dec, 80 hex, 200 octal",' ');
    c = 0200;
    SHA1Update(context, &c, 1);
    while ((context->count[0] & 504) != 448)
    {
        c = 0000;
	//printf("Call Sha1Update passing 1 byte 0x00:");
        SHA1Update(context, &c, 1);
    }
    gb_log("  \e[31mPADDING FINISHED. DIGEST CALCULATION INSIDE SHA1FINAL\e[0m\n\t",' ');
    /*for(i=0;i<64;i++)
    {
	printf("[%02x '%c']",context->buffer[i],context->buffer[i]);
    }*/
    gb_log("  \e[31mSHA1UPDATE INSIDE SHA1FINAL PASSING BIT LENGTH INSIDE LAST EIGHT BYTES\e[0m\n\t",' ');

    //printf("\n\nstate[0]=%08x state[1]=%08x state[2]=%08x state[3]=%08x state[4]=%08x",context->state[0],context->state[1],context->state[2],context->state[3],context->state[4]);
    //fflush(stdout);
	
    SHA1Update(context, finalcount, 8); /* Should cause a SHA1Transform() */
    for (i = 0; i < 20; i++)
    {
        digest[i] = (unsigned char) ((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
	
	gb_log("  wipe variables",' ');
	
    /* Wipe variables */
    memset(context, '\0', sizeof(*context));
    memset(&finalcount, '\0', sizeof(finalcount));
    indentationLevel--;
}

void gb_SHA1(char *hash_out,const char *str,int len)
{
    #if BYTE_ORDER == LITTLE_ENDIAN
    printf("Little Endian");
    #else
    printf("Big Endian");
    #endif
    indentationLevel++;
    gb_log("SHA1",'-');
    SHA1_CTX ctx;
    unsigned int ii;

    SHA1Init(&ctx);
    for (ii=0; ii<len; ii+=1)
    {
    	SHA1Update(&ctx, (const unsigned char*)str + ii, 1);
    }
    //printf("\e[32m\n********\n*************CALL SHA1FINAL\n**************\e[0m");
    //fflush(stdout);
    SHA1Final((unsigned char *)hash_out, &ctx);
    hash_out[20] = '\0';
    indentationLevel--;
}

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void gb_build_decoding_table() {

    decoding_table = malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}


void gb_base64_decoding_table_cleanup() 
{
    free(decoding_table);
}

char *gb_base64_encode(const unsigned char *data, int input_length, int *output_length) 
{
    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length+1); //+1 needed for trailing cstring 0
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


unsigned char *gb_base64_decode(const char *data,int input_length,int *output_length)
{
    if (decoding_table == NULL) gb_build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) 
    {
        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[(int)(data[i++])];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[(int)(data[i++])];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[(int)(data[i++])];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[(int)(data[i++])];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    gb_base64_decoding_table_cleanup();
    decoding_table=NULL;

    return decoded_data;
}
