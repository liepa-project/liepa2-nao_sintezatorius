/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.ra�tija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file Euristika.cpp
 * 
 * @author dr. Gintaras Skersys (gintaras.skersys@mif.vu.lt)
 * 2020 12 28
 */

// Euristin�s funkcijos: 
// funkcijos, kurios parenka, kuriuos burbulus reik�t� pa�alinti ar dubliuoti. 
// Jos gali �gyvendinti �vairias strategijas.

#define _CRTDBG_MAP_ALLOC

// #include "StdAfx.h"
#include <stdlib.h>
//#include <crtdbg.h>
//#include "StdAfx.h"

#include <string.h>
#include <time.h>
#include <stdexcept>
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
 *********************************************************
             PAGALBIN�S FUNKCIJOS
 *********************************************************
 *********************************************************/

/*********************************************************
Nustato, ar duotoje fonemoje pikai pasiskirst� reguliariai. 
Tikrina tik tai, ar ne per retai, t.y. ar netr�ksta kokio piko.
*********************************************************/
bool reguliarus_pikai (struct tkontekstas * kontekstas)
{
	// jei i�vis n�ra pik�
	if (kontekstas->piku_sk <= 0)
		return false;
	
	// jei pernelyg didelis atstumas nuo fonemos prad�ios iki pirmojo piko
	if (pikai [kontekstas->pirmojo_piko_nr] - kontekstas->fonemos_pradzia > MAX_ATSTUMAS_TARP_PIKU)
		return false;
	
	// jei pernelyg didelis atstumas nuo paskutinio piko iki fonemos pabaigos
	if (kontekstas->fonemos_pabaiga - pikai [kontekstas->pirmojo_piko_nr + kontekstas->piku_sk - 1] > MAX_ATSTUMAS_TARP_PIKU)
		return false;
	
	// jei pernelyg dideli atstumai tarp pik�
	for (unsigned int i = 0; i < kontekstas->piku_sk - 1; i++) {
		if (pikai[kontekstas->pirmojo_piko_nr + i + 1] - pikai[kontekstas->pirmojo_piko_nr + i] > MAX_ATSTUMAS_TARP_PIKU)
			return false;
	}
	
	return true;
}

/*********************************************************
U�pildo informacij� apie nurodytus burbulus.
Burbul� numeriai turi b�ti i�reik�ti viso signalo (ne vienos fonemos) pik� numeriais
*********************************************************/
void suformuoti_nurodytus_burbulus (int pirmojo_burbulo_nr, 
								    int paskutiniojo_burbulo_nr, int pakartojimu_skaicius, struct tkontekstas * kontekstas)
{
		int keiciamu_burbulu_sk = paskutiniojo_burbulo_nr - pirmojo_burbulo_nr + 1;
		
		if (kontekstas->keiciamu_burbulu_sk + keiciamu_burbulu_sk > MAX_KEICIAMI_BURBULAI)
		{
			throw std::logic_error("per mazas MAX_KEICIAMI_BURBULAI");
		}
		
		for (int burb_nr = 0; burb_nr < keiciamu_burbulu_sk; burb_nr++) {
			// suformuojame burbul�
			int burbulo_vidurinio_piko_nr = pirmojo_burbulo_nr + burb_nr;
			kontekstas->burbulai[kontekstas->keiciamu_burbulu_sk + burb_nr].pradzia = pikai [burbulo_vidurinio_piko_nr - 1];
			kontekstas->burbulai[kontekstas->keiciamu_burbulu_sk + burb_nr].vidurys = pikai [burbulo_vidurinio_piko_nr];
			kontekstas->burbulai[kontekstas->keiciamu_burbulu_sk + burb_nr].pabaiga = pikai [burbulo_vidurinio_piko_nr + 1];
			kontekstas->burbulai[kontekstas->keiciamu_burbulu_sk + burb_nr].kartai = pakartojimu_skaicius;
		}

		kontekstas->keiciamu_burbulu_sk += keiciamu_burbulu_sk;
}

/*********************************************************
Koreguoja grei�io koeficient�, priklausomai nuo fonemos pavadinimo 
(balsiams nekei�ia, kitiems suma�ina, sprogstamiesiems dar labiau suma�ina)
*********************************************************/
double koreguoti_greitinimo_koef_scenarijus4 (struct tkontekstas * kontekstas)
{
	// parodo, kiek patrump�jimas/pailg�jimas bus ma�esnis u� standartin�
	// pvz. 0.5 rei�kia, kad pailg�s/patrump�s 50% standartinio 
	double koregavimo_koeficientas = 1;

	switch (fonemos[kontekstas->fonemos_nr][0]) {
	// balsiams nekei�iame
	case 'i':
	case 'e':
	case 'a':
	case 'o':
	case 'u':
	case 'I':
	case 'E':
	case 'A':
	case 'O':
	case 'U':
		koregavimo_koeficientas = 1;
		break;
    // 
	case 'p':
	case 't':
	case 'k':
	case 'b':
	case 'd':
	case 'g':
		koregavimo_koeficientas = 0.2;
		break;
    // 
	case 's':
	case 'S':
	case 'z':
	case 'Z':
	case 'x':
	case 'h':
	case 'f':
		koregavimo_koeficientas = 0.5;
		break;
    // 
	case 'j':
	case 'J':
	case 'v':
	case 'w':
	case 'W':
	case 'l':
	case 'L':
	case 'm':
	case 'M':
	case 'n':
	case 'N':
		koregavimo_koeficientas = 0.5;
		break;
    // 
	case '_':
		koregavimo_koeficientas = 0.5;
		break;
    // 
	default:
		// ne�inoma fonema
		koregavimo_koeficientas = 0;
		break;
	}

	return 1 + koregavimo_koeficientas * (kontekstas->greitinimo_koef - 1);

}

/*********************************************************
Koreguoja grei�io koeficient�, priklausomai nuo fonemos ilgio skirtumo nuo vidutinio ilgio 
(patempia link vidutinio ilgio priklausomai nuo scenarijaus5_koeficientas reik�m�s)
*********************************************************/
double koreguoti_greitinimo_koef_scenarijus5 (int fonemos_ilgis, struct tkontekstas * kontekstas)
{
	// nustatykime, koks yra vidutinis tokios fonemos ilgis

	// ie�kome einamosios fonemos skirting� fonem� masyve
	size_t fon_nr = 0;
	while (fon_nr < skirtingu_fonemu_kiekis && strcmp (fonemos[kontekstas->fonemos_nr], skirtingos_fonemos[fon_nr]) != 0)
		fon_nr++;

	// jei neradome, klaida
	if (fon_nr >= skirtingu_fonemu_kiekis) {
		throw std::logic_error("einamoji fonema nerasta skirtingu fonemu masyve");
	}

	// i�siai�kinkime, kiek reikia pailginti fonem�

	int siekiamas_vidutinis_fonemos_ilgis = (int) (vidutiniai_fonemu_ilgiai[fon_nr] * kontekstas->greitinimo_koef);
	double siekiamas_fonemos_ilgis = fonemos_ilgis + scenarijaus5_koeficientas * (siekiamas_vidutinis_fonemos_ilgis - fonemos_ilgis);

	return siekiamas_fonemos_ilgis/fonemos_ilgis;
}

