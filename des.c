/* crypto/des/des.c */
/* Copyright (C) 1995-1997 Eric Young (eay@mincom.oz.au)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@mincom.oz.au).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@mincom.oz.au).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@mincom.oz.au)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@mincom.oz.au)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef MSDOS
#include <unistd.h>
#else
#include <io.h>
#define RAND
#endif

#include <time.h>
#include "des_ver.h"

#ifdef VMS
#include <types.h>
#include <stat.h>
#else
#ifndef _IRIX
#include <sys/types.h>
#endif
#include <sys/stat.h>
#endif
#if defined(NOCONST)
#define const
#endif
#include "des.h"

#if defined(__STDC__) || defined(VMS) || defined(M_XENIX) || defined(MSDOS)
#include <string.h>
#endif

#ifdef RAND
#define random rand
#define srandom(s) srand(s)
#endif

#ifndef NOPROTO
void usage(void);
void doencryption(void);
int uufwrite(unsigned char *data, int size, unsigned int num, FILE *fp);
void uufwriteEnd(FILE *fp);
int uufread(unsigned char *out,int size,unsigned int num,FILE *fp);
int uuencode(unsigned char *in,int num,unsigned char *out);
int uudecode(unsigned char *in,int num,unsigned char *out);
#else
void usage();
void doencryption();
int uufwrite();
void uufwriteEnd();
int uufread();
int uuencode();
int uudecode();
#endif

#ifdef VMS
#define EXIT(a) exit(a&0x10000000)
#else
#define EXIT(a) exit(a)
#endif

#define BUFSIZE (8*1024)
#define VERIFY  1
#define KEYSIZ	8
#define KEYSIZB 1024 /* should hit tty line limit first :-) */
char key[KEYSIZB+1];
int do_encrypt,longk=0;
FILE *DES_IN,*DES_OUT,*CKSUM_OUT;
char uuname[200];
unsigned char uubuf[50];
int uubufnum=0;
#define INUUBUFN	(45*100)
#define OUTUUBUF	(65*100)
unsigned char b[OUTUUBUF];
unsigned char bb[300];
des_cblock cksum={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
char cksumname[200]="";

int vflag,cflag,eflag,dflag,kflag,bflag,fflag,sflag,uflag,flag3,hflag,error;

int main(argc, argv)
int argc;
char **argv;
	{
	int i;
	struct stat ins,outs;
	char *p;
	char *in=NULL,*out=NULL;

	vflag=cflag=eflag=dflag=kflag=hflag=bflag=fflag=sflag=uflag=flag3=0;
	error=0;
	memset(key,0,sizeof(key));

	for (i=1; i<argc; i++)
		{
		p=argv[i];
		if ((p[0] == '-') && (p[1] != '\0'))
			{
			p++;
			while (*p)
				{
				switch (*(p++))
					{
				case '3':
					flag3=1;
					longk=1;
					break;
				case 'c':
					cflag=1;
					strncpy(cksumname,p,200);
					p+=strlen(cksumname);
					break;
				case 'C':
					cflag=1;
					longk=1;
					strncpy(cksumname,p,200);
					p+=strlen(cksumname);
					break;
				case 'e':
					eflag=1;
					break;
				case 'v':
					vflag=1;
					break;
				case 'E':
					eflag=1;
					longk=1;
					break;
				case 'd':
					dflag=1;
					break;
				case 'D':
					dflag=1;
					longk=1;
					break;
				case 'b':
					bflag=1;
					break;
				case 'f':
					fflag=1;
					break;
				case 's':
					sflag=1;
					break;
				case 'u':
					uflag=1;
					strncpy(uuname,p,200);
					p+=strlen(uuname);
					break;
				case 'h':
					hflag=1;
					break;
				case 'k':
					kflag=1;
					if ((i+1) == argc)
						{
						fputs("must have a key with the -k option\n",stderr);
						error=1;
						}
					else
						{
						int j;

						i++;
						strncpy(key,argv[i],KEYSIZB);
						for (j=strlen(argv[i])-1; j>=0; j--)
							argv[i][j]='\0';
						}
					break;
				default:
					fprintf(stderr,"'%c' unknown flag\n",p[-1]);
					error=1;
					break;
					}
				}
			}
		else
			{
			if (in == NULL)
				in=argv[i];
			else if (out == NULL)
				out=argv[i];
			else
				error=1;
			}
		}
	if (error) usage();
	/* We either
	 * do checksum or
	 * do encrypt or
	 * do decrypt or
	 * do decrypt then ckecksum or
	 * do checksum then encrypt
	 */
	if (((eflag+dflag) == 1) || cflag)
		{
		if (eflag) do_encrypt=DES_ENCRYPT;
		if (dflag) do_encrypt=DES_DECRYPT;
		}
	else
		{
		if (vflag) 
			{
#ifndef _Windows			
			fprintf(stderr,"des(1) built with %s\n",libdes_version);
#endif			
			EXIT(1);
			}
		else usage();
		}

#ifndef _Windows			
	if (vflag) fprintf(stderr,"des(1) built with %s\n",libdes_version);
#endif			
	if (	(in != NULL) &&
		(out != NULL) &&
#ifndef MSDOS
		(stat(in,&ins) != -1) &&
		(stat(out,&outs) != -1) &&
		(ins.st_dev == outs.st_dev) &&
		(ins.st_ino == outs.st_ino))
#else /* MSDOS */
		(strcmp(in,out) == 0))
#endif
			{
			fputs("input and output file are the same\n",stderr);
			EXIT(3);
			}

	if (!kflag)
		if (des_read_pw_string(key,KEYSIZB+1,"Enter key:",eflag?VERIFY:0))
			{
			fputs("password error\n",stderr);
			EXIT(2);
			}

	if (in == NULL)
		DES_IN=stdin;
	else if ((DES_IN=fopen(in,"r")) == NULL)
		{
		perror("opening input file");
		EXIT(4);
		}

	CKSUM_OUT=stdout;
	if (out == NULL)
		{
		DES_OUT=stdout;
		CKSUM_OUT=stderr;
		}
	else if ((DES_OUT=fopen(out,"w")) == NULL)
		{
		perror("opening output file");
		EXIT(5);
		}

#ifdef MSDOS
	/* This should set the file to binary mode. */
	{
#include <fcntl.h>
	if (!(uflag && dflag))
		setmode(fileno(DES_IN),O_BINARY);
	if (!(uflag && eflag))
		setmode(fileno(DES_OUT),O_BINARY);
	}
#endif

	doencryption();
	fclose(DES_IN);
	fclose(DES_OUT);
	EXIT(0);
	}

