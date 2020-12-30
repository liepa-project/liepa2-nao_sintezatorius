/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.ra�tija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file RateChange.cpp
 * 
 * @author dr. Gintaras Skersys (gintaras.skersys@mif.vu.lt)
 * 2020 12 28
 */

#define _CRTDBG_MAP_ALLOC

//#include "StdAfx.h"
#include <stdlib.h>
//#include <crtdbg.h>

#include <string.h>
#include "RateChange.h"

#ifdef _DEBUG
#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
#define DEBUG_CLIENTBLOCK
#endif //DEBUG

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

/*********************************************************
 * Global�s kintamieji
 ********************************************************/

// numeris pirmojo piko, esan�io einamosios fonemos prad�ioje 
// (tiksliau, pirmojo piko, nepriklausan�io prie� tai buvusiai fonemai. 
// Jis gali nepriklausyti ir einamajai, o kuriai nors tolimesnei).
int * pirmojo_piko_nr_fonemose = NULL;

// pik� skai�ius fonemoje
int * piku_skaicius_fonemose = NULL;

// fonem� prad�i� indeksai signalo masyve.
// Kai naudojami dinaminiam grei�io keitimui, nereikia atlaisvinti atminties, nes ji i�skiriama ne RateChange.dll'e.
// Kai naudojami statiniam grei�io keitimui, atmintis i�skiriama ir atlaisvinama change_DB_rate() funkcijoje.
long * fonemu_adresai = NULL;

/*********************************************************
 * atlaisvinti_atminti_ir_inicializuoti
 ********************************************************/
void atlaisvinti_atminti_ir_inicializuoti ()
{
#ifdef MEMORY_MAPPED
	if (signalas != NULL)
	{
		UnmapViewOfFile(signalas);
		signalas = NULL;
	}

	if (signalo_failo_mapping != NULL)
	{
		CloseHandle(signalo_failo_mapping);
		signalo_failo_mapping = NULL;
	}
#else
	if(signalas != NULL) {free(signalas); signalas=NULL;}
#endif
	signalo_ilgis = 0;

	if(fonemos != NULL) {
		for (size_t i=0; i < fonemu_kiekis; i++) {
			if(fonemos[i] != NULL) {free(fonemos[i]); fonemos[i]=NULL;}
		}
		free(fonemos); fonemos=NULL;
	}
	fonemu_kiekis = 0;
	if(fonemu_ilgiai != NULL) {free(fonemu_ilgiai); fonemu_ilgiai=NULL;}
	if(pikai != NULL) {free(pikai); pikai=NULL;}
	piku_kiekis = 0;
	if(skirtingos_fonemos != NULL) {free(skirtingos_fonemos); skirtingos_fonemos=NULL;}
	if(vidutiniai_fonemu_ilgiai != NULL) {free(vidutiniai_fonemu_ilgiai); vidutiniai_fonemu_ilgiai=NULL;}
	skirtingu_fonemu_kiekis = 0;
	if(pirmojo_piko_nr_fonemose != NULL) {free(pirmojo_piko_nr_fonemose); pirmojo_piko_nr_fonemose=NULL;}
	if(piku_skaicius_fonemose != NULL) {free(piku_skaicius_fonemose); piku_skaicius_fonemose=NULL;}
}

/*********************************************************
 * DllMain
 ********************************************************/
#ifdef _WINDLL
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    if(ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
		// apsitvarkome
		atlaisvinti_atminti_ir_inicializuoti ();
	}
    else if(ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		atlaisvinti_atminti_ir_inicializuoti ();
	}

    return TRUE;
}
#endif


/*********************************************************
 * Prie� kvie�iant �i� funkcij�, nuskaityti duomenis ir apskai�iuoti greitinimo_koef.
 * Gr��ina rekomenduojam� naujo signalo masyvo ilg� - �iek tiek didesn�, nei reikt� pagal greitinimo koeficient�.
 ********************************************************/
