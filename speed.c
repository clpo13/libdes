/* crypto/des/speed.c */
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

/* 11-Sep-92 Andrew Daviel   Support for Silicon Graphics IRIX added */
/* 06-Apr-92 Luke Brennan    Support for VMS and add extra signal calls */

#ifndef MSDOS
#define TIMES
#endif

#include <stdlib.h>
#include <stdio.h>
#ifndef MSDOS
#include <unistd.h>
#else
#include <io.h>
#endif
#include <signal.h>
#ifndef VMS
#ifndef _IRIX
#include <time.h>
#endif
#ifdef TIMES
#include <sys/types.h>
#include <sys/times.h>
#endif
#else /* VMS */
#include <types.h>
struct tms {
	time_t tms_utime;
	time_t tms_stime;
	time_t tms_uchild;	/* I dunno...  */
	time_t tms_uchildsys;	/* so these names are a guess :-) */
	}
#endif
#ifndef TIMES
#include <sys/timeb.h>
#endif

#ifdef sun
#include <limits.h>
#include <sys/param.h>
#endif

#include "des.h"

/* The following if from times(3) man page.  It may need to be changed */
#ifndef HZ
#ifndef CLK_TCK
#ifndef VMS
#define HZ	100.0
#else /* VMS */
#define HZ	100.0
#endif
#else /* CLK_TCK */
#define HZ ((double)CLK_TCK)
#endif
#endif

#define BUFSIZE	((long)1024)
long run=0;

#ifndef NOPROTO
double Time_F(int s);
#else
double Time_F();
#endif

#ifdef SIGALRM
#if defined(__STDC__) || defined(sgi)
#define SIGRETTYPE void
#else
#define SIGRETTYPE int
#endif

#ifndef NOPROTO
SIGRETTYPE sig_done(int sig);
#else
SIGRETTYPE sig_done();
#endif

SIGRETTYPE sig_done(sig)
int sig;
	{
	signal(SIGALRM,sig_done);
	run=0;
#ifdef LINT
	sig=sig;
#endif
	}
#endif

#define START	0
#define STOP	1

double Time_F(s)
int s;
	{
	double ret;
#ifdef TIMES
	static struct tms tstart,tend;

	if (s == START)
		{
		times(&tstart);
		return(0);
		}
	else
		{
		times(&tend);
		ret=((double)(tend.tms_utime-tstart.tms_utime))/HZ;
		return((ret == 0.0)?1e-6:ret);
		}
#else /* !times() */
	static struct timeb tstart,tend;
	long i;

	if (s == START)
		{
		ftime(&tstart);
		return(0);
		}
	else
		{
		ftime(&tend);
		i=(long)tend.millitm-(long)tstart.millitm;
		ret=((double)(tend.time-tstart.time))+((double)i)/1000.0;
		return((ret == 0.0)?1e-6:ret);
		}
#endif
	}