void usage()
	{
	char **u;
	static const char *Usage[]={
"des <options> [input-file [output-file]]",
"options:",
"-v         : des(1) version number",
"-e         : encrypt using sunOS compatible user key to DES key conversion.",
"-E         : encrypt ",
"-d         : decrypt using sunOS compatible user key to DES key conversion.",
"-D         : decrypt ",
"-c[ckname] : generate a cbc_cksum using sunOS compatible user key to",
"             DES key conversion and output to ckname (stdout default,",
"             stderr if data being output on stdout).  The checksum is",
"             generated before encryption and after decryption if used",
"             in conjunction with -[eEdD].",
"-C[ckname] : generate a cbc_cksum as for -c but compatible with -[ED].",
"-k key     : use key 'key'",
"-h         : the key that is entered will be a hexidecimal number",
"             that is used directly as the des key",
"-u[uuname] : input file is uudecoded if -[dD] or output uuencoded data if -[eE]",
"             (uuname is the filename to put in the uuencode header).",
"-b         : encrypt using DES in ecb encryption mode, the defaut is cbc mode.",
"-3         : encrypt using tripple DES encryption.  This uses 2 keys",
"             generated from the input key.  If the input key is less",
"             than 8 characters long, this is equivelent to normal",
"             encryption.  Default is tripple cbc, -b makes it tripple ecb.",
NULL
};
	for (u=(char **)Usage; *u; u++)
		{
		fputs(*u,stderr);
		fputc('\n',stderr);
		}

	EXIT(1);
	}