size_t rekomenduoti_naujo_signalo_masyvo_ilgi (int greitis, int tono_aukscio_pokytis) // ivertinti_naujo_signalo_ilgi ?
{
	// TODO: tur�t� atsi�velgti ir � tono_aukscio_pokytis (kaip?)

	if (greitis == 100 && tono_aukscio_pokytis == 100)
		// naujo signalo ilgis sutaps su seno
		return signalo_ilgis;

	double greitinimo_koef = (double) greitis/100;

	// d�l visa ko dar kiek padidinkime koeficient�
	double padidintas_koef = greitinimo_koef * NAUJO_SIGNALO_MASYVO_ILGIO_KOEF;

	// jei signal� reikia labai sutrumpinti, gali b�ti, kad tiek sutrumpinti nepavyks, ir signalo ilgis bus didesnis. 
	// Tokiu atveju d�l visa ko geriau i�skirkime daugiau atminties.
	if (greitis < 60)
		padidintas_koef *= NAUJO_SIGNALO_MASYVO_ILGIO_KOEF;

	return (size_t) (signalo_ilgis * padidintas_koef);
}

/*********************************************************
 * Konteksto suk�rimas ir inicializavimas turi vykti tik �ioje proced�roje, 
 * kad pakeitus konteksto sandar� (prid�jus nauj� lauk�), 
 * u�tekt� pakeisti kod� tik �ioje funkcijoje.
 ********************************************************/
void init_konteksta (struct tkontekstas * kontekstas) {
	kontekstas->fonemos_nr = 0;
	kontekstas->fonemos_pradzia = 0;
	kontekstas->fonemos_pabaiga = 0;
	kontekstas->pirmojo_piko_nr = 0;
	kontekstas->piku_sk = 0;
	kontekstas->greitinimo_koef = 1.0;
	kontekstas->einamasis_postumis = 0;
	kontekstas->einamasis_signalo_nr = 0;
	kontekstas->naujas_signalas = NULL;
	kontekstas->naujo_signalo_masyvo_ilgis = 0;
	kontekstas->einamasis_naujo_signalo_nr = 0;
	kontekstas->galima_pailginti_naujas_signalas = 0;
	kontekstas->tarpo_tarp_piku_didinimo_koef = 1.0;
	kontekstas->fonemos_klase = -1;
	kontekstas->keisti_tono_auksti = 0;
	kontekstas->keiciamu_burbulu_sk = 0;
}

/*********************************************************
Gr��ina fonemos klas�s numer� pagal fonemos pavadinimo pirm�j� raid�:
0 - turintys pik� informacij� (skardieji priebalsiai, balsiai, t.y. visi, i�skyrus x, f, p, t, k, s, S, _, r, R, z, Z, H)
1 - neturintys pik� informacijos (duslieji priebalsiai, t.y. x, f, p, t, k, s, S, _)
2 - gali tur�ti ar netur�ti pik� informacijos, tod�l gali b�ti priskirti kuriai nors i� pirm�j� dviej� klasi� - reikia papildomo tikrinimo (z, Z, h),
3 - neai�ku, k� daryti (r, R).
*********************************************************/
int fonemosKlase (struct tkontekstas * kontekstas)
{

	switch (fonemos[kontekstas->fonemos_nr][0]) {

	case 'r':
	case 'R':
		return FONEMU_KLASE_RR;

	case 'x':
	case 'f':
	case 'p':
	case 't':
	case 'k':
	case 's':
	case 'S':
	case '_':
		return FONEMU_KLASE_DUSLIEJI;

	case 'z':
	case 'Z':
	case 'h':
		if (reguliarus_pikai (kontekstas))
			return FONEMU_KLASE_SKARDIEJI;
		else
			return FONEMU_KLASE_DUSLIEJI;
	default:
		return FONEMU_KLASE_SKARDIEJI;
	}

}

/*********************************************************
 * apskaiciuoti_pirmojo_piko_nr_fonemose
 ********************************************************/
