/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raðtija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file TextNormalization.h
 * 
 * @author dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

#pragma once

int initTextNorm(char * rulesFilesDirectory, char * rulesFileName);
int normalizeText(char * buffer, char * retBuffer, int bufferSize, int * letPos);
int spellText(char * buffer, char * retBuffer, int bufferSize, int * letPos);
void unloadTextNormalization();
