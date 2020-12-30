/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raðtija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file Skiemen.cpp
 * 
 * @author dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

//#include "StdAfx.h"
#include <string.h>
#include <stdio.h>
#define Kiek 138

void skiem(char eil[], char SkPb[])
{
static const char *SkirSkiem[Kiek]={
"AA","AÀ","AE","AÆ","AË","AIE","AY","AÁ","AO","AUO","AÛ","AØ",
"ÀA","ÀÀ","ÀE","ÀÆ","ÀË","ÀI","ÀY","ÀÁ","ÀO","ÀU","ÀÛ","ÀØ",
"EA","EÀ","EE","EÆ","EË","EIE","EY","EÁ","EO","EUI","EUO","EÛ","EØ",
"ÆA","ÆÀ","ÆE","ÆÆ","ÆË","ÆI","ÆY","ÆÁ","ÆO","ÆU","ÆÛ","ÆØ",
"ËA","ËÀ","ËE","ËÆ","ËË","ËI","ËY","ËÁ","ËO","ËU","ËÛ","ËØ",
"IEI","IÆ","IË","II","IY","IÁ",
"YA","YÀ","YE","YÆ","YË","YI","YY","YÁ","YO","YU","YÛ","YØ",
"ÁA","ÁÀ","ÁE","ÁÆ","ÁË","ÁI","ÁY","ÁÁ","ÁO","ÁU","ÁÛ","ÁØ",
"OA","OÀ","OE","OÆ","OË","OIE","OY","OÁ","OO","OUO","OÛ","OØ",
"UA","UÀ","UE","UÆ","UË","UIE","UY","UÁ","UU","UÛ","UØ",
"ÛA","ÛÀ","ÛE","ÛÆ","ÛË","ÛI","ÛY","ÛÁ","ÛO","ÛU","ÛÛ","ÛØ",
"ØA","ØÀ","ØE","ØÆ","ØË","ØI","ØY","ØÁ","ØO","ØU","ØÛ","ØØ"};

int i;
char* ek;

i=strlen(SkPb)-1;

if(SkPb[i] & 1) SkPb[i]++; //eilutes pabaiga
while(i>0)
	{
	while(!strchr("AÀEÆËIYÁOUÛØ ",eil[i])&&(i>0)) i--;
	while(strchr("AÀEÆËIYÁOUÛØ",eil[i])&&(i>0)) i--;
	if((i>0)&&!(SkPb[i] & 2)&&strchr("JLMNRV",eil[i])) i--;
	if((i>1)&&!(SkPb[i] & 2)&&((eil[i]=='Z')&&(eil[i-1]=='D')
		||(eil[i]=='Þ')&&(eil[i-1]=='D')
		||(eil[i]=='H')&&(eil[i-1]=='C'))) i-=2;
	else if((i>0)&&!(SkPb[i] & 2)&&strchr("BDGKPTCÈFH",eil[i])) i--;
	if((i>0)&&!(SkPb[i] & 2)&&(strchr("SÐ",eil[i])
		||strchr("ZÞ",eil[i])&&(eil[i-1]!='D'))) i--;

	if(SkPb[i] & 1) SkPb[i]++;
	if(eil[i]==' ') {if(i>0) i--; if(SkPb[i] & 1) SkPb[i]++;}
	}

// negalimu balsiu kombinaciju priskyrimas skirtingiems skiemenims
for(i=0; i<Kiek; i++)
	{
	ek=eil-1;
	while(((ek=strstr(ek+1,SkirSkiem[i]))!=0)&&(SkPb[ek-eil] & 1))
		SkPb[ek-eil]++;
	}
}