int apskaiciuoti_pirmojo_piko_nr_fonemose ()
{
	// i�skiriame atminties
	pirmojo_piko_nr_fonemose = (int *) malloc ((fonemu_kiekis+1) * sizeof(int));
	if(pirmojo_piko_nr_fonemose == NULL) 
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PIK_INFO;
	piku_skaicius_fonemose = (int *) malloc ((fonemu_kiekis+1) * sizeof(int));
	if(pirmojo_piko_nr_fonemose == NULL) 
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PIK_INFO;

	// einamosios fonemos prad�ia ir pabaiga
	unsigned int fonemos_pradzia = 0;
	unsigned int fonemos_pabaiga = 0;

	// numeris pirmojo piko, esan�io einamosios fonemos prad�ioje 
	// (tiksliau, pirmojo piko, nepriklausan�io prie� tai buvusiai fonemai. 
	// Jis gali nepriklausyti ir einamajai, o kuriai nors tolimesnei).
	int pirmojo_piko_nr = 0;

	// kiekvienai fonemai darome �tai k�:
	for (size_t fonemos_nr = 0; fonemos_nr < fonemu_kiekis; fonemos_nr++) {

		// randame fonemos pabaig�
		fonemos_pabaiga = fonemos_pradzia + fonemu_ilgiai[fonemos_nr];

		// suskai�iuojame, kiek pik� yra tarp prad�ios ir pabaigos
		int piku_sk = 0; 
		size_t i = pirmojo_piko_nr;
		while (i < piku_kiekis && pikai[i] < fonemos_pabaiga) 
			i++;
		piku_sk = (int) i - pirmojo_piko_nr;

		// �simename
		pirmojo_piko_nr_fonemose [fonemos_nr] = pirmojo_piko_nr;
		piku_skaicius_fonemose [fonemos_nr] = piku_sk;

		// atnaujiname fonemos prad�i� ir pirmojo piko nr
		fonemos_pradzia = fonemos_pabaiga;
		pirmojo_piko_nr += piku_sk;
	}

	return 0;
}

/*********************************************************
 * pakeiciam fonemos, kurios numeris yra fonemos_nr, greit� ir tono auk�t�.
 * Parametras "greitis" nurodo procentais, kiek pailginti fonem� (pavyzd�iui, 120 rei�kia pailginti 1,2 karto).
 * Parametras "tono_aukscio_pokytis" nurodo procentais, kiek paauk�tinti pagrindin� ton� 
 * (pavyzd�iui, 120 rei�kia paauk�tinti 1,2 karto: jei pagrindinis tonas buvo 100 Hz, pasidarys 120 Hz).
 * Pat� nauj� signal� �ra�o � naujas_signalas masyv� (tiksliau, prie jo prisumuoja).
 * Laikome, kad "naujas_signalas" rodo � prie� tai buvusios (jei buvo) fonemos pabaig�+1.
 *
 * Gr��ina naujo signalo ilg�, jei pavyko, ir -1, jei nepavyko (jei masyve naujas_signalas neu�teko vietos).
 ********************************************************/