/*********************************************************
 *********************************************************
             ILGINIMAS: SKARDIEJI
 *********************************************************
 *********************************************************/

/*********************************************************
*********************************************************/
void ilginimo_euristika_skardieji_scenarijus1 (int pirmojo_burbulo_nr, 
								   int paskutiniojo_burbulo_nr, struct tkontekstas * kontekstas)
{
		// i�renkame vidurin� burbul� dubliavimui
		
		kontekstas->keiciamu_burbulu_sk = 1;
		
		// vidurinio burbulo numeris (numeruojant nuo 0, skai�iuojant nuo pirmojo_burbulo_nr)
		int vidurinio_burbulo_nr = pirmojo_burbulo_nr 
			+ (paskutiniojo_burbulo_nr - pirmojo_burbulo_nr)/2;
		
		// suformuojame burbul�
		int burbulo_vidurinio_piko_nr = kontekstas->pirmojo_piko_nr + vidurinio_burbulo_nr;
		kontekstas->burbulai -> pradzia = pikai [burbulo_vidurinio_piko_nr - 1];
		kontekstas->burbulai -> vidurys = pikai [burbulo_vidurinio_piko_nr];
		kontekstas->burbulai -> pabaiga = pikai [burbulo_vidurinio_piko_nr + 1];
		kontekstas->burbulai -> kartai = 1;	
}

/*********************************************************
// 3. Visus trumpinti/ilginti vienodai (tiek kart�, kiek liepia greitinimo_koef)
Greitinimo koeficientas paduodamas tam, 
kad b�t� galima i�kviesti 4 scenarijuje su skirtingais greitinimo koeficientais
*********************************************************/
void ilginimo_euristika_skardieji_scenarijus3 (int pirmojo_burbulo_nr, 
								   int paskutiniojo_burbulo_nr,
								   double greitinimo_koef, struct tkontekstas * kontekstas)
{
	// reikia 5 scenarijuje
	if (greitinimo_koef <= 1)
	{
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}

	// i�siai�kinkime, kiek reikia pailginti fonem�

	int fonemos_ilgis = kontekstas->fonemos_pabaiga - kontekstas->fonemos_pradzia;
	int siekiamas_fonemos_ilgis = (int) (fonemos_ilgis * greitinimo_koef);
	int siekiamas_pailginimas = siekiamas_fonemos_ilgis - fonemos_ilgis;

	// Algoritmas: dubliuosime kiekvien� burbul� po ka�kiek kart� (nul� arba daugiau), 
	// plius vidurinius (i� eil�s einan�ius) burbulus dar vienu kartu daugiau, kad pailgintume tiek, kiek reikia.
	// 1. Nustatome, kiek pailg�t�, jei panaudotume visus burbulus. 
	// 2. Paskai�iuojame, kiek kart� reikia panaudoti kiekvien� burbul�. 
	// 3. Paskai�iuojame, kuriuos vidurinius burbulus reiks dubliuoti papildomai.
	// Kadangi gauti lygiai tokio pailginimo, kokio reikia, nepavyks, 
	// tai imame tok� pailginim�, kad paklaida b�t� ma�iausia 
	// (arba �iek tiek pritr�ksta iki reikiamo pailginimo, arba �iek tiek vir�ija).

	// 1. Nustatome, kiek pailg�t�, jei panaudotume visus burbulus. 

	// maksimalus pailginimas, jei kiekvien� burbul� kartosime ne daugiau kaip po 1 kart�
	int maksimalus_pailginimas = (pikai [kontekstas->pirmojo_piko_nr + paskutiniojo_burbulo_nr] 
		+ pikai [kontekstas->pirmojo_piko_nr + paskutiniojo_burbulo_nr + 1] 
		- pikai [kontekstas->pirmojo_piko_nr + pirmojo_burbulo_nr] 
		- pikai [kontekstas->pirmojo_piko_nr + pirmojo_burbulo_nr - 1])/2;

	// 2. Paskai�iuojame, kiek kart� reikia panaudoti kiekvien� burbul�. 

	// kiek kart� reikia kartoti visus burbulus
	int pakartojimu_skaicius = siekiamas_pailginimas / maksimalus_pailginimas;
	// kiek dar lieka pailginti, pakartojus reikiam� kart� skai�i� vis� leistin� srit�
	int trukstamas_pailginimas = siekiamas_pailginimas % maksimalus_pailginimas;

	// 3. Paskai�iuojame, kuriuos vidurinius burbulus reiks dubliuoti papildomai.

	// algoritmas: dubliavimui i�rinksime tiek vidurini� i� eil�s einan�i� 
	// (t.y. gretim�) galim� burbul�, kad b�t� gautas tr�kstamas pailginimas 
	// (gali b�ti, kad bus i�rinkti visi).

	// vidurinio burbulo numeris, i�reik�tas viso signalo (ne vienos fonemos) pik� numeriais 
	int vidurinio_burbulo_nr = kontekstas->pirmojo_piko_nr + (pirmojo_burbulo_nr + paskutiniojo_burbulo_nr)/2;

	// maksimalus burbul�, kuriuos galime dubliuoti, skai�ius
	int max_burbulu_sk = paskutiniojo_burbulo_nr - pirmojo_burbulo_nr + 1;

	// ie�komas burbul� skai�ius
	int burbulu_sk = 0;

	// ie�komos burbul� sekos pirmojo ir paskutinio burbul� numeriai
	// (burbulo numeris sutampa su jo vidurinio piko numeriu,
	// o pikai fonemoje numeruojami nuo nulio)
	// inicializuojame taip, kad burbul� seka b�t� tu��ia
	int burbulu_sekos_pradzia = vidurinio_burbulo_nr + 1;
	int burbulu_sekos_pabaiga = vidurinio_burbulo_nr;

	// kiek pailg�t�, jei dubliuotume toki� sek�
	int busimas_pailginimas = 0;
	int busimas_pailginimas_old = 0; // �simename paskutinio burbulo tikrinimui

	// i�siai�kinkime, kiek reikia dubliuoti burbul� tokiam pailginimui

	while (busimas_pailginimas < trukstamas_pailginimas && burbulu_sk < max_burbulu_sk)
	{
		burbulu_sk++;
		
		// nustatome burbul� sekos prad�i�
		burbulu_sekos_pradzia = vidurinio_burbulo_nr - (burbulu_sk - 1)/2; // teisinga tik tada, kai burbulu_sk > 0

		// nustatome burbul� sekos pabaig�
		burbulu_sekos_pabaiga = vidurinio_burbulo_nr + burbulu_sk/2;

		// kiek pailg�t�, jei dubliuotume toki� sek�
		busimas_pailginimas_old = busimas_pailginimas;
		busimas_pailginimas = (pikai [burbulu_sekos_pabaiga] + pikai [burbulu_sekos_pabaiga + 1] 
			- pikai [burbulu_sekos_pradzia] - pikai [burbulu_sekos_pradzia - 1])/2;
	}

	// i�siai�kinkime, ar paskutinis burbulas tikrai reikalingas: 
	// paskutin� burbul� imame tik tuo atveju, jei j� imant paklaida bus ma�esn�, negu neimant

	if (burbulu_sk != 0 && busimas_pailginimas - trukstamas_pailginimas > trukstamas_pailginimas - busimas_pailginimas_old) {
		// paskutinio burbulo atsisakome, perskai�iuojame reik�mes
		burbulu_sk--;
		if (burbulu_sk == 0)
			burbulu_sekos_pradzia = vidurinio_burbulo_nr + 1;
		else
			burbulu_sekos_pradzia = vidurinio_burbulo_nr - (burbulu_sk - 1)/2; // teisinga tik tada, kai burbulu_sk > 0
		burbulu_sekos_pabaiga = vidurinio_burbulo_nr + burbulu_sk/2;
		busimas_pailginimas = (pikai [burbulu_sekos_pabaiga] + pikai [burbulu_sekos_pabaiga + 1] 
			- pikai [burbulu_sekos_pradzia] - pikai [burbulu_sekos_pradzia - 1])/2;
	}

	// suformuojame nurodytus burbulus
	if (pakartojimu_skaicius == 0)
		// dubliuoti tik vidurinius burbulus po vien� kart�
		suformuoti_nurodytus_burbulus (burbulu_sekos_pradzia, burbulu_sekos_pabaiga, 1, kontekstas);
	else {
		// dubliuoti visus burbulus po pakartojimu_skaicius kart�
		suformuoti_nurodytus_burbulus (kontekstas->pirmojo_piko_nr + pirmojo_burbulo_nr, kontekstas->pirmojo_piko_nr + paskutiniojo_burbulo_nr, pakartojimu_skaicius, kontekstas);

		// vidurinius burbulus dubliuoti vienu kartu daugiau
		for (int burb_nr = 0; burb_nr < burbulu_sekos_pabaiga - burbulu_sekos_pradzia + 1; burb_nr++) {
			(kontekstas->burbulai[burbulu_sekos_pradzia - kontekstas->pirmojo_piko_nr - pirmojo_burbulo_nr + burb_nr].kartai)++;
		}
	}
}

