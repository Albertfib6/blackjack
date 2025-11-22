#!/bin/bash

# COLORES
F="\033[42m"      # Verde
FN="\033[102m"    # Verde claro
T="\033[30m"      # Texto negro
RESET="\033[0m"

clear

linea() {
    if (( $1 % 2 == 0 )); then
        echo -e "|${F}                                                                              ${RESET}|"
    else
        echo -e "|${FN}                                                                              ${RESET}|"
    fi
}

echo -e "+------------------------------------------------------------------------------+"
echo -e "|${F}${T}                           BLACKJACK TABLE (25x80)                           ${RESET}|"

linea 1
echo -e "|${F}                        +---------------+                                     ${RESET}|"
echo -e "|${F}                        |   DEALER      |                                     ${RESET}|"
echo -e "|${F}                        |  [10]  [??]   |               [####] Shoe           ${RESET}|"
echo -e "|${F}                        +---------------+                                     ${RESET}|"
linea 2
linea 3

echo -e "|${F}   +--------+   +--------+   +--------+   +--------+   +--------+             ${RESET}|"
echo -e "|${F}   |P1      |   |P2      |   |P3      |   |P4      |   |P5      |             ${RESET}|"
echo -e "|${F}   |[A] [8] |   |[K] [??]|   |[ ] [ ] |   |[ ] [ ] |   |[ ] [ ] |             ${RESET}|"
echo -e "|${F}   +--------+   +--------+   +--------+   +--------+   +--------+             ${RESET}|"

echo -e "|${F}    Bet:25$     Bet:50$     Bet:10$     Bet:15$     Bet:5$                   ${RESET}|"

for i in {4..10}; do linea $i; done

echo -e "|${F}  Reglas: Dealer se planta en 17 | Blackjack paga 3:2 | Máx 5 jugadores       ${RESET}|"
linea 11
echo -e "+------------------------------------------------------------------------------+"