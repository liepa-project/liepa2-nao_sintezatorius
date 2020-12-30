/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.ra�tija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file RateChange.h
 * 
 * @author dr. Gintaras Skersys (gintaras.skersys@mif.vu.lt)
 * 2020 12 28
 */

#pragma once

/*********************************************************
 * Konstantos
 ********************************************************/

const double pi = 3.14159265359;

// Koks did�iausias gali b�ti atstumas tarp pik� reguliariame garse.
// Jei atstumas tarp pik� arba tarp piko ir garso ribos yra didesnis, 
// tai ka�kas negerai, tod�l negalime tam garsui trumpinti naudoti pik�
// Laikome, kad garsas negali b�ti �emesnis u� 50 Hz, 
// t.y atstumai tarp pik� negali b�ti didesni u� 1/50 s, 
// t.y. atstumai tarp pik� indeks� garso masyve negali b�ti didesni u� 22050/50=441.
#define MAX_ATSTUMAS_TARP_PIKU 441 

// koeficientas realloc funkcijai masyvo naujas_signalas pailginimui. 
// Tiek kart� bus pailgintas masyvas naujas_signalas.
// naujas_ilgis = REALLOC_COEFF * senas_ilgis, jei naujas_ilgis pakankamas
#define REALLOC_COEFF 1.2

// koeficientas, parodantis, kiek kart� daugiau reikia i�skirti atminties 
// naujo signalo masyvui, negu apytikslis �vertis (d�l visa ko)
#define NAUJO_SIGNALO_MASYVO_ILGIO_KOEF 1.1f

// nurodo signalo masyvo indeks� skai�iumi, kiek b�tina palikti leistinosios srities,  
// trumpinant dusliuosius priebalsius.
// Gali b�ti nulis. Normalu - apie 50.
// Nerekomenduojama > 200, nes tada kai kuri� fonem� visai nesutrumpins.
#define ISSAUGOTI_GALIMOS_SRITIES_ILGIO 100

// kadangi naujos fonemos ilgis gali b�ti ir neigiamas, klaidos kodui gr��inti reikia didel�s neigiamos reik�m�s
#define DIDELIS_NEIGIAMAS_KLAIDOS_KODAS -10000

/*********************************************************
Fonem� klasi� numeriai pagal fonemos pavadinimo pirm�j� raid�:
0 - turintys pik� informacij� (skardieji priebalsiai, balsiai, t.y. visi, i�skyrus x, f, p, t, k, s, S, _, r, R, z, Z, H)
1 - neturintys pik� informacijos (duslieji priebalsiai, t.y. x, f, p, t, k, s, S, _)
//2 - gali tur�ti ar netur�ti pik� informacijos, tod�l gali b�ti priskirti kuriai nors i� pirm�j� dviej� klasi� - reikia papildomo tikrinimo (z, Z, h),
3 - neai�ku, k� daryti (r, R).
*********************************************************/
#define FONEMU_KLASE_SKARDIEJI 0
#define FONEMU_KLASE_DUSLIEJI 1
//#define FONEMU_KLASE_ZZH 2
#define FONEMU_KLASE_RR 3

/*********************************************************
 * Burbulai
 ********************************************************/

struct burbulas
{
	// burbulo koordinat�s
	size_t pradzia;
	size_t vidurys;
	size_t pabaiga;

	// kiek kart� dubliuoti burbul�
	int kartai;

	// ar burbulas sudarytas pik� pagrindu, 
	// t.y. ar jo koordinat�s sutampa su pik� koordinat�mis.
	// Jei reik�m� nelygi nuliui, tai pik� pagrindu.
	//int pikai;
};

// nurodo, kiek burbul� vienai fonemai gali tekti keisti (pa�alinti ar dubliuoti)
#define MAX_KEICIAMI_BURBULAI 500

/*********************************************************
 * Kontekstas
 * 
 * Kad sintezavimas veikt� daugelio gij� re�imu 
 * (kai pakrauta viena gars� baz�, ta�iau funkcija change_phoneme_rate() i� RateChange.cpp gali b�ti vienu metu kvie�iama i� skirting� gij�), 
 * neturi b�ti globali� kintam�j�, � kuriuos ra�oma sintezavimo metu 
 * (pastaba: gali b�ti global�s kintamieji, � kuriuos ra�oma gars� baz�s u�krovimo metu). 
 * Visus tokius globalius kintamuosius, � kuriuos iki �iol buvo ra�oma sintezavimo metu, nuo �iol reikia perduoti kaip funkcij� parametrus. 
 * Kadangi j� nema�ai, juos sudedu � vien� strukt�r�, kuri� ir perduosiu.
 ********************************************************/

struct tkontekstas {
	// einamosios fonemos numeris
	unsigned int fonemos_nr;

	// einamosios fonemos prad�ia ir pabaiga
	unsigned int fonemos_pradzia;
	unsigned int fonemos_pabaiga;

	// numeris pirmojo piko, esan�io einamosios fonemos prad�ioje 
	// (tiksliau, pirmojo piko, nepriklausan�io prie� tai buvusiai fonemai. 
	// Jis gali nepriklausyti ir einamajai, o kuriai nors tolimesnei).
	unsigned int pirmojo_piko_nr;

	// kiek pik� yra tarp fonemos prad�ios ir pabaigos
	unsigned int piku_sk;
	
	// Greitinimo koeficientas (kiek kart� turi pailg�ti signalas)
	double greitinimo_koef;

	// Tarpo tarp pik� didinimo koeficientas (kiek kart� turi padid�ti tarpas tarp pik�)
	double tarpo_tarp_piku_didinimo_koef;
	
