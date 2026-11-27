program arith(output);
{ arithmetic: integers of 2 bytes and of 4, reals, the standard
  functions, and how each is written }
var
	i, j, k: integer;
	s, t: -32768..32767;
	x, y: real;
	r: 0..255;
begin
	i := 17;
	j := -5;
	writeln('div ', i div 5:1, ' ', i div j:1, ' ', (-i) div 5:1, ' ',
	    (-i) div j:1);
	writeln('mod ', i mod 5:1, ' ', i mod j:1, ' ', (-i) mod 5:1);
	s := 300;
	t := -7;
	writeln('short ', s + t:1, ' ', s - t:1, ' ', s * t:1, ' ',
	    s div t:1, ' ', s mod t:1);
	s := 30000;
	writeln('short times short ', s * s:1);
	i := 70000;
	s := 300;
	writeln('mixed ', s + i:1, ' ', s * i:1, ' ', i - s:1, ' ',
	    i div s:1, ' ', i mod s:1);
	writeln('maxint ', maxint:1, ' ', -maxint:1);
	i := 123456789;
	j := 987;
	writeln('long ', i + j:1, ' ', i - j:1, ' ', i div j:1, ' ', i mod j:1,
	    ' ', j * j * j:1);
	writeln('abs ', abs(-7):1, ' ', abs(i - 2 * i):1, ' sqr ', sqr(12):1, ' ',
	    sqr(j):1);
	writeln('odd ', odd(7):1, ' ', odd(-4):1, ' ', odd(i):1);
	writeln('succ ', succ(5):1, ' ', pred(-5):1, ' ', succ('a'), pred('b'),
	    ' ', ord('A'):1, ' ', chr(66));
	x := 1.5;
	y := -0.25;
	writeln('real ', x + y:6:3, x - y:6:3, x * y:7:3, x / y:6:2, sqr(y):7:4);
	writeln('compare ', x > y:1, ' ', x = 1.5:1, ' ', y >= 0:1, ' ',
	    i < x:1, ' ', 2 = 2.0:1);
	i := 17;
	j := -5;
	writeln('int to real ', i / j:6:2, ' ', i + x:6:2, ' ', j * y:6:2);
	writeln('sqrt ', sqrt(2.0):10:6, ' sin ', sin(0.5):9:6, ' cos ',
	    cos(0.5):9:6);
	writeln('exp ', exp(1.0):9:6, ' ln ', ln(10.0):9:6, ' arctan ',
	    arctan(1.0):9:6);
	writeln('trunc ', trunc(2.7):1, ' ', trunc(-2.7):1, ' round ',
	    round(2.5):1, ' ', round(-2.5):1, ' ', round(2.4):1, ' ',
	    round(-0.4):1);
	writeln('expo ', expo(8.0):1, ' ', expo(0.75):1, ' ', expo(1.0):1);
	writeln('e ', x, y:12, 1.0e10:12, -1.0e-10:12);
	writeln('f ', 0.0:6:2, 123.456:10:2, -0.001:8:3, 99.996:7:2);
	i := 255;
	s := -1;
	r := 10;
	writeln('oct ', i:1 oct, ' ', s:1 oct, ' ', i oct, ' hex ', i:1 hex,
	    ' ', s:1 hex, ' ', -i:1 hex, ' ', r:4 hex)
end.
