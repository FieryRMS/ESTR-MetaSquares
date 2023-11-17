#! /usr/bin/bash
pkill -f Genetic\ AI.py
rm nohup.out
nohup python3 Genetic\ AI.py & tail -f nohup.out