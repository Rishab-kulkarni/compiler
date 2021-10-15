%option noyywrap
%{
	#include <stdlib.h>
	#include <stdio.h>
	#include "tokens.h"
%}
	
number [0-9]+
 
%%
 
{number} { return atoi(yytext); }
"int"                             {printf("\t%-30s : %3d\n",yytext,INT);}
 
%%
 
int main(int argc, char **argv) {

	yyin = fopen("file.txt", "r");
	yylex();
}