	// skirtumas tarp to, koks plotis buvo panaudotas rezultatuose, ir koks duomenyse.
	// Bus neigiamas, jei signalo rezultatas sutrump�jo, ir teigiamas, jei pailg�jo.
	// Per tiek padid�s fonem� ilgiai
	int einamasis_postumis;
	
	// einamasis garso signalo masyvo indeksas
	size_t einamasis_signalo_nr;
	
	// garso signalo masyvas, i�skiriamas dinami�kai
	short * naujas_signalas;
	
	// naujojo garso signalo masyvo ilgis (ne signalo ilgis, o kiek i�skirta vietos masyvui. 
	// Paprastai masyvo ilgis didesnis u� signalo ilg�)
	size_t naujo_signalo_masyvo_ilgis;
	
	// garso signalo ilgis
	//size_t naujo_signalo_ilgis;
	
	// einamasis naujo garso signalo masyvo indeksas
	size_t einamasis_naujo_signalo_nr;

	// nurodo, ar galima pailginti masyv� naujas_signalas, jei per trumpas (0 - negalima, !=0 - galima)
	// (negalima - jei masyvas naujas_signalas gautas i� i�or�s (jei greitinama i� funkcijos change_phoneme_rate), 
	// galima - jei atmintis jam i�skirta viduje (RateChange.dll'e) (jei greitinama i� funkcijos change_DB_rate))
	int galima_pailginti_naujas_signalas;
	
	// Euristika nustatys, kuriuos burbuliukus reikia pa�alinti ar dubliuoti. 
	// J� s�ra�� pateiks �iame kintamajame "burbulai". 
	// Kad nereik�t� pastoviai i�skirin�ti jam atminties, 
	// �ia i�skiriame vien� kart� ir pastoviai naudojame.
	// TODO: pastoviai tikrinti, ar nevir�ijo MAX_BURBULAI. Vir�ijus ka�k� daryti.
	struct burbulas burbulai[MAX_KEICIAMI_BURBULAI];

	// kei�iam� (�alinam� ar dubliuojam�) burbul� skai�ius
	int keiciamu_burbulu_sk;
	
	// fonemos klas�, nurodanti, ar ilginimui turime pik� informacij�, ar ne 
	// (FONEMU_KLASE_SKARDIEJI - turime, FONEMU_KLASE_DUSLIEJI - ne, FONEMU_KLASE_RR - nieko nedarome)
	int fonemos_klase;

	// nurodo, ar keisti pagrindinio tono auk�t� (0 - negalima, !=0 - galima)
	// Keisti, jei:
	// 1) fonema bals� arba skardusis priebalsis
	// 2) fonema turi > 1 pik� (prie�ingu atveju pagrindinis tonas n�ra patikimas, nes abiejose pus�se gali b�ti fonemos be pik� 
	// - kaip tokiu atveju rasti tarp� tarp pik�?)
	// 3) funkcija change_phoneme_rate() i�kviesta su parametru tono_aukscio_pokytis != 100
	int keisti_tono_auksti;
};

/*********************************************************
 * Greitis.cpp
 ********************************************************/

extern char signalo_failo_pavadinimas[];
extern short * signalas;
extern size_t signalo_ilgis;
extern char * naujo_signalo_failo_pavadinimas;
#ifdef MEMORY_MAPPED
extern HANDLE signalo_failo_mapping;
#endif
extern char fonemu_failo_pavadinimas[];
extern char ** fonemos;
extern int * fonemu_ilgiai;
extern size_t fonemu_kiekis;
extern char * naujo_fonemu_failo_pavadinimas;
extern int * nauji_fonemu_ilgiai;
extern char vidutiniu_ilgiu_fonemu_failo_pavadinimas[];
extern char ** skirtingos_fonemos;
extern int * vidutiniai_fonemu_ilgiai;
extern size_t skirtingu_fonemu_kiekis;
extern char piku_failo_pavadinimas[];
extern unsigned int * pikai;
extern size_t piku_kiekis;
extern const int scenarijus;
extern double scenarijaus5_koeficientas;

void klaida (char * klaidos_pranesimas);
int kopijuoti_signala_pradzioj (struct tkontekstas * kontekstas);
int kopijuoti_signala_pabaigoj (struct tkontekstas * kontekstas);
int kopijuoti_signala (size_t iki, struct tkontekstas * kontekstas);
int trumpinti_fonema (struct tkontekstas * kontekstas);
int ilginti_fonema (struct tkontekstas * kontekstas);
int vykdyti (int greitis, int tono_aukscio_pokytis, struct tkontekstas * kontekstas);

/*********************************************************
 * VeiksmaiSuFailais.cpp
 ********************************************************/

void sukurti_kataloga (char * katalogoVardas);
int failu_sarasas_is_katalogo (char * katalogoVardas, char ** failu_vardai);

int nuskaityti_anotacijas (char * fonemu_failo_pavadinimas, char *** fonemos1, int ** fonemu_ilgiai1, size_t * fonemu_kiekis1);
int nuskaityti_duomenis ();
int nuskaityti_ilginimo_koeficientus (char * failo_pavadinimas, float ** pateikti_koef1, float ** faktiniai_koef1, int * koef_skaicius);

int irasyti_anotacijas ();
int irasyti_duomenis (struct tkontekstas * kontekstas);

/*********************************************************
 * Euristika.cpp
 ********************************************************/

void euristika (struct tkontekstas * kontekstas);
bool reguliarus_pikai (struct tkontekstas * kontekstas);

/*********************************************************
 * RateChange.cpp
 ********************************************************/

int fonemosKlase (struct tkontekstas * kontekstas);

/*********************************************************
 * Klaid� kodai
 ********************************************************/
#include "LithUSS_Error.h"
