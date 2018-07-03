import times, os

type
    Cons = ref object
       value: int
       next: Cons

proc cons(value: int, next: Cons): Cons =
    new(result)
    result.value = value
    result.next = next

proc create(len: int): Cons =
    if len > 0:
        result = cons(len, create(len - 1))
    else:
        result = nil

proc append(a, b: Cons): Cons =
    if a == nil:
        result = b
    else:
        result = cons(a.value, append(a.next, b))

proc reverse(a: Cons): Cons =
    if a == nil:
        result = nil
    else:
        result = append(reverse(a.next), cons(a.value, nil))

let time = cpuTime()

#takes 1.4s in release
for i in 1..100:
    var v = reverse(create(1000))

echo "Total time: ", cpuTime() - time
