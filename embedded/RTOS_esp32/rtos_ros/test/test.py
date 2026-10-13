# 迭代器：需要写类、__init__、__iter__、__next__
class SquareIterator:
    def __init__(self, max_num):
        self.max_num = max_num
        self.current = 1

    def __iter__(self):
        return self

    def __next__(self):
        if self.current > self.max_num:
            raise StopIteration
        res = self.current ** 2
        self.current += 1
        return res

# 使用
it = SquareIterator(5)
for num in it:
    print(num)