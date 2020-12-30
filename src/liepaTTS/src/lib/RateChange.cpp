/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raðtija.lt)
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
 * Globalûs kintamieji
 ********************************************************/

// numeris pirmojo piko, esanèio einamosios fonemos pradþioje 
// (tiksliau, pirmojo piko, nepriklausanèio prieð tai buvusiai fonemai. 
// Jis gali nepriklausyti ir einamajai, o kuriai nors tolimesnei).
int * pirmojo_piko_nr_fonemose = NULL;

// pikø skaièius fonemoje
int * piku_skaicius_fonemose = NULL;

// fonemø pradþiø indeksai signalo masyve.
// Kai naudojami dinaminiam greièio keitimui, nereikia atlaisvinti atminties, nes ji iðskiriama ne RateChange.dll'e.
// Kai naudojami statiniam greièio keitimui, atmintis iðskiriama ir atlaisvinama change_DB_rate() funkcijoje.
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
 * Prieð kvieèiant ðià funkcijà, nuskaityti duomenis ir apskaièiuoti greitinimo_koef.
 * Gràþina rekomenduojamà naujo signalo masyvo ilgá - ðiek tiek didesná, nei reiktø pagal greitinimo koeficientà.
 ********************************************************/
size_t rekomenduoti_naujo_signalo_masyvo_ilgi (int greitis, int tono_aukscio_pokytis) // ivertinti_naujo_signalo_ilgi ?
{
	// TODO: turëtø atsiþvelgti ir á tono_aukscio_pokytis (kaip?)

	if (greitis == 100 && tono_aukscio_pokytis == 100)
		// naujo signalo ilgis sutaps su seno
		return signalo_ilgis;

	double greitinimo_koef = (double) greitis/100;

	// dël visa ko dar kiek padidinkime koeficientà
	double padidintas_koef = greitinimo_koef * NAUJO_SIGNALO_MASYVO_ILGIO_KOEF;

	// jei signalà reikia labai sutrumpinti, gali bûti, kad tiek sutrumpinti nepavyks, ir signalo ilgis bus didesnis. 
	// Tokiu atveju dël visa ko geriau iðskirkime daugiau atminties.
	if (greitis < 60)
		padidintas_koef *= NAUJO_SIGNALO_MASYVO_ILGIO_KOEF;

	return (size_t) (signalo_ilgis * padidintas_koef);
}

