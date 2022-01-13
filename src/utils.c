//
// Created by paul on 11/01/22.
//

#include "../headers/utils.h"
#include <stdio.h>
#include <string.h>

char intIsChar(int str){
    if((str > 96 && str < 123) || (str > 64 && str < 91)) return (char)str;
    else return 0;
}

char* toOpt(char* str){
    switch (str[0]) {
        case 'w': {
            return "w";
        }
        case 'W': {
            return "W";
        }
        case 'D': {
            return "D";
        }
        case 'r': {
            return "r";
        }
        case 'R': {
            return "R";
        }
        case 'd': {
            return "d";
        }
        case 't': {
            return "t";
        }
        case 'l': {
            return "l";
        }
        case 'c': {
            return "c";
        }
        case 'u': {
            return "u";
        }
    }
    return 0;
}