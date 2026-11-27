program control(output);
{ control: procedures, functions and recursion, var parameters, the
  display, for, while, repeat, case on each width, goto out of calls }
label 9;
const
	n = 10;
var
	i, j, k, moves: integer;
	s: -100..100;
	c: char;
	b: boolean;

function fact(n: integer): integer;
begin
	if n <= 1 then
		fact := 1
	else
		fact := n * fact(n - 1)
end;

function fib(n: integer): integer;
begin
	if n < 2 then
		fib := n
	else
		fib := fib(n - 1) + fib(n - 2)
end;

procedure swap(var a, b: integer);
var
	t: integer;
begin
	t := a;
	a := b;
	b := t
end;

procedure outer(x: integer);
var
	y: integer;

	procedure inner(z: integer);
	begin
		y := y + x * z
	end;

begin
	y := 0;
	inner(1);
	inner(2);
	inner(3);
	writeln('outer ', x:1, ' ', y:1)
end;

procedure hanoi(n, a, b, c: integer);
begin
	if n > 0 then begin
		hanoi(n - 1, a, c, b);
		moves := moves + 1;
		hanoi(n - 1, c, b, a)
	end
end;

procedure escape(d: integer);
begin
	if d = 0 then
		goto 9;
	escape(d - 1);
	writeln('not here')
end;

begin
	writeln('fact ', fact(10):1, ' ', fact(12):1);
	writeln('fib ', fib(20):1);
	writeln('width', fact(10), fact(7):4, -5:3);
	i := 3;
	j := 4;
	swap(i, j);
	writeln('swap ', i:1, ' ', j:1);
	outer(5);
	moves := 0;
	hanoi(10, 1, 2, 3);
	writeln('hanoi ', moves:1);
	k := 0;
	for i := 1 to n do
		k := k + i;
	write('for ', k:1);
	k := 0;
	for i := n downto 1 do
		k := k * 2 + i;
	writeln(' ', k:1);
	for c := 'a' to 'e' do
		write(c);
	for s := 3 downto -2 do
		write(' ', s:1);
	writeln;
	i := 0;
	while i < 100 do
		i := i * 2 + 1;
	write('while ', i:1);
	i := 1000;
	repeat
		i := i div 3
	until i < 10;
	writeln(' repeat ', i:1);
	for i := 0 to 5 do
		case i of
		0:	write('zero');
		1, 2:	write(' small');
		3:	write(' three');
		4, 5:	write(' big')
		end;
	writeln;
	for c := 'x' to 'z' do
		case c of
		'x':	write('ex');
		'y':	write(' why');
		'z':	writeln(' zed')
		end;
	s := -50;
	case s of
	-50:	writeln('minus fifty');
	50:	writeln('fifty')
	end;
	j := 100000;
	case j of
	100000:	writeln('a hundred thousand');
	1:	writeln('one')
	end;
	i := 5;
	b := (i > 3) and not (j < 0) or false;
	writeln('boolean ', b:1, ' ', not b:1, ' ', i = 5:1, ' ', i <> 5:1);
	escape(5);
	writeln('not here either');
9:
	writeln('goto out of 6 calls')
end.