/*********************************************************
 * Konteksto sukûrimas ir inicializavimas turi vykti tik ðioje procedûroje, 
 * kad pakeitus konteksto sandarà (pridëjus naujø laukø), 
 * uþtektø pakeisti kodà tik ðioje funkcijoje.
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
Gràþina fonemos klasës numerá pagal fonemos pavadinimo pirmàjà raidæ:
0 - turintys pikø informacijà (skardieji priebalsiai, balsiai, t.y. visi, iðskyrus x, f, p, t, k, s, S, _, r, R, z, Z, H)
1 - neturintys pikø informacijos (duslieji priebalsiai, t.y. x, f, p, t, k, s, S, _)
2 - gali turëti ar neturëti pikø informacijos, todël gali bûti priskirti kuriai nors ið pirmøjø dviejø klasiø - reikia papildomo tikrinimo (z, Z, h),
3 - neaiðku, kà daryti (r, R).
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
	// iðskiriame atminties
	pirmojo_piko_nr_fonemose = (int *) malloc ((fonemu_kiekis+1) * sizeof(int));
	if(pirmojo_piko_nr_fonemose == NULL) 
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PIK_INFO;
	piku_skaicius_fonemose = (int *) malloc ((fonemu_kiekis+1) * sizeof(int));
	if(pirmojo_piko_nr_fonemose == NULL) 
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_PIK_INFO;

	// einamosios fonemos pradþia ir pabaiga
	unsigned int fonemos_pradzia = 0;
	unsigned int fonemos_pabaiga = 0;

	// numeris pirmojo piko, esanèio einamosios fonemos pradþioje 
	// (tiksliau, pirmojo piko, nepriklausanèio prieð tai buvusiai fonemai. 
	// Jis gali nepriklausyti ir einamajai, o kuriai nors tolimesnei).
	int pirmojo_piko_nr = 0;

	// kiekvienai fonemai darome ðtai kà:
	for (size_t fonemos_nr = 0; fonemos_nr < fonemu_kiekis; fonemos_nr++) {

		// randame fonemos pabaigà
		fonemos_pabaiga = fonemos_pradzia + fonemu_ilgiai[fonemos_nr];

		// suskaièiuojame, kiek pikø yra tarp pradþios ir pabaigos
		int piku_sk = 0; 
		size_t i = pirmojo_piko_nr;
		while (i < piku_kiekis && pikai[i] < fonemos_pabaiga) 
			i++;
		piku_sk = (int) i - pirmojo_piko_nr;

		// ásimename
		pirmojo_piko_nr_fonemose [fonemos_nr] = pirmojo_piko_nr;
		piku_skaicius_fonemose [fonemos_nr] = piku_sk;

		// atnaujiname fonemos pradþià ir pirmojo piko nr
		fonemos_pradzia = fonemos_pabaiga;
		pirmojo_piko_nr += piku_sk;
	}

	return 0;
}

/*********************************************************
 * pakeiciam fonemos, kurios numeris yra fonemos_nr, greitá ir tono aukðtá.
 * Parametras "greitis" nurodo procentais, kiek pailginti fonemà (pavyzdþiui, 120 reiðkia pailginti 1,2 karto).
 * Parametras "tono_aukscio_pokytis" nurodo procentais, kiek paaukðtinti pagrindiná tonà 
 * (pavyzdþiui, 120 reiðkia paaukðtinti 1,2 karto: jei pagrindinis tonas buvo 100 Hz, pasidarys 120 Hz).
 * Patá naujà signalà áraðo á naujas_signalas masyvà (tiksliau, prie jo prisumuoja).
 * Laikome, kad "naujas_signalas" rodo á prieð tai buvusios (jei buvo) fonemos pabaigà+1.
 *
 * Gràþina naujo signalo ilgá, jei pavyko, ir -1, jei nepavyko (jei masyve naujas_signalas neuþteko vietos).
 ********************************************************/