int change_phoneme_rate_internal (int greitis, int tono_aukscio_pokytis, unsigned int fonemos_nr, 
						 short ** naujas_signalas, size_t * naujo_signalo_masyvo_ilgis, 
						 int galima_pailginti_naujas_signalas, unsigned int einamasis_naujo_signalo_nr)
{
	// ------------------------- Inicializuojame parametrus --------------------------------- //

	// sukuriame kontekst�
	struct tkontekstas kkontekstas;
	struct tkontekstas * kontekstas = &kkontekstas;
	init_konteksta (kontekstas);

	kontekstas->fonemos_nr = fonemos_nr;

	// inicializuojame rezultat� masyv�
	kontekstas->naujas_signalas = *naujas_signalas;
	kontekstas->naujo_signalo_masyvo_ilgis = *naujo_signalo_masyvo_ilgis;
	kontekstas->galima_pailginti_naujas_signalas = galima_pailginti_naujas_signalas;

	// gr��iname rodykles � prad�i�
	kontekstas->einamasis_signalo_nr = fonemu_adresai [fonemos_nr];
	kontekstas->einamasis_naujo_signalo_nr = einamasis_naujo_signalo_nr;

	// inicializuojam einam�j� post�m�
	kontekstas->einamasis_postumis = 0;
	
	// einamosios fonemos prad�ia ir pabaiga
	kontekstas->fonemos_pradzia = (int) fonemu_adresai [fonemos_nr];
	kontekstas->fonemos_pabaiga = kontekstas->fonemos_pradzia + fonemu_ilgiai[fonemos_nr];

	// numeris pirmojo piko, esan�io einamosios fonemos prad�ioje 
	// (tiksliau, pirmojo piko, nepriklausan�io prie� tai buvusiai fonemai. 
	// Jis gali nepriklausyti ir einamajai, o kuriai nors tolimesnei).
	kontekstas->pirmojo_piko_nr = pirmojo_piko_nr_fonemose [fonemos_nr];

	// kiek pik� yra tarp prad�ios ir pabaigos
	kontekstas->piku_sk = piku_skaicius_fonemose [fonemos_nr];
	
	// nustatome fonemos klas�
	kontekstas->fonemos_klase = fonemosKlase (kontekstas);

	// ar keisti tono auk�t�
	kontekstas->keisti_tono_auksti =
		(kontekstas->fonemos_klase == FONEMU_KLASE_SKARDIEJI || kontekstas->fonemos_klase == FONEMU_KLASE_RR)
		&& (tono_aukscio_pokytis != 100)
		&& (kontekstas->piku_sk > 1);

	// nustatome tarpo tarp pik� keitimo koeficient�
	if (kontekstas->keisti_tono_auksti)
		kontekstas->tarpo_tarp_piku_didinimo_koef = 100.0 / tono_aukscio_pokytis;
	else
		kontekstas->tarpo_tarp_piku_didinimo_koef = 1.0;

	// apskai�iuojame reikiam� greitinimo koeficient� pagal pateiktus greitinimo ir tono keitimo koeficientus
	if (kontekstas->fonemos_klase == FONEMU_KLASE_RR)
		// jei r, R, tai grei�io nekei�iame (nors tono auk�t� galime keisti), t.y. neatsi�velgiame � nurodyt� greitinimo koeficiento reik�m�
		if (kontekstas->tarpo_tarp_piku_didinimo_koef < 1)
			// jei tono auk�t� didinsime, teks keisti ir greit�, bet tik tiek, kad atstatytume fonemos ilg� � buvus�
			kontekstas->greitinimo_koef = 1 / kontekstas->tarpo_tarp_piku_didinimo_koef;
		else
			// jei tono auk�t� ma�insime (ar jo nekeisime), grei�io nekeisime (neatstatysime fonemos ilgio � buvus�)
			// (t.y. jei fonemos r, R tono auk�t� ma�insime, tai jos ilgis padid�s)
			kontekstas->greitinimo_koef = 1;
	else
		// skard�iosioms fonemoms
		kontekstas->greitinimo_koef = (((double)greitis) / 100) / kontekstas->tarpo_tarp_piku_didinimo_koef;

	// ------------------------- Euristika --------------------------------- //

	// kei�iam� (�alinam� ar dubliuojam�) burbul� skai�ius
	kontekstas->keiciamu_burbulu_sk = 0;

	// euristi�kai parinkti burbuliukus i�metimui
	euristika (kontekstas);

	// ------------------------- Apdorojame signal� --------------------------------- //

	// jei pavyko == -1, visk� stabdome, nes nepavyko i�skirti atminties
	int pavyko = 0;

	// tono auk��io keitimas: apdorojame pus� pirmo burbulo, i�lendan�i� � prie� tai buvusi� fonem�
	if (kontekstas->keisti_tono_auksti)
		pavyko = kopijuoti_signala_pradzioj (kontekstas);
	// jei nepavyko, visk� stabdome
	if (pavyko == -1)
		return DIDELIS_NEIGIAMAS_KLAIDOS_KODAS;

	// i�mesti parinktus burbuliukus, perskai�iuoti masyvus
	if (kontekstas->greitinimo_koef<1)
		pavyko = trumpinti_fonema (kontekstas);
	else
		pavyko = ilginti_fonema (kontekstas);
	// jei nepavyko, visk� stabdome
	if (pavyko == -1) {
		return DIDELIS_NEIGIAMAS_KLAIDOS_KODAS; 
	}

	// nustatome, iki kiek kopijuojame signal�
	int iki = 0;
	if (kontekstas->keisti_tono_auksti)
		// tono auk��io keitimas: kopijuosime iki paskutinio piko
		iki = pikai [kontekstas->pirmojo_piko_nr + kontekstas->piku_sk -1];
	else
		// jei nekeisime tono auk��io, kopijuosime iki fonemos pabaigos
		iki = kontekstas->fonemos_pabaiga;

	// pabaigiame nukopijuoti signalo masyv�
	// jei yra nenukopijuoto signalo
	// nukopijuojame (prisumuojame) signalo duomenis iki signalo pabaigos
	// (negalime kopijuoti su memcpy, nes prarasime jau ten esan�i� informacij�).
	// Tuo pa�iu ir atnaujiname einam�sias signal� masyv� reik�mes.
	pavyko = kopijuoti_signala (iki, kontekstas);
	// jei nepavyko, visk� stabdome
	if (pavyko == -1) {
		return DIDELIS_NEIGIAMAS_KLAIDOS_KODAS; 
	}

	// tono auk��io keitimas: apdorojame pus� paskutinio burbulo, i�lendan�i� � po to einan�i� fonem�
	if (kontekstas->keisti_tono_auksti)
		pavyko = kopijuoti_signala_pabaigoj (kontekstas);
	// jei nepavyko, visk� stabdome
	if (pavyko == -1)
		return DIDELIS_NEIGIAMAS_KLAIDOS_KODAS;

	// apskai�iuojame nauj�j� fonemos ilg�
	int naujas_fonemos_ilgis = fonemu_ilgiai[fonemos_nr] + kontekstas->einamasis_postumis;

	// atnaujiname reik�mes
	*naujas_signalas = kontekstas->naujas_signalas;
	*naujo_signalo_masyvo_ilgis = kontekstas->naujo_signalo_masyvo_ilgis;

	return naujas_fonemos_ilgis;
}

