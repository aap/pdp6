#include <stdio.h>
#include <stdint.h>
#include "pdp6common.h"

word fw(hword l, hword r) { return ((word)l << 18) | (word)r; }
hword left(word w) { return (w >> 18) & 0777777; }
hword right(word w) { return w & 0777777; }

word item[01000000];
int itemp;

FILE *infp;
int type;

int peekc;
int ch(void){
	int c;
	if(peekc){
		c = peekc;
		peekc = 0;
		return c;
	}
tail:
	c = getc(infp);
	if(c == ';'){
		while(c = getc(infp), c != '\n')
			if(c == EOF) return c;
		goto tail;
	}
	if(c == '\t' || c == '\n')
		c = ' ';
	return c;
}
int chsp(void){
	int c;
	while(c = ch(), c == ' ');
	return c;
}
int peek(void) { return peekc = ch(); }
int peeksp(void) { return peekc = chsp(); }
int isoct(int c) { return c >= '0' && c <= '7'; }
word num(void){
	word w;
	int c;

	w = 0;
	while(c = ch(), c != EOF){
		if(!isoct(c)){
			peekc = c;
			break;
		}
		w = (w<<3) | c-'0';
	}
	return w;
}
void dumpitem(void){
	word head;
	int type, sz;
	int p;
	int i;

	type = left(item[0]);
	sz = right(item[0]);
	printf("%06o %06o\n", type, sz);
	p = 1;
	while(sz){
		head = item[p++];
		i = 18;
		while(sz && i--){
			printf(" %06o%c%06o%c\n",
			       left(item[p]),
			       head & 0400000000000 ? '\'' : ' ',
			       right(item[p]),
			       head & 0200000000000 ? '\'' : ' ');
			head <<= 2;
			p++;
			sz--;
		}
	}
}
void type0(void)
{
	int c;
	while(c = chsp(), c != EOF){
		switch(c){
		case ':':
			type = num();
			printf("TYPE %o\n", type);
			return;
		}
	}
}
void type1(void)
{
	int c;
	word bits;
	hword l, r;
	word head;
	word block[18];
	int n;
	int sz;
	int i;

	sz = 0;
	itemp = 0;
	item[itemp++] = 0;

	n = 0;
	head = 0;
	while(c = chsp(), c != EOF){
		switch(c){
		case ':':
			type = num();
			printf("TYPE %o\n", type);
			goto out;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			peekc = c;
			bits = 0;
			l = num();
			if(peeksp() == '\''){
				bits |= 2;
				chsp();
			}
			while(c = ch(), !isoct(c));
			peekc = c;
			r = num();
			if(peeksp() == '\''){
				bits |= 1;
				chsp();
			}
			block[n++] = fw(l, r);
			head = head<<2 | bits;
			/* write out one block */
			if(n == 18){
				item[itemp++] = head;
				for(i = 0; i < 18; i++){
					item[itemp++] = block[i];
					sz++;
				}
				n = 0;
			}
			break;
		}
	}
out:
	/* write incomplete block */
	if(n){
		head <<= (18-n)*2;
		item[itemp++] = head;
		for(i = 0; i < n; i++){
			item[itemp++] = block[i];
			sz++;
		}

	}
	/* write item header */
	item[0] = fw(01, sz);
	dumpitem();
}
int main(){
	int c;
	word n;

	infp = stdin;
	type = 0;
	while(peek() != EOF)
		switch(type){
		case 0:
			type0();
			break;
		case 1:
			type1();
			break;
		default:
			goto out;
		}
out:

	return 0;
}
