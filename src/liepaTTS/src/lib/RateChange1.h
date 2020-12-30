/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raðtija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file RateChange1.h
 * 
 * @author dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

#pragma once

int change_DB_rate(char *, int, char[][4], int *, long *, short **);
int initRateChange(char *, char[][4], int *, long *, short **);
int change_phoneme_rate(int, int, unsigned int, short *, size_t);
void atlaisvinti_atminti_ir_inicializuoti();
