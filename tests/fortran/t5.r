# ratfor, which f77 runs on a .r file: the sum of the even numbers
# to ten, and the first power of three past a hundred
	integer i, s
	s = 0
	for (i = 1; i <= 10; i = i + 1)
		if (mod(i, 2) == 0)
			s = s + i
	write (6, 10) s
10	format (' RATFOR', i4)
	i = 1
	while (i < 100)
		i = i * 3
	write (6, 20) i
20	format (' WHILE', i4)
	end