/*********************************************************
Skard�i�j� gars� ilginimas - i�kvie�iamas nurodytas scenarijus
*********************************************************/
void ilginimo_euristika_skardieji (struct tkontekstas * kontekstas)
{
	// jei tik 0, 1 ar 2 pikai fonemoje, n�ra burbulo dubliavimui
	if (kontekstas->piku_sk < 3) {
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}
	
	// nusistatome ribas - nuo kurio iki kurio burbulo galime dubliuoti
	int pirmojo_burbulo_nr = 1;
	int paskutiniojo_burbulo_nr = kontekstas->piku_sk - 2;
	
	// jei n�ra burbulo dubliavimui
	if (paskutiniojo_burbulo_nr < pirmojo_burbulo_nr) {
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}
	
	switch (scenarijus) {
		
	case 1:
	case 2:
		// i�renkame vidurin� burbul� dubliavimui
		
		ilginimo_euristika_skardieji_scenarijus1 (pirmojo_burbulo_nr, paskutiniojo_burbulo_nr, kontekstas);
		
		break;
		
	case 3:
		// 3. Visus trumpinti/ilginti vienodai (tiek kart�, kiek liepia greitinimo_koef)
		ilginimo_euristika_skardieji_scenarijus3 (pirmojo_burbulo_nr, paskutiniojo_burbulo_nr, kontekstas->greitinimo_koef, kontekstas);
		break;
		
	case 4:
		// 4. Visus trumpinti/ilginti proporcingai, priklausomai nuo fonemos pavadinimo 
		// (balsius labiausiai, kitus ma�iau, sprogstamuosius dar ma�iau, r nekeisti)
		ilginimo_euristika_skardieji_scenarijus3 (pirmojo_burbulo_nr, paskutiniojo_burbulo_nr,
			koreguoti_greitinimo_koef_scenarijus4 (kontekstas), kontekstas);
		break;
		
	case 5:
		// 5. Stengtis pritempti iki tos fonemos ilgio vidurkio, priklausomai nuo scenarijaus5_koeficientas
		
		ilginimo_euristika_skardieji_scenarijus3 (pirmojo_burbulo_nr, paskutiniojo_burbulo_nr,
			koreguoti_greitinimo_koef_scenarijus5 (kontekstas->fonemos_pabaiga - kontekstas->fonemos_pradzia, kontekstas), kontekstas);
		break;
		
	default:
		kontekstas->keiciamu_burbulu_sk = 0;
		break;
		
	}
}

/*********************************************************
 *********************************************************
             ILGINIMAS: DUSLIEJI
 *********************************************************
 *********************************************************/

/*********************************************************
Leistinos srities rib� dusli�j� gars� ilginimui nustatymas
*********************************************************/
void leistinos_srities_ribos_dusliuju_ilginimui (size_t fonemos_nr, double * galimos_srities_pradzia, double * galimos_srities_pabaiga)
{
	switch (fonemos[fonemos_nr][0]) {
		
	case 's':
	case 'S':
		*galimos_srities_pradzia = 0.25;
		*galimos_srities_pabaiga = 0.65;
		break;
	case 'f':
		*galimos_srities_pradzia = 0.20;// 0.25
		*galimos_srities_pabaiga = 0.70;//0.40;// 0.65
		break;
	case 'x':
		*galimos_srities_pradzia = 0.40;
		*galimos_srities_pabaiga = 0.70;
		break;
	case 'p':
		*galimos_srities_pradzia = 0.30;
		*galimos_srities_pabaiga = 0.55;// 0.65
		break;
	case 't':
		if (fonemos[fonemos_nr][1] == 's' || fonemos[fonemos_nr][1] == 'S') {// ts arba tS
			*galimos_srities_pradzia = 0.40;
			*galimos_srities_pabaiga = 0.70;
		} else {// t
			*galimos_srities_pradzia = 0.20;// 0.30
			*galimos_srities_pabaiga = 0.40;// 0.60
		}
		break;
	case 'k':
		*galimos_srities_pradzia = 0.20;
		*galimos_srities_pabaiga = 0.45;
		break;
	case '_':
		*galimos_srities_pradzia = 0.10;
		*galimos_srities_pabaiga = 0.90;
		break;
	case 'z':
	case 'Z':
	case 'h':
		// TODO: tai Z duomenys. Kol kas z ir h nepasitaik� visai. 
		// Kai pasitaikys, atitinkamai pakeisti galim� interval�
		*galimos_srities_pradzia = 0.45;
		*galimos_srities_pabaiga = 0.70;
		break;
	default: // nekeiskim visai; ne�inoma fonema
		*galimos_srities_pradzia = 0.50;
		*galimos_srities_pabaiga = 0.50;
		break;
	}
	
}

