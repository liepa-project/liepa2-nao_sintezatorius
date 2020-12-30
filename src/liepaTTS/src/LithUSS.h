#pragma once
struct event {short Id; short phonviz; int charOffset; long signOffset;}; 

int initLUSS(char*, char*);

int synthesizeWholeText(char*, short*, unsigned int*, event*, int*, int, int);
void unloadLibraries();
