program files(input, output, data);
{ files: text files named and not, files of records and their windows,
  a file the program's heading names, a number at the end of a file that
  has no newline at its end, and the input }
type
	item = record
		key: integer;
		val: real
	end;
var
	t, scratch: text;
	f: file of item;
	data: text;
	i, n, sum, lines, chars: integer;
	x, rsum: real;
	c: char;
begin
	rewrite(t, 'numbers');
	for i := 1 to 5 do
		writeln(t, i:4, i * 1.5:8:2);
	writeln(t, 'the end');
	reset(t, 'numbers');
	sum := 0;
	rsum := 0;
	for i := 1 to 5 do begin
		read(t, n, x);
		sum := sum + n;
		rsum := rsum + x
	end;
	readln(t);
	writeln('read ', sum:1, ' ', rsum:5:2);
	lines := 0;
	chars := 0;
	while not eof(t) do begin
		while not eoln(t) do begin
			read(t, c);
			chars := chars + 1
		end;
		readln(t);
		lines := lines + 1
	end;
	writeln('then ', lines:1, ' line of ', chars:1);
	rewrite(f, 'items');
	for i := 1 to 4 do begin
		f^.key := i;
		f^.val := i / 4;
		put(f)
	end;
	reset(f, 'items');
	rsum := 0;
	n := 0;
	while not eof(f) do begin
		rsum := rsum + f^.val * f^.key;
		n := n + 1;
		get(f)
	end;
	writeln('items ', n:1, ' ', rsum:5:2);
	rewrite(data);
	writeln(data, 'in data');
	reset(data);
	write('data: ');
	while not eoln(data) do begin
		read(data, c);
		write(c)
	end;
	writeln;
	rewrite(scratch);
	write(scratch, 12345);
	reset(scratch);
	read(scratch, n);
	writeln('scratch ', n:1);
	remove('numbers');
	read(n, x);
	readln;
	writeln('input ', n * 2:1, ' ', x * 2:6:3);
	read(n);
	writeln('negative ', n:1);
	readln;
	while not eoln do begin
		read(c);
		write(c)
	end;
	readln;
	writeln;
	writeln('eof ', eof:1)
end.
