# Class Structure
We will define two classes
- Task
- TodoList -> To represent a collection of tasks and methods to add, remove, display, and check tasks

Task will have the following attributes
- name
- description

TodoList will have the following attributes
- tasks (an array of Task objects)
- add_task
- remove_task
- display_tasks
- check_task

Key ideas in this part is that we will be using classes to represent objects and attributes to represent object's properties.

A little bit on access specifiers:
- public: accessible from anywhere
- private: accessible only from within the class
- protected: accessible from within the class and its subclasses

in our design, we will be using public private and protected access specifiers. Public methods such as add_task will be accessible from anywhere while private methods like Task::setName will be only accessible within the class. Private methods such as Task::setName will be only accessible within the class to ensure encapsulation.

> encapsulation is the process of hiding the internal details of an object from the outside world and exposing only the necessary methods to interact with the object

### Constructors and Destructors
Smart pointer and destruction
- when using std::unique_ptr, we do not need to define a destructor to manage Task objects. The unique_ptr will automatically delete the Task object when it goes out of scope. This is the key benefit of RAII in C++ unlike in C where we manually dispose of tasks.

### Passing by reference
- when passing objects by reference, we can pass the object and its methods directly to the function without copying. This is more efficient and faster than passing by value.
- this is done to avoid copying the object, which can be expensive in terms of memory and performance.
Remember, for safety, a const reference does not allow you to modify the original object, while a non-const reference allows you to modify the object.

in our code we pass the name and description by reference to the add_task function and check_task function.