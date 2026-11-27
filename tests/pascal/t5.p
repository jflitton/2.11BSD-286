program builtins(output);
{ Berkeley Pascal's own: the arguments, the date and time, the clocks,
  random numbers, undefined, message and null }
var
	i, old: integer;
	s: packed array [1..8] of char;
	d, t: alfa;
	x: real;
begin
	writeln('argc ', argc:1);
	for i := 0 to argc - 1 do begin
		argv(i, s);
		writeln('argv ', i:1, ' [', s, ']')
	end;
	date(d);
	time(t);
	writeln('date [', d[3], d[7], '] time [', t[4], t[7], ']');
	writeln('clocks ', clock >= 0:1, ' ', sysclock >= 0:1, ' ',
	    wallclock > 1000000000:1);
	old := seed(12345);
	writeln('seed ', old:1);
	write('random');
	for i := 1 to 6 do
		write(' ', trunc(random(1.0) * 1000000):1);
	writeln;
	old := seed(12345);
	writeln('again ', trunc(random(0.0) * 1000000):1, ' ', old:1);
	x := 1.0;
	writeln('undefined ', undefined(x):1);
	null;
	message('to the message file');
	writeln('done')
end.