int change_phoneme_rate_internal (int greitis, int tono_aukscio_pokytis, unsigned int fonemos_nr, 
						 short ** naujas_signalas, size_t * naujo_signalo_masyvo_ilgis, 
						 int galima_pailginti_naujas_signalas, unsigned int einamasis_naujo_signalo_nr)
{
	// ------------------------- Inicializuojame parametrus --------------------------------- //

	// sukuriame kontekstà
	struct tkontekstas kkontekstas;
	struct tkontekstas * kontekstas = &kkontekstas;
	init_konteksta (kontekstas);

	kontekstas->fonemos_nr = fonemos_nr;

	// inicializuojame rezultatø masyvà
	kontekstas->naujas_signalas = *naujas_signalas;
	kontekstas->naujo_signalo_masyvo_ilgis = *naujo_signalo_masyvo_ilgis;
	kontekstas->galima_pailginti_naujas_signalas = galima_pailginti_naujas_signalas;

	// gràþiname rodykles á pradþià
	kontekstas->einamasis_signalo_nr = fonemu_adresai [fonemos_nr];
	kontekstas->einamasis_naujo_signalo_nr = einamasis_naujo_signalo_nr;

	// inicializuojam einamàjá postûmá
	kontekstas->einamasis_postumis = 0;
	
	// einamosios fonemos pradþia ir pabaiga
	kontekstas->fonemos_pradzia = (int) fonemu_adresai [fonemos_nr];
	kontekstas->fonemos_pabaiga = kontekstas->fonemos_pradzia + fonemu_ilgiai[fonemos_nr];

	// numeris pirmojo piko, esanèio einamosios fonemos pradþioje 
	// (tiksliau, pirmojo piko, nepriklausanèio prieð tai buvusiai fonemai. 
	// Jis gali nepriklausyti ir einamajai, o kuriai nors tolimesnei).
	kontekstas->pirmojo_piko_nr = pirmojo_piko_nr_fonemose [fonemos_nr];

	// kiek pikø yra tarp pradþios ir pabaigos
	kontekstas->piku_sk = piku_skaicius_fonemose [fonemos_nr];
	
	// nustatome fonemos klasæ
	kontekstas->fonemos_klase = fonemosKlase (kontekstas);

	// ar keisti tono aukðtá
	kontekstas->keisti_tono_auksti =
		(kontekstas->fonemos_klase == FONEMU_KLASE_SKARDIEJI || kontekstas->fonemos_klase == FONEMU_KLASE_RR)
		&& (tono_aukscio_pokytis != 100)
		&& (kontekstas->piku_sk > 1);

	// nustatome tarpo tarp pikø keitimo koeficientà
	if (kontekstas->keisti_tono_auksti)
		kontekstas->tarpo_tarp_piku_didinimo_koef = 100.0 / tono_aukscio_pokytis;
	else
		kontekstas->tarpo_tarp_piku_didinimo_koef = 1.0;

	// apskaièiuojame reikiamà greitinimo koeficientà pagal pateiktus greitinimo ir tono keitimo koeficientus
	if (kontekstas->fonemos_klase == FONEMU_KLASE_RR)
		// jei r, R, tai greièio nekeièiame (nors tono aukðtá galime keisti), t.y. neatsiþvelgiame á nurodytà greitinimo koeficiento reikðmæ
		if (kontekstas->tarpo_tarp_piku_didinimo_koef < 1)
			// jei tono aukðtá didinsime, teks keisti ir greitá, bet tik tiek, kad atstatytume fonemos ilgá á buvusá
			kontekstas->greitinimo_koef = 1 / kontekstas->tarpo_tarp_piku_didinimo_koef;
		else
			// jei tono aukðtá maþinsime (ar jo nekeisime), greièio nekeisime (neatstatysime fonemos ilgio á buvusá)
			// (t.y. jei fonemos r, R tono aukðtá maþinsime, tai jos ilgis padidës)
			kontekstas->greitinimo_koef = 1;
	else
		// skardþiosioms fonemoms
		kontekstas->greitinimo_koef = (((double)greitis) / 100) / kontekstas->tarpo_tarp_piku_didinimo_koef;

	// ------------------------- Euristika --------------------------------- //

	// keièiamø (ðalinamø ar dubliuojamø) burbulø skaièius
	kontekstas->keiciamu_burbulu_sk = 0;

	// euristiðkai parinkti burbuliukus iðmetimui
	euristika (kontekstas);

	// ------------------------- Apdorojame signalà --------------------------------- //

	// jei pavyko == -1, viskà stabdome, nes nepavyko iðskirti atminties
	int pavyko = 0;

	// tono aukðèio keitimas: apdorojame pusæ pirmo burbulo, iðlendanèià á prieð tai buvusià fonemà
	if (kontekstas->keisti_tono_auksti)
		pavyko = kopijuoti_signala_pradzioj (kontekstas);
	// jei nepavyko, viskà stabdome
	if (pavyko == -1)
		return DIDELIS_NEIGIAMAS_KLAIDOS_KODAS;

	// iðmesti parinktus burbuliukus, perskaièiuoti masyvus
	if (kontekstas->greitinimo_koef<1)
		pavyko = trumpinti_fonema (kontekstas);
	else
		pavyko = ilginti_fonema (kontekstas);
	// jei nepavyko, viskà stabdome
	if (pavyko == -1) {
		return DIDELIS_NEIGIAMAS_KLAIDOS_KODAS; 
	}

	// nustatome, iki kiek kopijuojame signalà
	int iki = 0;
	if (kontekstas->keisti_tono_auksti)
		// tono aukðèio keitimas: kopijuosime iki paskutinio piko
		iki = pikai [kontekstas->pirmojo_piko_nr + kontekstas->piku_sk -1];
	else
		// jei nekeisime tono aukðèio, kopijuosime iki fonemos pabaigos
		iki = kontekstas->fonemos_pabaiga;

	// pabaigiame nukopijuoti signalo masyvà
	// jei yra nenukopijuoto signalo
	// nukopijuojame (prisumuojame) signalo duomenis iki signalo pabaigos
	// (negalime kopijuoti su memcpy, nes prarasime jau ten esanèià informacijà).
	// Tuo paèiu ir atnaujiname einamàsias signalø masyvø reikðmes.
	pavyko = kopijuoti_signala (iki, kontekstas);
	// jei nepavyko, viskà stabdome
	if (pavyko == -1) {
		return DIDELIS_NEIGIAMAS_KLAIDOS_KODAS; 
	}

	// tono aukðèio keitimas: apdorojame pusæ paskutinio burbulo, iðlendanèià á po to einanèià fonemà
	if (kontekstas->keisti_tono_auksti)
		pavyko = kopijuoti_signala_pabaigoj (kontekstas);
	// jei nepavyko, viskà stabdome
	if (pavyko == -1)
		return DIDELIS_NEIGIAMAS_KLAIDOS_KODAS;

	// apskaièiuojame naujàjá fonemos ilgá
	int naujas_fonemos_ilgis = fonemu_ilgiai[fonemos_nr] + kontekstas->einamasis_postumis;

	// atnaujiname reikðmes
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

	// pasiruoðimas duomenø nuskaitymui

	// sudarome garsø duomenø bazës failo pavadinimà
	strcpy (signalo_failo_pavadinimas, katVardas);
	strcat (signalo_failo_pavadinimas, "db.raw");

	// sudarome fonemø failo pavadinimà
	strcpy(fonemu_failo_pavadinimas, katVardas);
	strcat(fonemu_failo_pavadinimas, "db_fon_weights.txt");

	// sudarome pikø failo pavadinimà
	strcpy(piku_failo_pavadinimas, katVardas);
	strcat(piku_failo_pavadinimas, "db_pik.txt");

	// nuskaitome duomenis is failu (uþpildome duomenø masyvus)
	int a = nuskaityti_duomenis();
	if (a < 0) return a;

	// ------------------------- Inicializuojame parametrus --------------------------------- //

	// apskaièiuojame pagalbinius masyvus darbui su pikais
	a = apskaiciuoti_pirmojo_piko_nr_fonemose ();
	if (a < 0) return a;

	// ávertiname pailginto signalo masyvo ilgá
	size_t naujo_signalo_masyvo_ilgis = rekomenduoti_naujo_signalo_masyvo_ilgi (greitis, tono_aukscio_pokytis);

	// paþymime, kad masyvà naujas_signalas galima ilginti, jei netyèia jam iðskirta per maþai atminties
	int galima_pailginti_naujas_signalas = 1;

	// inicializuojame rezultatø masyvus

	short * naujas_signalas = (short *) calloc (naujo_signalo_masyvo_ilgis, sizeof(short)); // kad uþpildytø nuliais
	if (naujas_signalas == NULL)
		return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_CHANGED_RATE_DB; 

	// susikuriame ir uþpildome pagalbiná fonemø adresø masyvà
	
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

	// ------------------------- Skaièiuojame --------------------------------- //

	unsigned int einamasis_naujo_signalo_nr = 0;

	//for (size_t fonemos_nr = 0; fonemos_nr < 10; fonemos_nr++) {
	for (unsigned int fonemos_nr = 0; fonemos_nr < fonemu_kiekis; fonemos_nr++)
	{
		// kiekvienai fonemai kvieèiame change_phoneme_rate_internal() funkcijà
		int naujas_fonemos_ilgis = change_phoneme_rate_internal (
			greitis, tono_aukscio_pokytis, fonemos_nr, 
			&naujas_signalas, &naujo_signalo_masyvo_ilgis, 
			galima_pailginti_naujas_signalas, einamasis_naujo_signalo_nr);

		// jei nepavyko, viskà stabdome
		if (naujas_fonemos_ilgis == DIDELIS_NEIGIAMAS_KLAIDOS_KODAS)
			return ERROR_RATECHANGE_MEMORY_ALLOCATION_FOR_CHANGED_RATE_DB;

		// perskaièiuojame fonemø ilgiø masyvà
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

	// turime atlaisvinti atminti ðioje funkcijoje, nes èia sukûrëme, o dinaminiu reþimu visai nereikia atlaisvinti
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
	
	// pasiruoðimas duomenø nuskaitymui

	// sudarome garsø duomenø bazës failo pavadinimà
	strcpy (signalo_failo_pavadinimas, katVardas);
	strcat (signalo_failo_pavadinimas, "db.raw");

	// sudarome fonemø failo pavadinimà
	strcpy(fonemu_failo_pavadinimas, katVardas);
	strcat(fonemu_failo_pavadinimas, "db_fon_weights.txt");

	// sudarome pikø failo pavadinimà
	strcpy(piku_failo_pavadinimas, katVardas);
	strcat(piku_failo_pavadinimas, "db_pik.txt");

	// nuskaitome duomenis is failu (uþpildome duomenø masyvus)
	int a = nuskaityti_duomenis();
	if (a < 0) return a;

	// apskaièiuojame pagalbinius masyvus darbui su pikais
	a = apskaiciuoti_pirmojo_piko_nr_fonemose ();
	if (a < 0) return a;

	// ásimename fonemø pradþiø indeksø masyvà. 
	// Já naudosime dinaminiam greièio keitimui.
	// (Gal pasidaryti kopijà?)
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
 * pakeiciam fonemos, kurios numeris yra fonemos_nr, greitá ir tono aukðtá.
 * Parametras "greitis" nurodo procentais, kiek pailginti fonemà (pavyzdþiui, 120 reiðkia pailginti 1,2 karto).
 * Parametras "tono_aukscio_pokytis" nurodo procentais, kiek paaukðtinti pagrindiná tonà 
 * (pavyzdþiui, 120 reiðkia paaukðtinti 1,2 karto: jei pagrindinis tonas buvo 100 Hz, pasidarys 120 Hz).
 * Patá naujà signalà áraðo á naujas_signalas masyvà (tiksliau, prie jo prisumuoja).
 * Laikome, kad "naujas_signalas" rodo á prieð tai buvusios (jei buvo) fonemos pabaigà+1.
 *
 * Gràþina naujo signalo ilgá, jei pavyko, ir -1, jei nepavyko (jei masyve naujas_signalas neuþteko vietos).
 ********************************************************/
int change_phoneme_rate (int greitis, int tono_aukscio_pokytis, unsigned int fonemos_nr, 
						 short * naujas_signalas, size_t naujo_signalo_masyvo_ilgis)
{
	int galima_pailginti_naujas_signalas = 0;
	unsigned int einamasis_naujo_signalo_nr = 0;
	int naujas_fonemos_ilgis = change_phoneme_rate_internal (greitis, tono_aukscio_pokytis, fonemos_nr, &naujas_signalas, &naujo_signalo_masyvo_ilgis, 
		galima_pailginti_naujas_signalas, einamasis_naujo_signalo_nr);
	// jei nepavyko, viskà stabdome
	if (naujas_fonemos_ilgis == DIDELIS_NEIGIAMAS_KLAIDOS_KODAS)
		return ERROR_RATECHANGE_SIGNAL_BUFFER_OVERFLOW;
	else
		return naujas_fonemos_ilgis;
}
