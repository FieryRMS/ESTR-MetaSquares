#! /usr/bin/bash
mkdir -p .exes
gcc -Wall -fexceptions -Wshadow -Wextra -O3 basic.c ai_player_1155205640.c -o .exes/basic.exe -lm
gcc -shared -o .exes/ai_player1.dll -fPIC -O3 ai_player_1155205640.c -lm
cp -fr .exes/ai_player1.dll .exes/ai_player2.dll
pkill -f Genetic\ AI.py
rm nohup.out
nohup python3 Genetic\ AI.py & tail -f nohup.out