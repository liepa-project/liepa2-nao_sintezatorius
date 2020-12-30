/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.ra�tija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file VeiksmaiSuFailais.cpp
 * 
 * @author dr. Gintaras Skersys (gintaras.skersys@mif.vu.lt)
 * 2020 12 28
 */

// Veiksmai su failais (nuskaito duomenis ir �ra�o rezultatus).

//#include "StdAfx.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "RateChange.h"

/*********************************************************
 * Grazina failo fp dydi arba -1, jei ivyksta klaida.
 * Jei klaidu neivyko, einamoji failo pozicija lieka nepakeista.
 * Korektiskai veikia tik failams, kuriu dydis iki LONG_MAX = 2147483647 baitu.
 ********************************************************/
long failo_dydis(FILE *fp)
{
	// isimename einamaja failo pozicija
	long buvusi_pozicija = ftell(fp); 
	if (buvusi_pozicija == -1L) return -1L;

	// nuvaziuojame i failo pabaiga
	if (fseek(fp, 0L, SEEK_END) != 0) 
	{
		//jei klaida: bandome grizti i pradine padeti
		fseek(fp, buvusi_pozicija, SEEK_SET); 
		return -1L; 
	}

	// isimename paskutine failo pozicija (failo dydi)
	int dydis = ftell(fp); 

	//griztame i pradine padeti
	if (fseek(fp, buvusi_pozicija, SEEK_SET) != 0) return -1L; 

	return dydis;
}

/*********************************************************
 * Nuskaito duomenis is signalo failo.
 * Grazina 0, jei nebuvo klaidos, ir < 0, jei buvo klaida.
 ********************************************************/
int nuskaityti_signala()
{
	FILE *signalo_failas;
	
	// atidarome binarini signalo faila skaitymui
	if((signalo_failas=fopen(signalo_failo_pavadinimas, "rb"))==NULL)
	{
		return ERROR_RATECHANGE_OPENING_DB_FILE;
	}

	// nustatome jo dydi
	long signalo_failo_dydis = failo_dydis (signalo_failas);
	if (signalo_failo_dydis == -1L)
	{
		fclose(signalo_failas);
		return ERROR_RATECHANGE_DETECTING_DB_FILE_SIZE;
	}

	if (signalo_failo_dydis%2 != 0L)
	{
		fclose(signalo_failas);
		return ERROR_RATECHANGE_UNEVEN_DB_FILE_SIZE;
	}
	
	signalo_ilgis = (size_t) (signalo_failo_dydis/2);
	
#ifdef MEMORY_MAPPED
	// gauname WinAPI suprantam� failo identifikatori�
	intptr_t signalo_os_failas = _get_osfhandle(_fileno(signalo_failas));
	if (signalo_os_failas == -1)
	{
		fclose(signalo_failas);
		return ERROR_RATECHANGE_GET_OS_FHANDLE;
	}

	// suformuojame mapping'o vard�
	// tai tur�t� leisti naudoti t� pa�i� atmit� keliuose procesuose
	char mapping_vardas[256];
	for (int i = 0; i < 256; i++)
	{
		if (signalo_failo_pavadinimas[i] == '\\')
			mapping_vardas[i] = '_';
		else
			mapping_vardas[i] = signalo_failo_pavadinimas[i];
	}

	// sukuriame mapping'� ir gauname rodykl� � failo duomenis
	signalo_failo_mapping = CreateFileMappingA((HANDLE) signalo_os_failas, NULL, PAGE_READONLY, 0, 0, mapping_vardas);
	if (signalo_failo_mapping == NULL)
	{
		fclose(signalo_failas);
		return ERROR_RATECHANGE_CREATE_FILE_MAPPING;
	}

	signalas = (short*) MapViewOfFile(signalo_failo_mapping, FILE_MAP_READ, 0, 0, 0);
	if (signalas == NULL)
	{
		fclose(signalo_failas);
		return ERROR_RATECHANGE_MAP_VIEW_OF_FILE;
	}
#else
	// iskiriame atminties signalo masyvui
	signalas = (short *) malloc ((signalo_ilgis+1) * sizeof(short));
	if(signalas == NULL)
	{
		fclose(signalo_failas);
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_DB;
	}

	// nuskaitome signalo masyva is failo
	size_t nuskaityta=fread(signalas, sizeof(short), signalo_ilgis+1, signalo_failas);
	if (ferror (signalo_failas))
	{
		fclose(signalo_failas);
		return ERROR_RATECHANGE_READING_DB_FILE;
	}
	if (! feof (signalo_failas) || nuskaityta != signalo_ilgis)
	{
		fclose(signalo_failas);
		return ERROR_RATECHANGE_UNFINISHED_READING_DB_FILE;
	}
#endif

	// uzdarome signalo faila
	fclose(signalo_failas);

	return 0;
}

