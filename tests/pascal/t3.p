program structures(output);
{ sets, arrays, records, strings, pack and unpack, pointers }
type
	color = (red, orange, yellow, green, blue, violet);
	colors = set of color;
	digits = set of 0..9;
	letters = set of 'a'..'z';
	point = record
		x, y: integer
	end;
	link = ^node;
	node = record
		value: integer;
		next: link
	end;
var
	c: color;
	p, q: colors;
	d: digits;
	l: letters;
	ch: char;
	a: array [1..10] of integer;
	m: array [1..3, 1..4] of -1000..1000;
	pt: point;
	pts: array [1..3] of point;
	name, other: packed array [1..8] of char;
	u: array [1..8] of char;
	head, n: link;
	i, j, sum: integer;
begin
	p := [red, yellow, blue];
	q := [yellow..violet];
	write('sets');
	for c := red to violet do
		if c in p * q then
			write(' ', ord(c):1);
	for c := red to violet do
		if c in p + q then
			write(' +', ord(c):1);
	for c := red to violet do
		if c in p - q then
			write(' -', ord(c):1);
	writeln;
	writeln('relations ', p = q:1, ' ', p <> q:1, ' ', [yellow] <= p:1,
	    ' ', p >= [red, blue]:1, ' ', q <= p:1);
	d := [];
	for i := 0 to 9 do
		if odd(i) then
			d := d + [i];
	i := 4;
	j := 6;
	d := d + [i..j, 0];
	write('digits');
	for i := 0 to 9 do
		if i in d then
			write(' ', i:1);
	writeln;
	l := ['a'..'e', 'x', 'z'];
	ch := 'c';
	writeln('letters ', ch in l:1, ' ', 'y' in l:1, ' ', succ(ch) in l:1);
	for i := 1 to 10 do
		a[i] := i * i;
	sum := 0;
	for i := 1 to 10 do
		sum := sum + a[i];
	writeln('squares ', sum:1);
	for i := 1 to 3 do
		for j := 1 to 4 do
			m[i, j] := i * 10 + j;
	writeln('matrix ', m[2, 3]:1, ' ', m[3, 4] - m[1, 1]:1);
	pt.x := 3;
	pt.y := -4;
	pts[2] := pt;
	with pts[2] do
		writeln('record ', x * x + y * y:1);
	name := 'pascal  ';
	other := 'pascal  ';
	writeln('strings ', name = other:1, ' ', name < 'pasta   ':1, ' ',
	    name > 'pas     ':1);
	unpack(name, u, 1);
	u[1] := 'P';
	pack(u, 1, other);
	writeln('[', other, '] [', name, ']');
	head := nil;
	for i := 1 to 5 do begin
		new(n);
		n^.value := i * 11;
		n^.next := head;
		head := n
	end;
	sum := 0;
	n := head;
	write('list');
	while n <> nil do begin
		sum := sum + n^.value;
		write(' ', n^.value:1);
		n := n^.next
	end;
	writeln(' sum ', sum:1);
	dispose(head)
end.
