class Cons:
    def __init__(self, value, next):
        self.value = value
        self.next = next

def create(len):
    if len > 0:
        return Cons(len, create(len - 1))
    else:
        return None

def append(a, b):
    if a is None:
        return b
    else:
        return Cons(a.value, append(a.next, b))

def reverse(a):
    if a is None:
        return None
    else:
        return append(reverse(a.next), Cons(a.value, None))

def output(a):
    result = ""
    while not a is None:
        result = result + " " + str(a.value)
        a = a.next
    return result

import sys
sys.setrecursionlimit(1050)

# print(output(create(10)))
# print(output(reverse(create(10))))
# takes about 39s

for i in range(100):
    reverse(create(1000))