void doencryption()
	{
#ifdef _LIBC
	extern int srandom();
	extern int random();
	extern unsigned long time();
#endif

	register int i;
	des_key_schedule ks,ks2;
	unsigned char iv[8],iv2[8];
	char *p;
	int num=0,j,k,l,rem,ll,len,last,ex=0;
	des_cblock kk,k2;
	FILE *O;
	int Exit=0;
#ifndef MSDOS
	static unsigned char buf[BUFSIZE+8],obuf[BUFSIZE+8];
#else
	static unsigned char *buf=NULL,*obuf=NULL;

	if (buf == NULL)
		{
		if (    (( buf=(unsigned char *)malloc(BUFSIZE+8)) == NULL) ||
			((obuf=(unsigned char *)malloc(BUFSIZE+8)) == NULL))
			{
			fputs("Not enough memory\n",stderr);
			Exit=10;
			goto problems;
			}
		}
#endif

	if (hflag)
		{
		j=(flag3?16:8);
		p=key;
		for (i=0; i<j; i++)
			{
			k=0;
			if ((*p <= '9') && (*p >= '0'))
				k=(*p-'0')<<4;
			else if ((*p <= 'f') && (*p >= 'a'))
				k=(*p-'a'+10)<<4;
			else if ((*p <= 'F') && (*p >= 'A'))
				k=(*p-'A'+10)<<4;
			else
				{
				fputs("Bad hex key\n",stderr);
				Exit=9;
				goto problems;
				}
			p++;
			if ((*p <= '9') && (*p >= '0'))
				k|=(*p-'0');
			else if ((*p <= 'f') && (*p >= 'a'))
				k|=(*p-'a'+10);
			else if ((*p <= 'F') && (*p >= 'A'))
				k|=(*p-'A'+10);
			else
				{
				fputs("Bad hex key\n",stderr);
				Exit=9;
				goto problems;
				}
			p++;
			if (i < 8)
				kk[i]=k;
			else
				k2[i-8]=k;
			}
		des_set_key((C_Block *)k2,ks2);
		memset(k2,0,sizeof(k2));
		}
	else if (longk || flag3)
		{
		if (flag3)
			{
			des_string_to_2keys(key,(C_Block *)kk,(C_Block *)k2);
			des_set_key((C_Block *)k2,ks2);
			memset(k2,0,sizeof(k2));
			}
		else
			des_string_to_key(key,(C_Block *)kk);
		}
	else
		for (i=0; i<KEYSIZ; i++)
			{
			l=0;
			k=key[i];
			for (j=0; j<8; j++)
				{
				if (k&1) l++;
				k>>=1;
				}
			if (l & 1)
				kk[i]=key[i]&0x7f;
			else
				kk[i]=key[i]|0x80;
			}

	des_set_key((C_Block *)kk,ks);
	memset(key,0,sizeof(key));
	memset(kk,0,sizeof(kk));
	/* woops - A bug that does not showup under unix :-( */
	memset(iv,0,sizeof(iv));
	memset(iv2,0,sizeof(iv2));

	l=1;
	rem=0;
	/* first read */
	if (eflag || (!dflag && cflag))
		{
		for (;;)
			{
			num=l=fread(&(buf[rem]),1,BUFSIZE,DES_IN);
			l+=rem;
			num+=rem;
			if (l < 0)
				{
				perror("read error");
				Exit=6;
				goto problems;
				}

			rem=l%8;
			len=l-rem;
			if (feof(DES_IN))
				{
				srandom((unsigned int)time(NULL));
				for (i=7-rem; i>0; i--)
					buf[l++]=random()&0xff;
				buf[l++]=rem;
				ex=1;
				len+=rem;
				}
			else
				l-=rem;

			if (cflag)
				{
				des_cbc_cksum((C_Block *)buf,(C_Block *)cksum,
					(long)len,ks,(C_Block *)cksum);
				if (!eflag)
					{
					if (feof(DES_IN)) break;
					else continue;
					}
				}

			if (bflag && !flag3)
				for (i=0; i<l; i+=8)
					des_ecb_encrypt(
						(des_cblock *)&(buf[i]),
						(des_cblock *)&(obuf[i]),
						ks,do_encrypt);
			else if (flag3 && bflag)
				for (i=0; i<l; i+=8)
					des_ecb2_encrypt(
						(des_cblock *)&(buf[i]),
						(des_cblock *)&(obuf[i]),
						ks,ks2,do_encrypt);
			else if (flag3 && !bflag)
				{
				char tmpbuf[8];

				if (rem) memcpy(tmpbuf,&(buf[l]),
					(unsigned int)rem);
				des_3cbc_encrypt(
					(des_cblock *)buf,(des_cblock *)obuf,
					(long)l,ks,ks2,(des_cblock *)iv,
					(des_cblock *)iv2,do_encrypt);
				if (rem) memcpy(&(buf[l]),tmpbuf,
					(unsigned int)rem);
				}
			else
				{
				des_cbc_encrypt(
					(des_cblock *)buf,(des_cblock *)obuf,
					(long)l,ks,(des_cblock *)iv,do_encrypt);
				if (l >= 8) memcpy(iv,&(obuf[l-8]),8);
				}
			if (rem) memcpy(buf,&(buf[l]),(unsigned int)rem);

			i=0;
			while (i < l)
				{
				if (uflag)
					j=uufwrite(obuf,1,(unsigned int)l-i,
						DES_OUT);
				else
					j=fwrite(obuf,1,(unsigned int)l-i,
						DES_OUT);
				if (j == -1)
					{
					perror("Write error");
					Exit=7;
					goto problems;
					}
				i+=j;
				}
			if (feof(DES_IN))
				{
				if (uflag) uufwriteEnd(DES_OUT);
				break;
				}
			}
		}
	else /* decrypt */
		{
		ex=1;
		for (;;)
			{
			if (ex) {
				if (uflag)
					l=uufread(buf,1,BUFSIZE,DES_IN);
				else
					l=fread(buf,1,BUFSIZE,DES_IN);
				ex=0;
				rem=l%8;
				l-=rem;
				}
			if (l < 0)
				{
				perror("read error");
				Exit=6;
				goto problems;
				}

			if (bflag && !flag3)
				for (i=0; i<l; i+=8)
					des_ecb_encrypt(
						(des_cblock *)&(buf[i]),
						(des_cblock *)&(obuf[i]),
						ks,do_encrypt);
			else if (flag3 && bflag)
				for (i=0; i<l; i+=8)
					des_ecb2_encrypt(
						(des_cblock *)&(buf[i]),
						(des_cblock *)&(obuf[i]),
						ks,ks2,do_encrypt);
			else if (flag3 && !bflag)
				{
				des_3cbc_encrypt(
					(des_cblock *)buf,(des_cblock *)obuf,
					(long)l,ks,ks2,(des_cblock *)iv,
					(des_cblock *)iv2,do_encrypt);
				}
			else
				{
				des_cbc_encrypt(
					(des_cblock *)buf,(des_cblock *)obuf,
				 	(long)l,ks,(des_cblock *)iv,do_encrypt);
				if (l >= 8) memcpy(iv,&(buf[l-8]),8);
				}

			if (uflag)
				ll=uufread(&(buf[rem]),1,BUFSIZE,DES_IN);
			else
				ll=fread(&(buf[rem]),1,BUFSIZE,DES_IN);
			ll+=rem;
			rem=ll%8;
			ll-=rem;
			if (feof(DES_IN) && (ll == 0))
				{
				last=obuf[l-1];

				if ((last > 7) || (last < 0))
					{
					fputs("The file was not decrypted correctly.\n",
						stderr);
					Exit=8;
					last=0;
					}
				l=l-8+last;
				}
			i=0;
			if (cflag) des_cbc_cksum((C_Block *)obuf,
				(C_Block *)cksum,(long)l/8*8,ks,
				(C_Block *)cksum);
			while (i != l)
				{
				j=fwrite(obuf,1,(unsigned int)l-i,DES_OUT);
				if (j == -1)
					{
					perror("Write error");
					Exit=7;
					goto problems;
					}
				i+=j;
				}
			l=ll;
			if ((l == 0) && feof(DES_IN)) break;
			}
		}
	if (cflag)
		{
		l=0;
		if (cksumname[0] != '\0')
			{
			if ((O=fopen(cksumname,"w")) != NULL)
				{
				CKSUM_OUT=O;
				l=1;
				}
			}
		for (i=0; i<8; i++)
			fprintf(CKSUM_OUT,"%02X",cksum[i]);
		fprintf(CKSUM_OUT,"\n");
		if (l) fclose(CKSUM_OUT);
		}
problems:
	memset(buf,0,sizeof(buf));
	memset(obuf,0,sizeof(obuf));
	memset(ks,0,sizeof(ks));
	memset(ks2,0,sizeof(ks2));
	memset(iv,0,sizeof(iv));
	memset(iv2,0,sizeof(iv2));
	memset(kk,0,sizeof(kk));
	memset(k2,0,sizeof(k2));
	memset(uubuf,0,sizeof(uubuf));
	memset(b,0,sizeof(b));
	memset(bb,0,sizeof(bb));
	memset(cksum,0,sizeof(cksum));
	if (Exit) EXIT(Exit);
	}

