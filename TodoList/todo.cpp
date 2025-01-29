#include <iostream>
#include <vector>
#include <memory>
#include <string>

class Task {
    public:
        Task(const std::string &name, const std::string &description)
            : name(name), description(description), completed(false) {}

        std::string getName() const { return name; }
        std::string getDescription() const { return description; }
        bool isCompleted() const { return completed; }

        void setName(const std::string &name) { this->name = name; }
        void setDescription(const std::string &description) { this->description = description; }
        void setCompleted(bool completed) { this->completed = completed; }

    private:
        std::string name;
        std::string description;
        bool completed;
};

class TodoList {
    public:
        TodoList() = default;
        void addTask(const std::string &name, const std::string &description);
        void removeTask(const std::string &name);
        void markTaskCompleted(const std::string &name);
        void displayTasks() const;

    private:
        std::vector<std::unique_ptr<Task>> tasks;
};

void TodoList::addTask(const std::string &name, const std::string &description) {
    tasks.push_back(std::make_unique<Task>(name, description));
    std::cout << "Task added: " << name << std::endl;
}

void TodoList::removeTask(const std::string &name) {
    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
        if ((*it)->getName() == name) {
            tasks.erase(it);
            std::cout << "Task removed: " << name << std::endl;
            return;
        }
    }
    std::cout << "Task not found: " << name << std::endl;
}

void TodoList::markTaskCompleted(const std::string &name) {
    for (auto &task : tasks) {
        if (task->getName() == name) {
            task->setCompleted(true);
            std::cout << "Task completed: " << name << std::endl;
            return;
        }
    }
    std::cout << "Task not found: " << name << std::endl;
}

void TodoList::displayTasks() const {
    std::cout << "Tasks:" << std::endl;
    for (const auto &task : tasks) {
        std::cout << "- " << task->getName() << (task->isCompleted() ? " (Completed)" : "") << std::endl;
    }
}

int main() {
    // Testing the TodoList class
    TodoList todoList;
    todoList.addTask("Buy groceries", "Milk, eggs, bread");
    todoList.addTask("Clean the house", "Vacuum, mop, dust");
    todoList.addTask("Do laundry", "Fold clothes, iron");

    todoList.displayTasks();

    todoList.markTaskCompleted("Buy groceries");
    todoList.displayTasks();

    todoList.removeTask("Do laundry");
    todoList.displayTasks();

    return 0;
}
