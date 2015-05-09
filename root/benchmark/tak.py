def tak(x, y, z):
    if y < x:
        return tak(tak(x-1, y, z), tak(y-1, z, x), tak(z-1, x, y))
    else:
        return z

# takes 24m30s (in Linux VM)
def testTak():
    for i in range(13):
        print("Tak ", i)
        print("=> ", tak(i, 0, -i))

testTak()