/*********************************************************
// 3. Visus trumpinti/ilginti vienodai (tiek kart�, kiek liepia greitinimo_koef)
galimos_srities_pradzia - galimos srities prad�ios indeksas signalo masyve.
galimos_srities_pabaiga - galimos srities pabaigos indeksas signalo masyve.
Greitinimo koeficientas paduodamas tam, 
kad b�t� galima i�kviesti 4 scenarijuje su skirtingais greitinimo koeficientais
*********************************************************/
void ilginimo_euristika_duslieji_scenarijus3 (size_t galimos_srities_pradzia, 
											  size_t galimos_srities_pabaiga,
								              double greitinimo_koef, struct tkontekstas * kontekstas)
{
	// reikia 5 scenarijuje
	if (greitinimo_koef <= 1)
	{
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}

	// i�siai�kinkime, kiek reikia pailginti fonem�

	size_t fonemos_ilgis = kontekstas->fonemos_pabaiga - kontekstas->fonemos_pradzia;
	size_t siekiamas_fonemos_ilgis = (size_t) (fonemos_ilgis * greitinimo_koef);
	int siekiamas_pailginimas = siekiamas_fonemos_ilgis - fonemos_ilgis;
	
	int burbulu_sk = 0;

	// ptk ilginame, imdami burbulus i� pirmos gars� baz�s fonemos (pauz�s), 
	// tod�l parametrus galimos_srities_pradzia ir galimos_srities_pabaiga
	// nurodome ne einamosios fonemos, o pauz�s,
	// bei kad nukopijuot� sen� signal� iki fonemos leistinos srities vidurio, 
	// suformuojame fiktyv� nulinio plo�io burbul� toje vietoje
	
	if (fonemos[kontekstas->fonemos_nr][0] == 'p' || fonemos[kontekstas->fonemos_nr][0] == 'k' || 
		(fonemos[kontekstas->fonemos_nr][0] == 't' && fonemos[kontekstas->fonemos_nr][1] != 's' && fonemos[kontekstas->fonemos_nr][1] != 'S')) {
		
		// kad nukopijuot� sen� signal� iki fonemos leistinos srities vidurio, 
		// suformuojame fiktyv� nulinio plo�io burbul� toje vietoje

		// suformuojame fiktyv� burbul�
		kontekstas->burbulai[burbulu_sk].pradzia = (galimos_srities_pradzia + galimos_srities_pabaiga)/2;
		kontekstas->burbulai[burbulu_sk].pabaiga = kontekstas->burbulai[burbulu_sk].pradzia;
		kontekstas->burbulai[burbulu_sk].vidurys = kontekstas->burbulai[burbulu_sk].pradzia;
		kontekstas->burbulai[burbulu_sk].kartai = 0;

		// padidiname burbul� skaitliuk�
		burbulu_sk++;

		// su�inome pauz�s galimos srities ribas
		double pauzes_galimos_srities_pradzia = 0;
		double pauzes_galimos_srities_pabaiga = 1;
		leistinos_srities_ribos_dusliuju_ilginimui (0, &pauzes_galimos_srities_pradzia, &pauzes_galimos_srities_pabaiga);
		
		// perskai�iuojame parametrus galimos_srities_pradzia ir galimos_srities_pabaiga, 
		// kad atitikt� pirm� gars� baz�s fonem� (pauz�)
		galimos_srities_pradzia = (size_t) (pauzes_galimos_srities_pradzia * fonemu_ilgiai[0]);
		galimos_srities_pabaiga = (size_t) (pauzes_galimos_srities_pabaiga * fonemu_ilgiai[0]); 
	}
		
	// Algoritmas: burbulus parenkame atsitiktinai, kol pasiekiame norim� pailg�jim�. 
	// Paskutin� burbul�, jei per didelis, suma�iname iki reikiamo ilgio.

	int galimos_srities_ilgis = galimos_srities_pabaiga - galimos_srities_pradzia;
	// kiek dar liko pailginti
	int trukstamas_pailginimas = siekiamas_pailginimas;

	srand((unsigned int)time(NULL));

	while (trukstamas_pailginimas > 0) {
		// atsitiktinai parenkame burbulo ilg�
		size_t burbulo_plotis = (size_t) ((((double)rand())/RAND_MAX)*galimos_srities_ilgis);

		// jei per ilgas burbulas, suma�iname iki reikiamo ilgio
		if (burbulo_plotis/2 > (size_t)trukstamas_pailginimas)
			burbulo_plotis = trukstamas_pailginimas*2;

		// atsitiktinai parenkame burbulo prad�ios pozicij� leistinoje srityje
		size_t burbulo_pradzia = (size_t) ((((double)rand())/RAND_MAX)*(galimos_srities_ilgis-burbulo_plotis));

		// suformuojame burbul�
		kontekstas->burbulai[burbulu_sk].pradzia = galimos_srities_pradzia + burbulo_pradzia;
		kontekstas->burbulai[burbulu_sk].pabaiga = kontekstas->burbulai[burbulu_sk].pradzia + burbulo_plotis;
		kontekstas->burbulai[burbulu_sk].vidurys 
			= (size_t) (kontekstas->burbulai[burbulu_sk].pradzia + kontekstas->burbulai[burbulu_sk].pabaiga)/2;
		kontekstas->burbulai[burbulu_sk].kartai = 1;

		// padidiname burbul� skaitliuk�
		burbulu_sk++;

		// perskai�iuojame tr�kstam� pailginim�
		trukstamas_pailginimas -= burbulo_plotis/2;
	}
	
		// �simename sudaryt� burbul� skai�i�
		kontekstas->keiciamu_burbulu_sk = burbulu_sk;
}

/*********************************************************
Dusli�j� gars� ilginimas - i�kvie�iamas nurodytas scenarijus
*********************************************************/
void ilginimo_euristika_duslieji (struct tkontekstas * kontekstas)
{
	// Duslieji garsai, kuri� trumpinimui nenaudojame pik� informacijos.
	// Sudarome 1 burbul�, kur� reik�s i�kirpti.
	// TODO: Burbulo dydis ir pad�tis turi priklausyti nuo:
	// 1) trumpinimo koeficiento,
	// 2) (galb�t?) fonemos pavadinimo,
	// 3) fonemos rib� (pvz, burbulas negali prasid�ti ar�iau fonemos prad�ios kaip 20% jos ilgio 
	//		ir negali pasibaigti ar�iau fonemos pabaigos kaip 70% jos ilgio. 
	//		Ribos gali priklausyti nuo fonemos pavadinimo).
	
	// nusistatome ribas - nuo kur iki kur galime dubliuoti
	double galimos_srities_pradzia = 0;
	double galimos_srities_pabaiga = 1;
	
	leistinos_srities_ribos_dusliuju_ilginimui (kontekstas->fonemos_nr, &galimos_srities_pradzia, &galimos_srities_pabaiga);

	switch (scenarijus) {
		
	case 1:
	case 2:
		kontekstas->keiciamu_burbulu_sk = 1;
		
		// suformuojame burbul�
		kontekstas->burbulai[0].pradzia = kontekstas->fonemos_pradzia + (size_t) (0.30 * fonemu_ilgiai[kontekstas->fonemos_nr]);
		kontekstas->burbulai[0].vidurys = kontekstas->fonemos_pradzia + (size_t) (0.40 * fonemu_ilgiai[kontekstas->fonemos_nr]);
		kontekstas->burbulai[0].pabaiga = kontekstas->fonemos_pradzia + (size_t) (0.50 * fonemu_ilgiai[kontekstas->fonemos_nr]);
		kontekstas->burbulai[0].kartai = 1;
		//kontekstas->burbulai[0].pikai = 0;
		
		break;
		
	case 3:
		// 3. Visus trumpinti/ilginti vienodai (tiek kart�, kiek liepia greitinimo_koef)
		ilginimo_euristika_duslieji_scenarijus3 (
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pradzia * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pabaiga * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			kontekstas->greitinimo_koef, kontekstas);
		break;
		
	case 4:
		// 4. Visus trumpinti/ilginti proporcingai, priklausomai nuo fonemos pavadinimo 
		// (balsius labiausiai, kitus ma�iau, sprogstamuosius dar ma�iau, r nekeisti)
		ilginimo_euristika_duslieji_scenarijus3 (
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pradzia * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pabaiga * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			koreguoti_greitinimo_koef_scenarijus4 (kontekstas), kontekstas);
		break;
		
	case 5:
		// 5. Stengtis pritempti iki tos fonemos ilgio vidurkio, priklausomai nuo scenarijaus5_koeficientas
		
		ilginimo_euristika_duslieji_scenarijus3 (
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pradzia * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pabaiga * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			koreguoti_greitinimo_koef_scenarijus5 (kontekstas->fonemos_pabaiga - kontekstas->fonemos_pradzia, kontekstas), kontekstas);
		break;
		
	default:
		kontekstas->keiciamu_burbulu_sk = 0;
		break;
	}
}