/*********************************************************
 * Nuskaito duomenis i� nurodyto tekstinio failo ir sura�o � parametr� turinys. 
 * � parametr� turinio_ilgis �ra�o sura�yt� duomen� kiek� 
 * (reik�mini� duomen�, t.y. neskai�iuojant '\0').
 * Grazina 0, jei nebuvo klaidos, ir < 0, jei buvo klaida.
 ********************************************************/
int nuskaityti_faila (char * failo_pavadinimas, char ** turinys, size_t * turinio_ilgis)
{
	FILE *failas;
	
	// atidarome tekstin� faila skaitymui
	if((failas=fopen(failo_pavadinimas, "r"))==NULL)
	{
		return -1;
	}

	// nustatome failo dydi
	long failo_dydis_long = failo_dydis (failas);
	if (failo_dydis_long == -1L)
	{
		fclose(failas);
		return -2;
	}

	size_t failo_dydis = (size_t) failo_dydis_long;

	// iskiriame atminties failo turiniui
	char * failo_turinys = (char *) malloc ((failo_dydis+1) * sizeof(char));
	if(failo_turinys == NULL) 
		return -5;

	// nuskaitome fail�
	size_t nuskaityta_zenklu = fread (failo_turinys, sizeof(char), failo_dydis+1, failas);
	if (ferror (failas))
	{
		free(failo_turinys);
		fclose(failas);
		return -3;
	}
	if (! feof (failas))
	{
		free(failo_turinys);
		fclose(failas);
		return -4;
	}

	// uzdarome faila
	fclose(failas);

	// pa�ymime eilut�s pabaig�
	failo_turinys[nuskaityta_zenklu] = '\0';

	*turinys = failo_turinys;
	*turinio_ilgis = nuskaityta_zenklu;

	return 0;
}

/*********************************************************
 * Nuskaito duomenis is anotacij� failo.
 * Grazina 0, jei nebuvo klaidos, ir < 0, jei buvo klaida.
 ********************************************************/
int nuskaityti_anotacijas (char * fonemu_failo_pavadinimas, char *** fonemos1, int ** fonemu_ilgiai1, size_t * fonemu_kiekis1)
{
	// nuskaitome failo turin�
	char * fonemu_failo_turinys = NULL;
	size_t nuskaityta_zenklu = 0;
	int nepavyko = nuskaityti_faila (fonemu_failo_pavadinimas, &fonemu_failo_turinys, &nuskaityta_zenklu);

	switch (nepavyko)
	{
		case -1 : return ERROR_RATECHANGE_OPENING_DB_FON_WEIGHTS_FILE;
		case -2 : return ERROR_RATECHANGE_DETECTING_DB_FON_WEIGHTS_FILE_SIZE;
		case -3 : return ERROR_RATECHANGE_READING_DB_FON_WEIGHTS_FILE;
		case -4 : return ERROR_RATECHANGE_UNFINISHED_READING_DB_FON_WEIGHTS_FILE;
		case -5 : return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PHONEME_LIST;
	}
	
	// susiskai�iuojame, kiek fonemu failo duomenyse yra eilu�i�
	int eiluciu_skaicius = 0;
	int i;
	for (i=0; i<nuskaityta_zenklu; i++) {
		if (fonemu_failo_turinys[i] == '\n')
			eiluciu_skaicius++;
	}

	char ** fonemos = NULL;
	int * fonemu_ilgiai = NULL;
	size_t fonemu_kiekis = 0;

	// iskiriame atminties fonemu ir ju ilgiu masyvams
	// (d�l visa ko pridedam vienet�, nes paskutin� eilut� gal�jo netur�ti '\n')
	fonemos = (char **) malloc ((eiluciu_skaicius+1) * sizeof(char *));
	if(fonemos == NULL) 
	{
		free (fonemu_failo_turinys);
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PHONEME_LIST;
	}
	fonemu_ilgiai = (int *) malloc ((eiluciu_skaicius+1) * sizeof(int));
	if(fonemu_ilgiai == NULL)
	{
		free (fonemu_failo_turinys);
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PHONEME_LIST;
	}
	
	// analizuojame fonemu failo duomenis
	// duomenys eina poromis: fonemos pavadinimas ir ilgis
	char * eilute;
	char * skirtukai = "\t \n";
	i = 0;

	eilute = strtok (fonemu_failo_turinys, skirtukai); // fonemos pavadinimas

	// kol yra fonemu
	while (eilute != NULL)
	{
		// isimename fonemos pavadinima
		//fonemos[i] = eilute;
		fonemos[i] = (char *) malloc ((strlen(eilute)+1) * sizeof(char));// strlen nepriskai�iuoja '\0', tod�l +1
		if(fonemos[i] == NULL)
			{
			free (fonemu_failo_turinys);
			return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PHONEME_LIST;
			}
		strcpy (fonemos[i], eilute);

		// nuskaitome fonemos ilgi
		eilute = strtok (NULL, skirtukai); // fonemos ilgis

		if (eilute == NULL) {
			free (fonemu_failo_turinys);
			return ERROR_RATECHANGE_UNSPECIFIED_PHONEME_LENGTH;
		}
		
		// konvertuojame fonemos ilg� � int
		int n = sscanf (eilute, "%d", &fonemu_ilgiai[i]);
		if (n != 1) {
			free (fonemu_failo_turinys);
			return ERROR_RATECHANGE_NONNUMERICAL_PHONEME_LENGTH;
		}

		// nuskaitome fonemos svor�. Jo mums nereikia, tod�l nieko su juo nedarome
		eilute = strtok (NULL, skirtukai); // fonemos svoris

		// nuskaitome fonemos svor�. Jo mums nereikia, tod�l nieko su juo nedarome
		eilute = strtok (NULL, skirtukai); // fonemos svoris

		// nuskaitome fonemos svor�. Jo mums nereikia, tod�l nieko su juo nedarome
//		eilute = strtok (NULL, skirtukai); // fonemos svoris						//20180125

	   // nuskaitome iki eilutes pabaigos, nes daugiau mums nieko nereikia, tod�l nieko nedarome
		eilute = strtok(NULL, "\n"); // eilute iki pabaigos

		// nuskaitome nauja fonemos pavadinima
		eilute = strtok (NULL, skirtukai); // fonemos pavadinimas

		// padidiname skaitliuk�
		i++;
	}

	// isimename nuskaitytu fonemu kieki
	fonemu_kiekis = i;

	free (fonemu_failo_turinys);

	*fonemos1 = fonemos;
	*fonemu_ilgiai1 = fonemu_ilgiai;
	*fonemu_kiekis1 = fonemu_kiekis;

	return 0;
}

