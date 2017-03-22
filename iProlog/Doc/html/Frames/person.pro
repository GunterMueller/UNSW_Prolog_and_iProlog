person ako [] with
[
	name:
	[
		range		atom,
		help		print("The name should be a string."),
		if_needed	ask_user,
		if_new		ask_user,
		cache		true
	],
	sex:
	[
		range		[male, female],
		help		print("Sex can only be male or female, not ", new_value),
		if_needed	ask_user,
		if_replaced	print("Are you sure you want a sex change?"),
		if_removed	print("Are you sure you want the sex removed?"),
		cache		true
	],
	year_of_birth:
	[
		range		year(current_date) - 120 .. year(current_date),
		help		print("Invalid year of birth."),
		if_needed	ask_user("Year of birth"),
		cache		true
	],
	age:
	[
		cache		true,
		if_needed	year(current_date) - year_of_birth
	],
	parents:
	[
		multivalued	true,
		range		person,
		help		print("The value in a parents slot must be a person.")
	],
	height:
	[
		range		10..220,
		cache		true,
		help
		((		if new_value < 10 then
					print(new_value, "cm is too short."),
				if new_value > 220 then
					print(new_value, "cm is too tall."),
				print("The height should be between 10 and 220cm.")
		)),
		if_needed	ask_user("What is the height of ", name)
	],
	weight:
	[
		range		1..150	,
		if_needed	ask_user("What is the weight of ", name),
		cache		true,
		if_added	if new_value > 100 then
					print("Your ", current_slot, " is too high!")
	],
	occupation:
	[
		range		atom,
		help		print("The occupation should be a string."),
		if_needed	ask_user("What is the occupation"),
		cache		true,
		if_removed	print("I used to be a ", old_value, ".")
	]
]!
