from enum import Enum

class weekend(Enum):
	MONDAY	= 1
	TUESDAY	= 2
	THIRSDAY= 3
	FORTHDAY= 4
	FRIDAY	= 5
	SATURDAY= 6
	SUNDAY	= 7

#print(f"{weekend.MONDAY.key}")
a	= 1
print(f"{a if a>0 else 0}")