/*********************************************************
 * Nuskaito duomenis is pik� failo.
 * Grazina 0, jei nebuvo klaidos, ir < 0, jei buvo klaida.
 ********************************************************/
int nuskaityti_pikus ()
{
	// nuskaitome failo turin�
	char * piku_failo_turinys = NULL;
	size_t nuskaityta_zenklu = 0;
	int nepavyko = nuskaityti_faila (piku_failo_pavadinimas, &piku_failo_turinys, &nuskaityta_zenklu);

	switch (nepavyko)
		{
		case -1 : return ERROR_RATECHANGE_OPENING_DB_PIK_FILE;
		case -2 : return ERROR_RATECHANGE_DETECTING_DB_PIK_FILE_SIZE;
		case -3 : return ERROR_RATECHANGE_READING_DB_PIK_FILE;
		case -4 : return ERROR_RATECHANGE_UNFINISHED_READING_DB_PIK_FILE;
		case -5 : return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PIK_LIST;
		}

	// susiskai�iuojame, kiek piku failo duomenyse yra eilu�i�
	int eiluciu_skaicius = 0;
	int i;
	for (i=0; i<nuskaityta_zenklu; i++) {
		if (piku_failo_turinys[i] == '\n')
			eiluciu_skaicius++;
	}

	// iskiriame atminties piku masyvui
	// (d�l visa ko pridedam vienet�, nes paskutin� eilut� gal�jo netur�ti '\n')
	pikai = (unsigned int *) malloc ((eiluciu_skaicius+1) * sizeof(int));
	if(pikai == NULL)
	{
		free (piku_failo_turinys);
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PIK_LIST;
	}
	
	// analizuojame piku failo duomenis
	char * eilute;
	char * skirtukai = " \n";
	i = 0;
	// nuskaitome piko reik�m�
	eilute = strtok (piku_failo_turinys, skirtukai); // piko reik�m�
	// kol yra pik�
	while (eilute != NULL)
	{
		// konvertuojame piko reik�m� � int
		int n = sscanf (eilute, "%d", &pikai[i]);
		if (n != 1)
		{
			free (piku_failo_turinys);
			return ERROR_RATECHANGE_NONNUMERICAL_PIK_VALUE;
		}
		// nuskaitome nauja piko reik�m�
		eilute = strtok (NULL, skirtukai);
		// padidiname skaitliuk�
		i++;
	}

	// isimename pik� kieki
	piku_kiekis = i;

	free (piku_failo_turinys);

	return 0;
}


/*********************************************************
 * Nuskaito reikiamus duomenis is failu.
 * Grazina 0, jei nebuvo klaidos, ir -1, jei buvo klaida.
 ********************************************************/
int nuskaityti_duomenis()
{
	// nuskaitome duomenis is signalo failo
	int a = nuskaityti_signala ();
	if (a < 0) return a;

	// nuskaitome duomenis i� anotacij� failo
	a = nuskaityti_anotacijas (fonemu_failo_pavadinimas, &fonemos, &fonemu_ilgiai, &fonemu_kiekis);
	if (a < 0) return a;

	// nuskaitome duomenis i� pik� failo
	a = nuskaityti_pikus ();
	if (a < 0) return a;

	return 0;
}
