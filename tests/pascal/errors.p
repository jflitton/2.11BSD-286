program errors(input, output);
{ the errors px finds as a program runs, one for each word it is given,
  each with the calls it was made in.  At the end of the input, a readln
  finds it, and the next one errs. }
var
	which: packed array [1..8] of char;
	a: array [1..5] of integer;
	i, z: integer;
	s: 1..10;
	p: ^integer;
	x: real;
	c: char;

procedure deep(n: integer);
begin
	if n > 0 then
		deep(n - 1)
	else
		a[n] := 1
end;

procedure forever(n: integer);
var
	space: array [1..1000] of integer;
begin
	space[1] := n;
	forever(n + 1)
end;

begin
	argv(1, which);
	z := 0;
	if which = 'range   ' then
		deep(2)
	else if which = 'div     ' then
		i := 1 div z
	else if which = 'mod     ' then
		i := 1 mod z
	else if which = 'real    ' then
		x := 1 / z
	else if which = 'overflow' then begin
		x := 1.0e300;
		x := x * x
	end else if which = 'nil     ' then begin
		p := nil;
		p^ := 1
	end else if which = 'case    ' then
		case z of
		1:	writeln('one')
		end
	else if which = 'subrange' then begin
		i := 11;
		s := i
	end else if which = 'chr     ' then
		c := chr(z + 300)
	else if which = 'sqrt    ' then
		x := sqrt(z - 1.0)
	else if which = 'ln      ' then
		x := ln(z)
	else if which = 'read    ' then
		read(i)
	else if which = 'eof     ' then begin
		readln;
		readln;
		readln
	end else if which = 'halt    ' then
		halt
	else if which = 'stlimit ' then begin
		stlimit(20);
		for i := 1 to 100 do
			z := z + 1
	end else if which = 'argv    ' then
		argv(5, which)
	else if which = 'stack   ' then
		forever(1)
	else
		writeln('no error')
end.