/*********************************************************
 *********************************************************
             TRUMPINIMAS: DUSLIEJI
 *********************************************************
 *********************************************************/

/*********************************************************
// 3. Visus trumpinti/ilginti vienodai (tiek kart�, kiek liepia greitinimo_koef).
galimos_srities_pradzia - galimos srities prad�ios indeksas signalo masyve.
galimos_srities_pabaiga - galimos srities pabaigos indeksas signalo masyve.
Greitinimo koeficientas paduodamas tam, 
kad b�t� galima i�kviesti 4 scenarijuje su skirtingais greitinimo koeficientais
*********************************************************/
void trumpinimo_euristika_duslieji_scenarijus3 (size_t galimos_srities_pradzia, 
											  size_t galimos_srities_pabaiga,
											  double greitinimo_koef, struct tkontekstas * kontekstas)
{
	// reikia 5 scenarijuje
	if (greitinimo_koef >= 1)
	{
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}

	// i�siai�kinkime, kiek reikia patrumpinti fonem�

	size_t fonemos_ilgis = kontekstas->fonemos_pabaiga - kontekstas->fonemos_pradzia;
	size_t siekiamas_fonemos_ilgis = (size_t) (fonemos_ilgis * greitinimo_koef);
	size_t siekiamas_patrumpinimas = fonemos_ilgis - siekiamas_fonemos_ilgis;

	// randame leistinosios srities ilg�

	size_t galimos_srities_centras = (galimos_srities_pradzia + galimos_srities_pabaiga)/2;
	size_t galimos_srities_ilgis = galimos_srities_pabaiga - galimos_srities_pradzia;

	// jei i�saugoti reikia vis� leistin�j� srit�, nieko nedarome
	if (galimos_srities_ilgis <= ISSAUGOTI_GALIMOS_SRITIES_ILGIO) {
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}

	// Paskai�iuokime, kiek galime patrumpinti fonem�.

	size_t galimas_patrumpinimas = galimos_srities_ilgis - ISSAUGOTI_GALIMOS_SRITIES_ILGIO;

	size_t busimas_patrumpinimas = 0;
	if (siekiamas_patrumpinimas > galimas_patrumpinimas)
		busimas_patrumpinimas = galimas_patrumpinimas;
	else
		busimas_patrumpinimas = siekiamas_patrumpinimas;
	
	// Jei trumpinti nedaug, sudarysime vien� burbul�.
	// Jei daugiau, sudarysime grandin� i� 2 burbul�.

	if (busimas_patrumpinimas <= ISSAUGOTI_GALIMOS_SRITIES_ILGIO) {
		
		kontekstas->keiciamu_burbulu_sk = 1;
		
		// suformuojame burbul�
		kontekstas->burbulai[0].pradzia = galimos_srities_centras - busimas_patrumpinimas;
		kontekstas->burbulai[0].vidurys = galimos_srities_centras;
		kontekstas->burbulai[0].pabaiga = galimos_srities_centras + busimas_patrumpinimas;
		kontekstas->burbulai[0].kartai = 1;
		
	} else {
		
		kontekstas->keiciamu_burbulu_sk = 2;

		size_t burbulu_srities_ilgis = busimas_patrumpinimas + ISSAUGOTI_GALIMOS_SRITIES_ILGIO;
		size_t burbulu_srities_pradzia = galimos_srities_centras - burbulu_srities_ilgis/2;
		
		// suformuojame burbulus

		// pirmojo burbulo kair�s plotis = ISSAUGOTI_GALIMOS_SRITIES_ILGIO
		kontekstas->burbulai[0].pradzia = burbulu_srities_pradzia;
		kontekstas->burbulai[0].vidurys = burbulu_srities_pradzia + ISSAUGOTI_GALIMOS_SRITIES_ILGIO;
		kontekstas->burbulai[0].pabaiga = burbulu_srities_pradzia + busimas_patrumpinimas;
		kontekstas->burbulai[0].kartai = 1;
		
		// antrojo burbulo de�in�s plotis = ISSAUGOTI_GALIMOS_SRITIES_ILGIO
		kontekstas->burbulai[1].pradzia = kontekstas->burbulai[0].vidurys;
		kontekstas->burbulai[1].vidurys = kontekstas->burbulai[0].pabaiga;
		kontekstas->burbulai[1].pabaiga = burbulu_srities_pradzia + burbulu_srities_ilgis;
		kontekstas->burbulai[1].kartai = 1;
		
	}
}

/*********************************************************
Leistinos srities rib� dusli�j� gars� trumpinimui nustatymas
*********************************************************/
void leistinos_srities_ribos_dusliuju_trumpinimui (size_t fonemos_nr, double * galimos_srities_pradzia, double * galimos_srities_pabaiga)
{
	// kol kas nesiskiria nuo leistinosios srities rib� dusli�j� gars� ilginimui,
	// ta�iau jei reik�s, kad skirt�si, galima bus pakeisti
	leistinos_srities_ribos_dusliuju_ilginimui (fonemos_nr, galimos_srities_pradzia, galimos_srities_pabaiga);
}

