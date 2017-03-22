%% Simple example of frames

cylinder ako [] with
[
	height:
	[
		range		number(new_value) and new_value > 0,
		help		print("Height must be a positive number"),
		if_needed	ask_user,
		if_removed	remove volume,
		cache		true
	],
	radius:
	[
		range		number(new_value) and new_value > 0,
		help		print("Radius must be a positive number"),
		if_needed	ask_user,
		if_removed	remove cross_section,
		cache		true
	],
	cross_section:
	[
		if_needed	pi * radius ** 2,
		if_removed	remove volume,
		cache		true
	],
	volume:
	[
		if_needed	cross_section * height,
		cache		true
	]
]!

c isa [cylinder] with [height: 2, radius: 3]!
d isa [cylinder] with [colour: red]!