int uufwrite(data, size, num, fp)
unsigned char *data;
int size;
unsigned int num;
FILE *fp;
      
     /* We ignore this parameter but it should be > ~50 I believe */
   
    
	{
	int i,j,left,rem,ret=num;
	static int start=1;

	if (start)
		{
		fprintf(fp,"begin 600 %s\n",
			(uuname[0] == '\0')?"text.d":uuname);
		start=0;
		}

	if (uubufnum)
		{
		if (uubufnum+num < 45)
			{
			memcpy(&(uubuf[uubufnum]),data,(unsigned int)num);
			uubufnum+=num;
			return(num);
			}
		else
			{
			i=45-uubufnum;
			memcpy(&(uubuf[uubufnum]),data,(unsigned int)i);
			j=uuencode((unsigned char *)uubuf,45,b);
			fwrite(b,1,(unsigned int)j,fp);
			uubufnum=0;
			data+=i;
			num-=i;
			}
		}

	for (i=0; i<(((int)num)-INUUBUFN); i+=INUUBUFN)
		{
		j=uuencode(&(data[i]),INUUBUFN,b);
		fwrite(b,1,(unsigned int)j,fp);
		}
	rem=(num-i)%45;
	left=(num-i-rem);
	if (left)
		{
		j=uuencode(&(data[i]),left,b);
		fwrite(b,1,(unsigned int)j,fp);
		i+=left;
		}
	if (i != num)
		{
		memcpy(uubuf,&(data[i]),(unsigned int)rem);
		uubufnum=rem;
		}
	return(ret);
	}

