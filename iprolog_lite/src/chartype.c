/*		Character Table for determining lexical type		*/


#include "prolog.h"


chartype chtype[128] =
{ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	PUNCTCH,
/* nul		soh		stx		etx		eot    */
 ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	WHITESP,
/* enq		ack		bel		bs		ht     */
 WHITESP,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* nl		vt		np		cr		so     */
 ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* si		dle		dc1		dc2		dc3    */
 ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* dc4		nak		syn		etb		can    */
 ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* em		sub		esc		fs		gs     */
 ILLEGALCH,	ILLEGALCH,	WHITESP,	PUNCTCH,	STRINGCH,
/* rs		us		sp		!		"      */
 SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	QUOTECH,
/* #		$		%		&		'	*/
PUNCTCH,	PUNCTCH,	SYMBOLCH,	SYMBOLCH,	PUNCTCH,
/* (		)		*		+		,	*/
 SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	DIGIT,		DIGIT,
/* -		.		/		0		1	*/
 DIGIT,		DIGIT,		DIGIT,		DIGIT,		DIGIT,
/* 2		3		4		5		6	*/
 DIGIT,		DIGIT,		DIGIT,		SYMBOLCH,	PUNCTCH,
/* 7		8		9		:		;	*/
 SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	SYMBOLCH,
/* <		=		>		?		@      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* A		B		C		D		E      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* F		G		H		I		J      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* K		L		M		N		O      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* P		Q		R		S		T      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* U		V		W		X		Y      */
 WORDCH,	PUNCTCH,	SYMBOLCH,	PUNCTCH,	SYMBOLCH,
/* Z		[		\		]		^      */
 WORDCH,	SYMBOLCH,	WORDCH,		WORDCH,		WORDCH,
/* _		`		a		b		c      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* d		e		f		g		h      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* i		j		k		l		m	*/
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* n		o		p		q		r	*/
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* s		t		u		v		w	*/
 WORDCH,	WORDCH,		WORDCH,		PUNCTCH,	PUNCTCH,
/* x		y		z		{		|	*/
 PUNCTCH,	SYMBOLCH,	PUNCTCH};
/* }		~		del					*/
/* $Extended$File$Info$
 */