/*********************************************************
Dusli�j� gars� trumpinimas - i�kvie�iamas nurodytas scenarijus
*********************************************************/
void trumpinimo_euristika_duslieji (struct tkontekstas * kontekstas)
{
	// Duslieji garsai, kuri� trumpinimui nenaudojame pik� informacijos.
	// Sudarome 1 burbul�, kur� reik�s i�kirpti.
	// TODO: Burbulo dydis ir pad�tis turi priklausyti nuo:
	// 1) trumpinimo koeficiento,
	// 2) (galb�t?) fonemos pavadinimo,
	// 3) fonemos rib� (pvz, burbulas negali prasid�ti ar�iau fonemos prad�ios kaip 20% jos ilgio 
	//		ir negali pasibaigti ar�iau fonemos pabaigos kaip 70% jos ilgio. 
	//		Ribos gali priklausyti nuo fonemos pavadinimo).
	
	// nusistatome ribas - nuo kur iki kur galime "�alinti"
	double galimos_srities_pradzia = 0;
	double galimos_srities_pabaiga = 1;
	
	leistinos_srities_ribos_dusliuju_trumpinimui (kontekstas->fonemos_nr, &galimos_srities_pradzia, &galimos_srities_pabaiga);

	switch (scenarijus) {
		
	case 1:
	case 2:
		kontekstas->keiciamu_burbulu_sk = 1;
		
		// suformuojame burbul�
		kontekstas->burbulai[0].pradzia = kontekstas->fonemos_pradzia + (size_t) (0.30 * fonemu_ilgiai[kontekstas->fonemos_nr]);
		kontekstas->burbulai[0].vidurys = kontekstas->fonemos_pradzia + (size_t) (0.40 * fonemu_ilgiai[kontekstas->fonemos_nr]);
		kontekstas->burbulai[0].pabaiga = kontekstas->fonemos_pradzia + (size_t) (0.50 * fonemu_ilgiai[kontekstas->fonemos_nr]);
		kontekstas->burbulai[0].kartai = 1;
		//kontekstas->burbulai[0].pikai = 0;
		
		break;

	case 3:
		// 3. Visus trumpinti/ilginti vienodai (tiek kart�, kiek liepia greitinimo_koef)
		trumpinimo_euristika_duslieji_scenarijus3 (
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pradzia * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pabaiga * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			kontekstas->greitinimo_koef, kontekstas);
		break;
		
	case 4:
		// 4. Visus trumpinti/ilginti proporcingai, priklausomai nuo fonemos pavadinimo 
		// (balsius labiausiai, kitus ma�iau, sprogstamuosius dar ma�iau, r nekeisti)
		trumpinimo_euristika_duslieji_scenarijus3 (
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pradzia * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pabaiga * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			koreguoti_greitinimo_koef_scenarijus4 (kontekstas), kontekstas);
		break;
		
	case 5:
		// 5. Stengtis pritempti iki tos fonemos ilgio vidurkio, priklausomai nuo scenarijaus5_koeficientas
		
		trumpinimo_euristika_duslieji_scenarijus3 (
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pradzia * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			kontekstas->fonemos_pradzia + (size_t) (galimos_srities_pabaiga * fonemu_ilgiai[kontekstas->fonemos_nr]), 
			koreguoti_greitinimo_koef_scenarijus5 (kontekstas->fonemos_pabaiga - kontekstas->fonemos_pradzia, kontekstas), kontekstas);
		break;
		
	default:
		kontekstas->keiciamu_burbulu_sk = 0;
		break;
		
	}
}

/*********************************************************
 *********************************************************
             TRUMPINIMAS: SKARDIEJI
 *********************************************************
 *********************************************************/

