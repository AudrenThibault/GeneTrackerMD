#!/bin/sh
# Montre ce que contient une sauvegarde de cartouche — ou toutes celles d'un
# dossier. Sert a repondre a « laquelle de ces sauvegardes porte mon travail ? »
# quand l'EverDrive en a laisse une par nom de ROM.
#
#   ./outils/sauvegardes.sh /Volumes/<carte>/EDMD/SAVE
#   ./outils/sauvegardes.sh ~/Documents/geneTrackerTUTU_RAM.bin
set -e
ICI=$(cd "$(dirname "$0")" && pwd)
R="$ICI/.."
cc -O1 -w -DMD_HORS_CONSOLE -I"$R/moteur/morceau" \
   "$ICI/lit_sauvegarde.c" "$R/moteur/morceau/md_codec.c" -o "$ICI/lit_sauvegarde"
"$ICI/lit_sauvegarde" "$@"
