proc tac(x, y, z :int): int =
     if y < x:
       result = tac(tac(x - 1, y, z), tac(y - 1, z, x), tac(z - 1, x, y))
     else:
       result = z

for i in 0..12:
    echo("Tac ", i)
    echo("=> ", tac(i, 0, -i))

# Built with -d:release, takes 19s + 2s compile