/*********************************************************
// 3. Visus trumpinti/ilginti vienodai (tiek kart�, kiek liepia greitinimo_koef)
Greitinimo koeficientas paduodamas tam, 
kad b�t� galima i�kviesti 4 scenarijuje su skirtingais greitinimo koeficientais
*********************************************************/
void trumpinimo_euristika_skardieji_scenarijus3 (int pirmojo_burbulo_nr, 
								   int paskutiniojo_burbulo_nr,
								   double greitinimo_koef, struct tkontekstas * kontekstas)
{
	// reikia 5 scenarijuje
	if (greitinimo_koef >= 1)
	{
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}

	// i�siai�kinkime, kiek reikia patrumpinti fonem�

	int fonemos_ilgis = kontekstas->fonemos_pabaiga - kontekstas->fonemos_pradzia;
	int siekiamas_fonemos_ilgis = (int) (fonemos_ilgis * greitinimo_koef);
	int siekiamas_patrumpinimas = fonemos_ilgis - siekiamas_fonemos_ilgis;

	// Algoritmo tikslas: i�mesti burbulus taip, kad net ir i�metus nema�ai burbul�, vis tiek likt� burbul� i� fonemos vidurio, ne tik i� kra�t�.
	//
	// Algoritmo id�ja: pradedant nuo vidurini� burbul�, i�metin�ti kas antr� burbul�. Likusius i�metin�ti, pradedant nuo kra�tini�.
	//
	// Detalesn� algoritmo id�ja: 
	// pirmame etape i�metin�sime nelyginius burbulus nuo vidurini� link kra�tini�, 
	// antrame - lyginius nuo kra�tini� link vidurini�.
	// (laikome, kad pirmas burbulas yra pirmojo_burbulo_nr)
	//
	// Algoritmo realizacija:
	//
	// I etapas.
	//
	// Nelygini� skai�i� sekos 1, 3, 5, ..., 2k-1 i�rikiavim� nuo vidurini� nari� suvedame 
	// � skai�i� sekos 1, 2, 3, ..., k i�rikiavim� nuo vidurini� nari�.
	// Pa�ym�kime v:=[(k+1)/2] - vidurinis (arba vienas i� vidurini�) sekos 1, 2, 3, ..., k narys. 
	// Tada vienas i� galim� i�rikiavim� nuo vidurini� nari� b�d� b�t� toks:
	// v, v+1, v-1, v+2, v-2, ... (kol panaudosime visus k nari�).
	// Tokiai sekai konstruoti sugalvojau tok� rekursyv� b�d�:
	// a_0 = v, a_i = a_{i-1} + b_i * i, kur
	// b_0 = -1; b_i = (-1) * b_{i-1}.
	//
	// II etapas.
	// 
	// Lygini� skai�i� sekos 2, 4, 6, ..., 2k i�rikiavim� nuo kra�tini� nari� suvedame 
	// � skai�i� sekos 1, 2, 3, ..., k i�rikiavim� nuo kra�tini� nari�.
	// Tada vienas i� galim� i�rikiavim� nuo kra�tini� nari� b�d� b�t� toks:
	// 1, k, 2, k-1, ... (kol panaudosime visus k nari�).
	// Tokiai sekai konstruoti sugalvojau tok� rekursyv� b�d�:
	// a_0 = 1, a_i = a_{i-1} + b_i * (k-i), kur
	// b_0 = -1; b_i = (-1) * b_{i-1}.
	// 
	// Bet mums neu�tenka sudaryti burbul� sek�, reikia dar, kad burbulai masyve b�t� i�rikiuoti nuo pirmiausio iki paskutinio.
	// Tai realizuosime taip: pirma �ym�sim�s �alinam� burbul� numerius specialiame pagalbiniame masyve
	// (prad�ioje visi masyvo elementai = 0. Jei �alinsime i-t�j� burbul�, i-1-�j� masyvo element� kei�iame � 1), 
	// kol pasieksime reikiam� patrump�jim�. 
	// Tik tada suformuosime tuos burbulus burbul� masyve, kuri� reik�m�s specialiame masyve lygios 1.


	// maksimalus burbul�, kuriuos galime "i�mesti", skai�ius
	int max_burbulu_sk = paskutiniojo_burbulo_nr - pirmojo_burbulo_nr + 1;

	// �alinam� burbul� numeri� masyvas
	// (prad�ioje visi masyvo elementai = 0. Jei �alinsime i-t�j� burbul�, i-1-�j� masyvo element� kei�iame � 1)
	int * salinami_burbulai = (int *) calloc (max_burbulu_sk, sizeof(int));

	// kiek patrump�t�, jei "i�mestume" tokius burbulus
	int busimas_patrumpinimas = 0;
	int busimas_patrumpinimas_old = 0; // �simename paskutinio burbulo tikrinimui

	// �simename paskutin� burbul�, kad gal�tume prireikus jo atsisakyti (numeruojame nuo 1 iki max_burbulu_sk)
	int burb = 0;
	int pik = 0;

	// I etapas

	// Nelyginio numerio burbul� skai�ius (jei numeruojame juos nuo 1 iki max_burbulu_sk)
	int k = (max_burbulu_sk + 1)/2;

	// Pa�ym�kime v:=[(k+1)/2] - vidurinis (arba vienas i� vidurini�) sekos 1, 2, 3, ..., k narys. 
	int v = (k+1)/2;

	// i�metin�jame nelyginius burbulus nuo vidurinio, kol pasieksime siekiam� patrumpinim�, arba kol panaudosime visus sekos narius

	int i = 0;
	int a = v;
	int b = -1;

	while (busimas_patrumpinimas < siekiamas_patrumpinimas && i < k)
	{
		// a_0 = v, a_i = a_{i-1} + b_i * i, (a_{-1}=v)
		a = a + b * i;
		// b_0 = -1; b_i = (-1) * b_{i-1}
		b = -b;

		// �alinsime burbul� 2a-1
		//burb_old = burb;
		burb = 2*a - 1;
		salinami_burbulai [burb - 1] = 1;
		// burbulo numeris, i�reik�tas viso signalo (ne vienos fonemos) pik� numeriais 
		pik = kontekstas->pirmojo_piko_nr + pirmojo_burbulo_nr - 1 + burb;

		// kiek patrump�t�, jei "i�mestume" tok� burbul�
		busimas_patrumpinimas_old = busimas_patrumpinimas;
		busimas_patrumpinimas += pikai [pik+1] - pikai [pik-1] - (pikai [pik+1] - pikai [pik-1])/2;

		i++;
	}

	// II etapas

	// Lyginio numerio burbul� skai�ius (jei numeruojame juos nuo 1 iki max_burbulu_sk)
	k = max_burbulu_sk/2;

	i = 0;
	a = k+1;
	b = -1;

	while (busimas_patrumpinimas < siekiamas_patrumpinimas && i < k)
	{
		// a_0 = 1, a_i = a_{i-1} + b_i * (k-i), (a_{-1}=k+1)
		a = a + b * (k-i);
		// b_0 = -1; b_i = (-1) * b_{i-1}
		b = -b;

		// �alinsime burbul� 2a
		//burb_old = burb;
		burb = 2*a;
		salinami_burbulai [burb - 1] = 1;
		// burbulo numeris, i�reik�tas viso signalo (ne vienos fonemos) pik� numeriais 
		pik = kontekstas->pirmojo_piko_nr + pirmojo_burbulo_nr - 1 + burb;

		// kiek patrump�t�, jei "i�mestume" tok� burbul�
		busimas_patrumpinimas_old = busimas_patrumpinimas;
		busimas_patrumpinimas += pikai [pik+1] - pikai [pik-1] - (pikai [pik+1] - pikai [pik-1])/2; // tai da�niausiai tikslus patrumpinimas, bet kartais gali skirtis per 1 ar -1, �r. 2015-03-27 u�ra�us 

		i++;
	}

	// i�siai�kinkime, ar paskutinis burbulas tikrai reikalingas: 
	// paskutin� burbul� imame tik tuo atveju, jei j� imant paklaida bus ma�esn�, negu neimant

	if (burb != 0 && busimas_patrumpinimas - siekiamas_patrumpinimas > siekiamas_patrumpinimas - busimas_patrumpinimas_old) {
		// paskutinio burbulo atsisakome, perskai�iuojame reik�mes
		salinami_burbulai [burb - 1] = 0;
		busimas_patrumpinimas -= pikai [pik+1] - pikai [pik-1] - (pikai [pik+1] - pikai [pik-1])/2;
	}

	// suformuojame nurodytus burbulus
	for (i=0; i<max_burbulu_sk; i++)
		if (salinami_burbulai [i] == 1) {
			pik = kontekstas->pirmojo_piko_nr + pirmojo_burbulo_nr + i;
			suformuoti_nurodytus_burbulus (pik, pik, 1, kontekstas);
		}

	free (salinami_burbulai);
}

