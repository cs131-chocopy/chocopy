def f():
    global x
    x = x + 1
x: int = 0
f()
print(x)
