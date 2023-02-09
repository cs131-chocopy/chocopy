class A(object):
    x: int = 10
    def call(self: "A"):
        print(1)
    def print(self: "A"):
        call()
        print(self.x)
a: A = None
a = A()
a.print()
