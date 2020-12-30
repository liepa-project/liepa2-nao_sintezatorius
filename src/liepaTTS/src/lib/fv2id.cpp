/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raðtija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file fv2id.cpp
 * 
 * @author dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

//#include "StdAfx.h"
#include "fv2id.h"
#include <string.h>

char* strtokf(char*, const char*, char**);

unsigned short fv2id(char *fpav)
{
	for (int i = 0; i < FonSk; i++)
		if (strcmp(fpav, FonV[i].fv) == 0)
			return FonV[i].id;

	return FonV[0].id; // pauze "_"
}

char* id2fv(unsigned short id)
{
	for (int i = 0; i < FonSk; i++)
		if (id == FonV[i].id)
			return FonV[i].fv;

	return FonV[0].fv; // pauze "_"
}

int trText2UnitList(char *TrSakinys, unsigned short *units, unsigned short *unitseparators)
{
	char temp[500], *pos, *newpos; // turetu pakakti 480
	strcpy(temp, TrSakinys);

	int i = 0;
	pos = strtokf(temp, "+- ", &newpos);
	while (pos != NULL)
	{
		units[i] = fv2id(pos);
		pos = strtokf(NULL, "+- ", &newpos);
		if (pos == NULL) unitseparators[i] = '+';
		else unitseparators[i] = TrSakinys[pos - temp - 1];
		i++;
	}

	return i;
}
