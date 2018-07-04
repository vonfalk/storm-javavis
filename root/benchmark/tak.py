def tak(x, y, z):
    if y < x:
        return tak(tak(x-1, y, z), tak(y-1, z, x), tak(z-1, x, y))
    else:
        return z

import time

# takes 24m30s (in Linux VM)
def testTak():
    start = time.time()

    for i in range(12):
        print("Tak ", i)
        print("=> ", tak(2*i, i, 0))

    print("Total time: {}".format(time.time() - start))

for i in range(30):
    testTak()
