/*
 (w) 2000 Matthias Kluth mk@helinet.de

 compile:  gcc al2alaw.c -o al2alaw
   Usage:  sox sound.wav sound.al; cat sound.al | al2alaw > sound.alaw
 */

#include <stdio.h>
unsigned char creverse( unsigned char inb ) {
   unsigned char outb=0, i;

   //reverse bit order within byte
   for (i=0 ;i < 8; i++) {
        outb=outb << 1;
        if (inb & 1)
                outb=outb | 1;
        inb=inb >> 1;
   }
   return outb;
}

main (void) {
	unsigned int wInWord;
	unsigned char bOutByte;
	while ( !feof(stdin) ) {
		fread( &wInWord, 1, 1, stdin);
		bOutByte = creverse(wInWord);
		fwrite(&bOutByte, 1, 1, stdout);
	}
}

