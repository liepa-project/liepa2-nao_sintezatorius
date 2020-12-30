/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.ra�tija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file ArKirciuoti.cpp
 * 
 * @author dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

// #include "StdAfx.h"
#include <string.h>
#define TaisSk 77

static struct NekircZod{char *kz[2]; char *ez; char *dz[2];
						int arKirc;} NkZod[TaisSk] = {
{{""}, "AR", {""}, 0},
{{""}, "ARBA", {""}, 0},
{{""}, "BET", {""}, 0},
{{""}, "IK", {""}, 0},
{{""}, "IR", {""}, 0},
{{""}, "KAD", {""}, 0},
{{""}, "KAI", {""}, 0},
{{""}, "LYG", {""}, 0},
{{""}, "NE", {"_"}, 1},
{{""}, "NE", {""}, 0},
{{""}, "NEI", {""}, 0},
{{""}, "N�", {""}, 0},
{{""}, "NORINT", {""}, 0},
{{""}, "O", {"_"}, 1},
{{""}, "O", {""}, 0},
{{""}, "TAI", {""}, 0},
{{""}, "BEI", {""}, 0},
{{""}, "BETGI", {""}, 0},
{{""}, "B�TENT", {""}, 0},
{{""}, "IDANT", {""}, 0},
{{""}, "JOG", {""}, 0},
{{""}, "NEBENT", {""}, 0},
{{""}, "NEG", {""}, 0},
{{""}, "NEGU", {""}, 0},
{{""}, "NES", {""}, 0},
{{""}, "PAKOL", {""}, 0},
{{""}, "TA�IAU", {""}, 0},
{{""}, "TAD", {""}, 0},
{{""}, "U�UOT", {""}, 0},
{{""}, "VIENOK", {""}, 0},
{{""}, "VISGI", {""}, 0},
{{""}, "BEGU", {""}, 0},
{{""}, "NORS", {""}, 0},
{{""}, "KA�I", {""}, 0},
{{""}, "TARTUM", {""}, 0},
{{""}, "TEGU", {""}, 0},
{{""}, "TEGUL", {""}, 0},
{{""}, "TIKTAI", {""}, 0},
{{""}, "BENT", {""}, 0},
{{""}, "GI", {""}, 0},
{{""}, "IT", {""}, 0},
{{""}, "JUK", {""}, 0},
{{""}, "NEBE", {"_"}, 1},
{{""}, "NEBE", {""}, 0},
{{""}, "BE", {""}, 0},
{{""}, "D�L", {""}, 0},
{{""}, "LIGI", {""}, 0},
{{""}, "NUO", {""}, 0},
{{""}, "PER", {""}, 0},
{{""}, "ANOT", {""}, 0},
{{""}, "ANT", {""}, 0},
{{""}, "I�", {""}, 0},
{{""}, "LIG", {""}, 0},
{{""}, "PASAK", {""}, 0},
{{""}, "PIRM", {""}, 0},
{{""}, "PO", {""}, 0},
{{""}, "PRIE", {""}, 0},
{{""}, "TARP", {""}, 0},
{{""}, "U�", {""}, 0},
{{""}, "U�U", {""}, 0},
{{""}, "VIDUR", {""}, 0},
{{""}, "VIR�", {""}, 0},
{{""}, "VIR�UJ", {""}, 0},
{{""}, "APIE", {""}, 0},
{{""}, "APSUK", {""}, 0},
{{""}, "�", {"_"}, 1},
{{""}, "�", {""}, 0},
{{""}, "PAS", {""}, 0},
{{""}, "PRO", {""}, 0},
{{""}, "PAGAL", {""}, 0},
{{""}, "PALEI", {""}, 0},
{{""}, "SU", {""}, 0},
{{""}, "SULIG", {""}, 0},
{{""}, "TIES", {""}, 0},
{{""}, "JAU", {""}, 0},
{{""}, "TIK", {""}, 0},
{{""}, "IKI", {""}, 0}};

int ArKirciuoti(char *kz, char * ez, char *dz)
{
int i;
for(i=0; i<TaisSk; i++)
	if((strcmp(ez, NkZod[i].ez)==0)
		&&((NkZod[i].dz[0][0]==0)||(strcmp(dz, NkZod[i].dz[0])==0)))
		return NkZod[i].arKirc;

return 1;
}
