// ============================================================================
//  QUELLE SAUVEGARDE CONTIENT QUOI.
//
//  L'EverDrive range UNE SAUVEGARDE PAR NOM DE ROM. En essayant tour à tour
//  geneTracker.bin, geneTrackerTUTU.bin et GeneTrackerMD-v0.1.0.bin, on se
//  retrouve avec trois fichiers qui portent le même morceau à trois âges
//  différents — et rien, dans le Finder, ne dit lequel est le bon. On a perdu
//  une soirée à croire que l'import de la DS était en panne alors qu'on lui
//  donnait une sauvegarde périmée.
//
//  Cet outil lit un fichier ou un dossier entier et montre, pour chacun, les
//  morceaux qu'il contient et leur grille SONG. On voit d'un coup d'oeil
//  lequel a le travail qu'on cherche.
//
//  Usage : outils/sauvegardes.sh <fichier-ou-dossier>
// ============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include "md_song.h"
#include "md_codec.h"

// ⚠️ LARGE, ET C'EST VOULU. md_codec_decomprime ne connait pas la taille de
// sa sortie : sur la console elle vaut exactement MD_TAILLE_TOTALE, mais ici
// on lit des fichiers dont rien ne garantit qu'ils soient intacts. Un flux
// abime ecrivait au-dela du tampon et l'outil sautait — sur le fichier meme
// qu'on cherchait a examiner.
static uint8_t brut[1 << 20], utile[32768], morceau[1 << 20];

// La Mega Drive n'expose qu'un octet sur deux de sa mémoire de sauvegarde :
// 64 Ko de fichier pour 32 Ko utiles. Certains émulateurs, eux, écrivent
// l'image déjà compacte — on accepte les deux.
static uint32_t desentrelace(uint32_t n) {
  static const char MAGIE[6] = { 'G','T','L','I','B','1' };
  if (n >= 12) {
    uint32_t m = n / 2;
    if (m > sizeof utile) m = sizeof utile;
    for (uint32_t i = 0; i < m; i++) utile[i] = brut[i * 2 + 1];
    if (!memcmp(utile, MAGIE, 6)) return m;
  }
  if (n >= 12 && !memcmp(brut, MAGIE, 6)) {
    uint32_t m = n > sizeof utile ? (uint32_t)sizeof utile : n;
    memcpy(utile, brut, m);
    return m;
  }
  return 0;
}

static void grille(void) {
  printf("        FM1 FM2 FM3 FM4 FM5 PCM PS1 PS2 PS3 NOI\n");
  int derniere = 0;
  for (int r = 0; r < MD_SONG_LIGNES; r++)
    for (int c = 0; c < MD_CANAUX; c++)
      if (morceau[MD_OFF_SONG + c * MD_SONG_LIGNES + r] != MD_VIDE) derniere = r;
  for (int r = 0; r <= derniere; r++) {
    printf("    %02X  ", r);
    for (int c = 0; c < MD_CANAUX; c++) {
      const uint8_t x = morceau[MD_OFF_SONG + c * MD_SONG_LIGNES + r];
      if (x == MD_VIDE) printf(" -- "); else printf(" %02X ", x);
    }
    printf("\n");
  }
  if (!derniere) printf("    (aucune ligne)\n");
}

static void un_fichier(const char *chemin) {
  fprintf(stderr, "  ... %s\n", chemin);
  FILE *h = fopen(chemin, "rb");
  if (!h) return;
  const uint32_t n = (uint32_t)fread(brut, 1, sizeof brut, h);
  fclose(h);
  const uint32_t m = desentrelace(n);
  if (!m) return;                       // pas une sauvegarde GeneTracker

  printf("\n== %s  (%u octets)\n", chemin, n);
  int trouves = 0;
  for (int e = 0; e < MD_BIB_EMPLACEMENTS; e++) {
    const uint8_t *b = utile + 8 + e * 14;
    const uint16_t off = (uint16_t)(b[10] | (b[11] << 8));
    const uint16_t len = (uint16_t)(b[12] | (b[13] << 8));
    if (!len || (uint32_t)off + len > m) continue;
    char nom[MD_BIB_NOM + 1];
    int k = MD_BIB_NOM;
    while (k > 0 && (b[k - 1] == ' ' || !b[k - 1])) k--;
    memcpy(nom, b, (size_t)k); nom[k] = 0;
    printf("\n  %02d  %-10s  %u octets comprimes\n", e, nom, len);
    memset(morceau, MD_VIDE, sizeof morceau);
    md_codec_decomprime(utile + off, len, morceau);
    grille();
    trouves++;
  }
  if (!trouves) printf("  (bibliotheque vide : rien n'a ete enregistre)\n");
}

int main(int argc, char **argv) {
  if (argc < 2) { fprintf(stderr, "usage: %s <fichier-ou-dossier>\n", argv[0]); return 1; }
  struct stat st;
  if (stat(argv[1], &st) != 0) { perror(argv[1]); return 1; }
  if (S_ISDIR(st.st_mode)) {
    DIR *d = opendir(argv[1]);
    struct dirent *e;
    while (d && (e = readdir(d))) {
      if (e->d_name[0] == '.') continue;
      char c[1024];
      snprintf(c, sizeof c, "%s/%s", argv[1], e->d_name);
      un_fichier(c);
    }
    if (d) closedir(d);
  } else {
    un_fichier(argv[1]);
  }
  return 0;
}