int main(argc,argv)
int argc;
char **argv;
	{
	long count;
	static unsigned char buf[BUFSIZE];
	static des_cblock key ={0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0};
	static des_cblock key2={0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x12};
	static des_cblock key3={0x56,0x78,0x9a,0xbc,0xde,0xf0,0x12,0x34};
	des_key_schedule sch,sch2,sch3;
	double a,b,c,d,e;
#ifndef SIGALRM
	long ca,cb,cc,cd,ce;
#endif

#ifndef TIMES
	printf("To get the most acurate results, try to run this\n");
	printf("program when this computer is idle.\n");
#endif

	des_set_key((C_Block *)key2,sch2);
	des_set_key((C_Block *)key3,sch3);

#ifndef SIGALRM
	printf("First we calculate the approximate speed ...\n");
	des_set_key((C_Block *)key,sch);
	count=10;
	do	{
		long i;
		DES_LONG data[2];

		count*=2;
		Time_F(START);
		for (i=count; i; i--)
			des_encrypt(data,&(sch[0]),DES_ENCRYPT);
		d=Time_F(STOP);
		} while (d < 3.0);
	ca=count;
	cb=count*3;
	cc=count*3*8/BUFSIZE+1;
	cd=count*8/BUFSIZE+1;
	ce=count/20+1;
	printf("Doing set_key %ld times\n",ca);
#define COND(d)	(count != (d))
#define COUNT(d) (d)
#else
#define COND(c)	(run)
#define COUNT(d) (count)
	signal(SIGALRM,sig_done);
	printf("Doing set_key for 10 seconds\n");
	alarm(10);
#endif

	Time_F(START);
	for (count=0,run=1; COND(ca); count++)
		des_set_key((C_Block *)key,sch);
	d=Time_F(STOP);
	printf("%ld set_key's in %.2f seconds\n",count,d);
	a=((double)COUNT(ca))/d;

#ifdef SIGALRM
	printf("Doing des_encrypt's for 10 seconds\n");
	alarm(10);
#else
	printf("Doing des_encrypt %ld times\n",cb);
#endif
	Time_F(START);
	for (count=0,run=1; COND(cb); count++)
		{
		DES_LONG data[2];

		des_encrypt(data,&(sch[0]),DES_ENCRYPT);
		}
	d=Time_F(STOP);
	printf("%ld des_encrypt's in %.2f second\n",count,d);
	b=((double)COUNT(cb)*8)/d;

#ifdef SIGALRM
	printf("Doing des_cbc_encrypt on %ld byte blocks for 10 seconds\n",
		BUFSIZE);
	alarm(10);
#else
	printf("Doing des_cbc_encrypt %ld times on %ld byte blocks\n",cc,
		BUFSIZE);
#endif
	Time_F(START);
	for (count=0,run=1; COND(cc); count++)
		des_ncbc_encrypt((C_Block *)buf,(C_Block *)buf,BUFSIZE,&(sch[0]),
			(C_Block *)&(key[0]),DES_ENCRYPT);
	d=Time_F(STOP);
	printf("%ld des_cbc_encrypt's of %ld byte blocks in %.2f second\n",
		count,BUFSIZE,d);
	c=((double)COUNT(cc)*BUFSIZE)/d;

#ifdef SIGALRM
	printf("Doing des_ede_cbc_encrypt on %ld byte blocks for 10 seconds\n",
		BUFSIZE);
	alarm(10);
#else
	printf("Doing des_ede_cbc_encrypt %ld times on %ld byte blocks\n",cd,
		BUFSIZE);
#endif
	Time_F(START);
	for (count=0,run=1; COND(cd); count++)
		des_ede3_cbc_encrypt((C_Block *)buf,(C_Block *)buf,BUFSIZE,
			&(sch[0]),
			&(sch2[0]),
			&(sch3[0]),
			(C_Block *)&(key[0]),
			DES_ENCRYPT);
	d=Time_F(STOP);
	printf("%ld des_ede_cbc_encrypt's of %ld byte blocks in %.2f second\n",
		count,BUFSIZE,d);
	d=((double)COUNT(cd)*BUFSIZE)/d;

#ifdef SIGALRM
	printf("Doing crypt for 10 seconds\n");
	alarm(10);
#else
	printf("Doing crypt %ld times\n",ce);
#endif
	Time_F(START);
	for (count=0,run=1; COND(ce); count++)
		crypt("testing1","ef");
	e=Time_F(STOP);
	printf("%ld crypts in %.2f second\n",count,e);
	e=((double)COUNT(ce))/e;

	printf("set_key            per sec = %12.2f (%5.1fuS)\n",a,1.0e6/a);
	printf("DES raw ecb bytes  per sec = %12.2f (%5.1fuS)\n",b,8.0e6/b);
	printf("DES cbc bytes      per sec = %12.2f (%5.1fuS)\n",c,8.0e6/c);
	printf("DES ede cbc bytes  per sec = %12.2f (%5.1fuS)\n",d,8.0e6/d);
	printf("crypt              per sec = %12.2f (%5.1fuS)\n",e,1.0e6/e);
	exit(0);
#if defined(LINT) || defined(MSDOS)
	return(0);
#endif
	}