/*********************************************************
Skard�i�j� gars� trumpinimas - i�kvie�iamas nurodytas scenarijus
*********************************************************/
void trumpinimo_euristika_skardieji (struct tkontekstas * kontekstas)
{
	// Skardieji garsai, kuri� trumpinimui naudojame pik� informacij�.
	// Kol kas parinksime vidurin� burbul�, kur� reik�s i�kirpti.
	// TODO: padaryti priklausomyb� nuo trumpinimo koeficiento.
	
	// jei tik 0, 1 ar 2 pikai fonemoje, n�ra burbulo �alinimui
	if (kontekstas->piku_sk < 3) {
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}
	
	// randame pirmojo burbulo, kur� galima pa�alinti, numer�
	
	// numeris pirmojo burbulo fonemoje, kur� galima pa�alinti. Numeruojama nuo nulio.
	// Burbulo numeris sutampa su burbulo vidurinio piko numeriu,
	// t.y. burbulas i yra tas, 
	// kurio vidurinis pikas yra pirmojo_piko_nr + i.
	// Nulinio burbulo vidurinis pikas yra pirmojo_piko_nr, 
	// tod�l nulinis burbulas visada kirs fonemos rib� 
	// ir jo d�l to niekada negal�sime �alinti.
	int pirmojo_burbulo_nr = 0;
	
	// jei nuo pirmojo piko iki fonemos prad�ios lieka ma�iau (arba lygu) 
	// kaip pus� pirmojo periodo ilgio (atstumas tarp pirm�j� pik�), 
	// tai pirmojo burbulo negalime �alinti, nes jo prad�ia labai arti fonemos prad�ios
	// (galime �alinti nuo antrojo),
	// prie�ingu atveju galime �alinti jau ir pirm�j�.
	if (2 * (pikai [kontekstas->pirmojo_piko_nr] - kontekstas->fonemos_pradzia) <= 
		pikai [kontekstas->pirmojo_piko_nr+1] - pikai [kontekstas->pirmojo_piko_nr])
		pirmojo_burbulo_nr = 2;
	else
		pirmojo_burbulo_nr = 1;
	
	// analogi�kai randame paskutiniojo burbulo, kur� galima pa�alinti, numer�
	
	int paskutiniojo_burbulo_nr = 0;
	int paskutiniojo_piko_nr = kontekstas->pirmojo_piko_nr+kontekstas->piku_sk-1;
	if (2 * (kontekstas->fonemos_pabaiga - pikai [paskutiniojo_piko_nr]) <= 
		pikai [paskutiniojo_piko_nr] - pikai [paskutiniojo_piko_nr-1])
		paskutiniojo_burbulo_nr = kontekstas->piku_sk - 3;
	else
		paskutiniojo_burbulo_nr = kontekstas->piku_sk - 2;
	
	// jei n�ra burbulo �alinimui
	if (paskutiniojo_burbulo_nr < pirmojo_burbulo_nr) {
		kontekstas->keiciamu_burbulu_sk = 0;
		return;
	}
	
	int vidurinio_burbulo_nr = 0;

	switch (scenarijus) {
		
	case 1:
		// i�renkame vidurin� burbul� �alinimui
		
		// vidurinio burbulo numeris (numeruojant nuo 0, skai�iuojant nuo pirmojo_burbulo_nr)
		vidurinio_burbulo_nr = kontekstas->pirmojo_piko_nr 
			+ (pirmojo_burbulo_nr + paskutiniojo_burbulo_nr)/2;
		
		// suformuojame nurodytus burbulus
		suformuoti_nurodytus_burbulus (
			vidurinio_burbulo_nr, vidurinio_burbulo_nr, 1, kontekstas);

		break;
	case 2:
		// �alinimui paskiriame visus galimus burbulus
		
		// suformuojame nurodytus burbulus
		suformuoti_nurodytus_burbulus (kontekstas->pirmojo_piko_nr + pirmojo_burbulo_nr, 
								   kontekstas->pirmojo_piko_nr + paskutiniojo_burbulo_nr, 1, kontekstas);

		break;
		
	case 3:
		// 3. Visus trumpinti/ilginti vienodai (tiek kart�, kiek liepia greitinimo_koef)
		trumpinimo_euristika_skardieji_scenarijus3 (
			pirmojo_burbulo_nr, paskutiniojo_burbulo_nr, kontekstas->greitinimo_koef, kontekstas);
		break;
		
	case 4:
		// 4. Visus trumpinti/ilginti proporcingai, priklausomai nuo fonemos pavadinimo 
		// (balsius labiausiai, kitus ma�iau, sprogstamuosius dar ma�iau, r nekeisti)
		trumpinimo_euristika_skardieji_scenarijus3 (
			pirmojo_burbulo_nr, paskutiniojo_burbulo_nr,  
			koreguoti_greitinimo_koef_scenarijus4 (kontekstas), kontekstas);
		break;
		
	case 5:
		// 5. Stengtis pritempti iki tos fonemos ilgio vidurkio, priklausomai nuo scenarijaus5_koeficientas
		
		trumpinimo_euristika_skardieji_scenarijus3 (
			pirmojo_burbulo_nr, paskutiniojo_burbulo_nr,
			koreguoti_greitinimo_koef_scenarijus5 (kontekstas->fonemos_pabaiga - kontekstas->fonemos_pradzia, kontekstas), kontekstas);
		break;
		
	default:
		kontekstas->keiciamu_burbulu_sk = 0;
		break;
				
	}
	
}

/*********************************************************
 *********************************************************
             PAGRINDIN�S FUNKCIJOS
 *********************************************************
 *********************************************************/

/*********************************************************
Dusli�j� gars� apdorojimas - i�kvie�iama trumpinimo ar ilginimo funkcija, 
priklausomai nuo greitinimo koeficiento
*********************************************************/
void euristika_duslieji (struct tkontekstas * kontekstas)
{
	if (kontekstas->greitinimo_koef < 1)
		trumpinimo_euristika_duslieji (kontekstas);
	else if (kontekstas->greitinimo_koef == 1)
	{
		kontekstas->keiciamu_burbulu_sk = 0;
	} 
	else
		ilginimo_euristika_duslieji (kontekstas);
}

/*********************************************************
Skard�i�j� gars� apdorojimas - i�kvie�iama trumpinimo ar ilginimo funkcija, 
priklausomai nuo greitinimo koeficiento
*********************************************************/
void euristika_skardieji (struct tkontekstas * kontekstas)
{
	if (kontekstas->greitinimo_koef < 1)
		trumpinimo_euristika_skardieji (kontekstas);
	else if (kontekstas->greitinimo_koef == 1)
	{
		kontekstas->keiciamu_burbulu_sk = 0;
	} 
	else
		ilginimo_euristika_skardieji (kontekstas);
}

/*********************************************************
Pagrindin� euristikos funkcija.
Parenka burbulus signalo trumpinimui (kalb�jimo tempo greitinimui).
Bandomos �vairios euristikos.
Svarbu: 
1) Gr��ina burbulus, i�rikiuotus eil�s tvarka (!!!).
2) Gr��inti burbulai gali persidengti tik puse burbulo 
(kaip pik� pagrindu suformuot� burbul� atveju).
//3) Vis� gr��int� burbul� reik�m�s "pikai" yra vienodos 
//(t.y. arba visi burbulai suformuoti pik� pagrindu, arba n� vieno).
*********************************************************/
void euristika (struct tkontekstas * kontekstas)
{
	/* Algoritmas.
		Jei ne x, f, p, t, k, s, S, _, r, R, z, Z, H - daryti pagal pikus. 
			Tikrinti, ar yra i�metam� burbul� (pagal pik� skai�i� ir burbul� centrus).
			Jei yra - i�mesti, jei n�ra - nedaryti nieko.
		Jei x, f, p, t, k, s, S, _ - daryti be pik� 
			(parinkti ir i�mesti gabaliuk� i� vidurio, link prad�ios).
		Jei r, R - nedaryti nieko.
		Jei z, Z, h - tikrinti, ar taisyklingai pasiskirst� pikai. 
			Jei taip, naudoti pikus, 
			jei tr�ksta tokio piko, pa�iam prid�ti, 
			o jei ma�ai pik�, tai daryti ne pagal pikus.*/
	
	switch (kontekstas->fonemos_klase) {

	case FONEMU_KLASE_SKARDIEJI:
		// Skardieji garsai, kuri� trumpinimui naudojame pik� informacij�.
	case FONEMU_KLASE_RR:
		// Fonemas r, R, jei reikia, trumpiname/ilginame, naudodami pik� informacij� (jei jos yra pakankamai. Jei ne - fonemos nekei�iame).
		euristika_skardieji (kontekstas);
		break;

	case FONEMU_KLASE_DUSLIEJI:
		// Duslieji garsai, kuri� trumpinimui nenaudojame pik� informacijos.
		euristika_duslieji (kontekstas);
		break;

	default:
		// Dar nenustatyta fonemos klas�. Tokio atvejo netur�t� b�ti, o jei yra, rei�kia, ka�kas negerai su realizacija, "internal error". K� daryti tokiu atveju? Kol kas nekei�iame visai.
		kontekstas->keiciamu_burbulu_sk = 0;
		break;
	}
}