/*********************************************************
 * Nuskaitom BD is failu ir pakeiciam kalbejimo greiti
 ********************************************************/
int change_DB_rate (char *katVardas, int greitis, int tono_aukscio_pokytis, char dbfv1[][4], 
					int *dbilg1, long *dbadr1, short ** wholeinputt1)
{
	// ------------------------- Nuskaitome duomenis --------------------------------- //

	// apsitvarkome

	atlaisvinti_atminti_ir_inicializuoti ();

	// pasiruo�imas duomen� nuskaitymui

	// sudarome gars� duomen� baz�s failo pavadinim�
	strcpy (signalo_failo_pavadinimas, katVardas);
	strcat (signalo_failo_pavadinimas, "db.raw");

	// sudarome fonem� failo pavadinim�
	strcpy(fonemu_failo_pavadinimas, katVardas);
	strcat(fonemu_failo_pavadinimas, "db_fon_weights.txt");

	// sudarome pik� failo pavadinim�
	strcpy(piku_failo_pavadinimas, katVardas);
	strcat(piku_failo_pavadinimas, "db_pik.txt");

	// nuskaitome duomenis is failu (u�pildome duomen� masyvus)
	int a = nuskaityti_duomenis();
	if (a < 0) return a;

	// ------------------------- Inicializuojame parametrus --------------------------------- //

	// apskai�iuojame pagalbinius masyvus darbui su pikais
	a = apskaiciuoti_pirmojo_piko_nr_fonemose ();
	if (a < 0) return a;

	// �vertiname pailginto signalo masyvo ilg�
	size_t naujo_signalo_masyvo_ilgis = rekomenduoti_naujo_signalo_masyvo_ilgi (greitis, tono_aukscio_pokytis);

	// pa�ymime, kad masyv� naujas_signalas galima ilginti, jei nety�ia jam i�skirta per ma�ai atminties
	int galima_pailginti_naujas_signalas = 1;

	// inicializuojame rezultat� masyvus

	short * naujas_signalas = (short *) calloc (naujo_signalo_masyvo_ilgis, sizeof(short)); // kad u�pildyt� nuliais
	if (naujas_signalas == NULL)
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_CHANGED_RATE_DB; 

	// susikuriame ir u�pildome pagalbin� fonem� adres� masyv�
	
	fonemu_adresai = (long *) calloc (fonemu_kiekis+1, sizeof(long));
	if (fonemu_adresai == NULL) {
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_CHANGED_RATE_DB; 
	}

	unsigned int l;
	fonemu_adresai[0]=0;
	for(l=0; l<fonemu_kiekis; l++) {
		fonemu_adresai[l+1] = fonemu_adresai[l] + fonemu_ilgiai[l];
	}

	// nauji_fonemu_ilgiai

	nauji_fonemu_ilgiai = dbilg1;

	// ------------------------- Skai�iuojame --------------------------------- //

	unsigned int einamasis_naujo_signalo_nr = 0;

	//for (size_t fonemos_nr = 0; fonemos_nr < 10; fonemos_nr++) {
	for (unsigned int fonemos_nr = 0; fonemos_nr < fonemu_kiekis; fonemos_nr++)
	{
		// kiekvienai fonemai kvie�iame change_phoneme_rate_internal() funkcij�
		int naujas_fonemos_ilgis = change_phoneme_rate_internal (
			greitis, tono_aukscio_pokytis, fonemos_nr, 
			&naujas_signalas, &naujo_signalo_masyvo_ilgis, 
			galima_pailginti_naujas_signalas, einamasis_naujo_signalo_nr);

		// jei nepavyko, visk� stabdome
		if (naujas_fonemos_ilgis == DIDELIS_NEIGIAMAS_KLAIDOS_KODAS)
			return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_CHANGED_RATE_DB;

		// perskai�iuojame fonem� ilgi� masyv�
		nauji_fonemu_ilgiai[fonemos_nr] = naujas_fonemos_ilgis;

		// atnaujiname einamasis_naujo_signalo_nr
		einamasis_naujo_signalo_nr += naujas_fonemos_ilgis;
		
	}

	// ------------------------- pritaikome rezultatus Pijaus LithUSS'ui --------------------------------- //

	*wholeinputt1 = naujas_signalas;

	nauji_fonemu_ilgiai[fonemu_kiekis]=0;

	dbadr1[0]=0;
	for(l=0; l<fonemu_kiekis; l++) {
		dbadr1[l+1] = dbadr1[l] + nauji_fonemu_ilgiai[l];
		dbfv1[l][0] = fonemos[l][0];
		dbfv1[l][1] = fonemos[l][1];
		if (dbfv1[l][1] == 0) dbfv1[l][2] = 0; else dbfv1[l][2] = fonemos[l][2];
		if (dbfv1[l][2] == 0) dbfv1[l][3] = 0; else dbfv1[l][3] = fonemos[l][3];
	}

	// apsitvarkome
	atlaisvinti_atminti_ir_inicializuoti ();

	// turime atlaisvinti atminti �ioje funkcijoje, nes �ia suk�r�me, o dinaminiu re�imu visai nereikia atlaisvinti
	if(fonemu_adresai != NULL) {free(fonemu_adresai); fonemu_adresai=NULL;}

	return NO_ERR;
}

