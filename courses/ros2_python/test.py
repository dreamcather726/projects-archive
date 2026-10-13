

class   MyClass:
    student_name = "John Doe"
    def __init__(self, value):
        self.value = value

    def display_value(self):
        print(f"The value is: {self.value}")
myclass = MyClass(42)
a=myclass.student_name
MyClass.student_name="Jane Smith"
print(MyClass.student_name)         # 输出: Jane Smith
print(myclass.student_name)        # 输出: Jane Smith