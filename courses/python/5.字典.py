def add_to_dict(cls):
    def to_dict(self):
        result = {}
        if hasattr(self, '__dict__'):
            result.update(self.__dict__)
        if hasattr(self, '__slots__'):
            for slot in self.__slots__:
                if hasattr(self, slot):
                    result[slot] = getattr(self, slot)
        
        return result

    cls.to_dict = to_dict
    return cls
@add_to_dict
class Person:
    __slots__ = ('name', 'age')   
    def __init__(self, name, age):
        self.name = name
        self.age = age

p = Person('Alice', 30)
print(p.to_dict()) 