/*********************************************************
 * Nuskaitom BD is failu
 ********************************************************/
int initRateChange (char *katVardas, char dbfv1[][4], int *dbilg1, long *dbadr1, short ** wholeinput1)
{

	// apsitvarkome
	atlaisvinti_atminti_ir_inicializuoti ();
	
	// pasiruo�imas duomen� nuskaitymui

	// sudarome gars� duomen� baz�s failo pavadinim�
	strcpy (signalo_failo_pavadinimas, katVardas);
	strcat (signalo_failo_pavadinimas, "db.raw");

	// sudarome fonem� failo pavadinim�
	strcpy(fonemu_failo_pavadinimas, katVardas);
	strcat(fonemu_failo_pavadinimas, "db_fon_weights.txt");

	// sudarome pik� failo pavadinim�
	strcpy(piku_failo_pavadinimas, katVardas);
	strcat(piku_failo_pavadinimas, "db_pik.txt");

	// nuskaitome duomenis is failu (u�pildome duomen� masyvus)
	int a = nuskaityti_duomenis();
	if (a < 0) return a;

	// apskai�iuojame pagalbinius masyvus darbui su pikais
	a = apskaiciuoti_pirmojo_piko_nr_fonemose ();
	if (a < 0) return a;

	// �simename fonem� prad�i� indeks� masyv�. 
	// J� naudosime dinaminiam grei�io keitimui.
	// (Gal pasidaryti kopij�?)
	fonemu_adresai = dbadr1;

	// pritaikome duomenis Pijaus LithUSS'ui

	*wholeinput1 = signalas;

	dbilg1[fonemu_kiekis]=0;

	unsigned int l;
	fonemu_adresai[0]=0;
	for(l=0; l<fonemu_kiekis; l++) {
		dbilg1[l] = fonemu_ilgiai[l];
		fonemu_adresai[l+1] = fonemu_adresai[l] + fonemu_ilgiai[l];
		dbfv1[l][0] = fonemos[l][0];
		dbfv1[l][1] = fonemos[l][1];
		if (dbfv1[l][1] == 0) dbfv1[l][2] = 0; else dbfv1[l][2] = fonemos[l][2];
		if (dbfv1[l][2] == 0) dbfv1[l][3] = 0; else dbfv1[l][3] = fonemos[l][3];
	}

	return NO_ERR;
}

