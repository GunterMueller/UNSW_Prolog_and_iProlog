city ako [] with
[
	population:	[range	number]
]!

state ako [] with
[
	capital:	[range	atom],
	population:	[range	number]
]!

country ako [] with
[
	region:		[range	atom],
	latitude:	[range	number],
	longitude:	[range	number],
	area:		[range	number],
	population:	[range	number],
	capital:	[range	atom],
	currency:	[range	atom]
]!


'NSW' isa [state] with [capital: 'Sydney']!
'Victoria' isa [state] with [capital: 'Melbourne']!

'Australia' isa [country] with
[
	region:		'Australasia',
	latitude:	-23,
	longitude:	-135,
	area:		2967,
	population:	20,
	capital:	'Canberra',
	currency:	'AUD'
]!

script('../scripts/geography.script')!

go :- probot(geography).
