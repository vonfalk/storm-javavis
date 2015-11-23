program factorial;
var
    n : integer;

{ write_int -- write integer on standard output }
procedure write_int(val : integer);
const
    ASCII0 = 48;       { ascii value of '0' }
    MINUS = 45;
var
    c : integer;
    buf : array[10] of integer; { no integer can occupy more than 10 digits }
    bufp : integer;
begin
    if (val = 0) then
	write(ASCII0);
	return;
    end;
    if (val < 0) then
	write(MINUS);
        val := -val;
    end;
    bufp := 0;
    while val > 0 do
    	c := val mod 10;
	buf[bufp] := c + ASCII0;
	bufp := bufp + 1;
	val := val div 10;
    end;
    while (bufp > 0) do 
	bufp := bufp - 1;
    	write(buf[bufp]);
    end;
end;

function fact(n : integer) : integer;
var
    temp : integer;
begin
    if n = 0 then
    	return 1;
    else
    	temp := fact(n - 1);
	return n * temp;
    end;
end;

function fib(n : integer) : integer;
begin
   if n < 2 then
      return n;
   else
      return fib(n - 1) + fib(n - 2);
   end;
end;

function fib_iter(n : integer) : integer;
var
   tmp	: integer;
   now	: integer;
   prev	: integer;
begin
   if n = 0 then
      return 0;
   end;

   prev := 0;
   now := 1;
   while n > 1 do
      tmp := now + prev;
      prev := now;
      now := tmp;
      n := n - 1;
   end;
   return now;
end;

begin
    n := read();
    n := n - 48;
    write_int(fact(n));
    write(10);
end.