/*********************************************************
 * pakeiciam fonemos, kurios numeris yra fonemos_nr, greit� ir tono auk�t�.
 * Parametras "greitis" nurodo procentais, kiek pailginti fonem� (pavyzd�iui, 120 rei�kia pailginti 1,2 karto).
 * Parametras "tono_aukscio_pokytis" nurodo procentais, kiek paauk�tinti pagrindin� ton� 
 * (pavyzd�iui, 120 rei�kia paauk�tinti 1,2 karto: jei pagrindinis tonas buvo 100 Hz, pasidarys 120 Hz).
 * Pat� nauj� signal� �ra�o � naujas_signalas masyv� (tiksliau, prie jo prisumuoja).
 * Laikome, kad "naujas_signalas" rodo � prie� tai buvusios (jei buvo) fonemos pabaig�+1.
 *
 * Gr��ina naujo signalo ilg�, jei pavyko, ir -1, jei nepavyko (jei masyve naujas_signalas neu�teko vietos).
 ********************************************************/
int change_phoneme_rate (int greitis, int tono_aukscio_pokytis, unsigned int fonemos_nr, 
						 short * naujas_signalas, size_t naujo_signalo_masyvo_ilgis)
{
	int galima_pailginti_naujas_signalas = 0;
	unsigned int einamasis_naujo_signalo_nr = 0;
	int naujas_fonemos_ilgis = change_phoneme_rate_internal (greitis, tono_aukscio_pokytis, fonemos_nr, &naujas_signalas, &naujo_signalo_masyvo_ilgis, 
		galima_pailginti_naujas_signalas, einamasis_naujo_signalo_nr);
	// jei nepavyko, visk� stabdome
	if (naujas_fonemos_ilgis == DIDELIS_NEIGIAMAS_KLAIDOS_KODAS)
		return ERROR_RATECHANGE_SIGNAL_BUFFER_OVERFLOW;
	else
		return naujas_fonemos_ilgis;
}