void uufwriteEnd(fp)
FILE *fp;
	{
	int j;
	static const char *end=" \nend\n";

	if (uubufnum != 0)
		{
		uubuf[uubufnum]='\0';
		uubuf[uubufnum+1]='\0';
		uubuf[uubufnum+2]='\0';
		j=uuencode(uubuf,uubufnum,b);
		fwrite(b,1,(unsigned int)j,fp);
		}
	fwrite(end,1,strlen(end),fp);
	}

int uufread(out, size, num, fp)
unsigned char *out;
int size; /* should always be > ~ 60; I actually ignore this parameter :-) */
unsigned int num;
FILE *fp;
	{
	int i,j,tot;
	static int done=0;
	static int valid=0;
	static int start=1;

	if (start)
		{
		for (;;)
			{
			b[0]='\0';
			fgets((char *)b,300,fp);
			if (b[0] == '\0')
				{
				fprintf(stderr,"no 'begin' found in uuencoded input\n");
				return(-1);
				}
			if (strncmp((char *)b,"begin ",6) == 0) break;
			}
		start=0;
		}
	if (done) return(0);
	tot=0;
	if (valid)
		{
		memcpy(out,bb,(unsigned int)valid);
		tot=valid;
		valid=0;
		}
	for (;;)
		{
		b[0]='\0';
		fgets((char *)b,300,fp);
		if (b[0] == '\0') break;
		i=strlen((char *)b);
		if ((b[0] == 'e') && (b[1] == 'n') && (b[2] == 'd'))
			{
			done=1;
			while (!feof(fp))
				{
				fgets((char *)b,300,fp);
				}
			break;
			}
		i=uudecode(b,i,bb);
		if (i < 0) break;
		if ((i+tot+8) > num)
			{
			/* num to copy to make it a multiple of 8 */
			j=(num/8*8)-tot-8;
			memcpy(&(out[tot]),bb,(unsigned int)j);
			tot+=j;
			memcpy(bb,&(bb[j]),(unsigned int)i-j);
			valid=i-j;
			break;
			}
		memcpy(&(out[tot]),bb,(unsigned int)i);
		tot+=i;
		}
	return(tot);
	}

