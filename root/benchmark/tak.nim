import times, os

proc tak(x, y, z :int): int =
     if y < x:
       result = tak(tak(x - 1, y, z), tak(y - 1, z, x), tak(z - 1, x, y))
     else:
       result = z

proc test() =
    let time = cpuTime()

    for i in 0..11:
        echo("Tak ", i)
        echo("=> ", tak(2*i, i, 0))

    echo "Total time: ", cpuTime() - time

for i in 1..100:
    test()
