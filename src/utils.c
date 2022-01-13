//
// Created by paul on 11/01/22.
//

#include "../headers/utils.h"
#include <stdio.h>
#include <string.h>

char intToChar(int str){
    if((str > 96 && str < 123) || (str > 64 && str < 91)) return (char)str;
    else return 0;
}

void intToString(char** tmp, int str){
    (*tmp)[0] = str;
    (*tmp)[1] = '\0';
}