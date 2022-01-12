//
// Created by paul on 11/01/22.
//

#include "../headers/utils.h"

char toChar(int str){
    if((str > 96 && str < 123) || (str > 64 && str < 91)) return (char)str;
    else return 0;
}