program prof(output);
{ for pxp: printed again as it is, and with how often each part ran }
var
	i, n, primes: integer;

function prime(n: integer): boolean;
var
	d: integer;
	p: boolean;
begin
	p := n > 1;
	d := 2;
	while p and (d * d <= n) do begin
		if n mod d = 0 then p := false;
		d := d + 1
	end;
	prime := p
end;

begin
primes := 0;
for i := 1 to 50 do if prime(i) then primes := primes + 1;
if primes > 10 then writeln(primes:1, ' primes') else writeln('few')
end.
