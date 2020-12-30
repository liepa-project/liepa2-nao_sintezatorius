/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raðtija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file Kircdb.h
 * 
 * @author dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

typedef struct struct_variantas {int Zodynas;
                  int GramForma;
                  int KamNr;
                  int KirtRaide;
                  int Priegaide;
                  int Prioritetas;
                  char Skiem[136];
                  char Tarpt;} variantas;

int Kirciuoti(char *Zodis, char *SkPb, variantas *Variant);
void initKircLUSS();