#define ccc2l(c,l)      (l =((DES_LONG)(*((c)++)))<<16, \
			 l|=((DES_LONG)(*((c)++)))<< 8, \
		 	 l|=((DES_LONG)(*((c)++))))

#define l2ccc(l,c)      (*((c)++)=(unsigned char)(((l)>>16)&0xff), \
                    *((c)++)=(unsigned char)(((l)>> 8)&0xff), \
                    *((c)++)=(unsigned char)(((l)    )&0xff))


int uuencode(in, num, out)
unsigned char *in;
int num;
unsigned char *out;
	{
	int j,i,n,tot=0;
	DES_LONG l;
	register unsigned char *p;
	p=out;

	for (j=0; j<num; j+=45)
		{
		if (j+45 > num)
			i=(num-j);
		else	i=45;
		*(p++)=i+' ';
		for (n=0; n<i; n+=3)
			{
			ccc2l(in,l);
			*(p++)=((l>>18)&0x3f)+' ';
			*(p++)=((l>>12)&0x3f)+' ';
			*(p++)=((l>> 6)&0x3f)+' ';
			*(p++)=((l    )&0x3f)+' ';
			tot+=4;
			}
		*(p++)='\n';
		tot+=2;
		}
	*p='\0';
	l=0;
	return(tot);
	}

int uudecode(in, num, out)
unsigned char *in;
int num;
unsigned char *out;
	{
	int j,i,k;
	unsigned int n=0,space=0;
	DES_LONG l;
	DES_LONG w,x,y,z;
	unsigned int blank=(unsigned int)'\n'-' ';

	for (j=0; j<num; )
		{
		n= *(in++)-' ';
		if (n == blank)
			{
			n=0;
			in--;
			}
		if (n > 60)
			{
			fprintf(stderr,"uuencoded line length too long\n");
			return(-1);
			}
		j++;

		for (i=0; i<n; j+=4,i+=3)
			{
			/* the following is for cases where spaces are
			 * removed from lines.
			 */
			if (space)
				{
				w=x=y=z=0;
				}
			else
				{
				w= *(in++)-' ';
				x= *(in++)-' ';
				y= *(in++)-' ';
				z= *(in++)-' ';
				}
			if ((w > 63) || (x > 63) || (y > 63) || (z > 63))
				{
				k=0;
				if (w == blank) k=1;
				if (x == blank) k=2;
				if (y == blank) k=3;
				if (z == blank) k=4;
				space=1;
				switch (k) {
				case 1:	w=0; in--;
				case 2: x=0; in--;
				case 3: y=0; in--;
				case 4: z=0; in--;
					break;
				case 0:
					space=0;
					fprintf(stderr,"bad uuencoded data values\n");
					w=x=y=z=0;
					return(-1);
					break;
					}
				}
			l=(w<<18)|(x<<12)|(y<< 6)|(z    );
			l2ccc(l,out);
			}
		if (*(in++) != '\n')
			{
			fprintf(stderr,"missing nl in uuencoded line\n");
			w=x=y=z=0;
			return(-1);
			}
		j++;
		}
	*out='\0';
	w=x=y=z=0;
	return(n);
	}
