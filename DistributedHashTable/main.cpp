#include <iostream>
#include <memory>
#include <vector>

// number of bits used in the identifier
const int N = 5;

class Node : public std::enable_shared_from_this<Node> {
    public:
        using NodePtr = std::shared_ptr<Node>; // alias for shared_ptr
        

        static NodePtr create(unsigned int identifier) {
            return NodePtr(new Node(identifier));
        }
    
        unsigned int getId() const { return identifier; }

        void printInfo() const {
            std::cout << "Node: " << identifier << std::endl;
            if (successor){
                std::cout << " | successor: " << successor->getId();
            }
            
        }
    
    void init() {
        successor = shared_from_this();
    }
    
    void join(NodePtr existingNode){
        if (existingNode){
            successor = existingNode->findSuccessor(identifier);
        } else {
            init();
        }
    }
    
    NodePtr findSuccessor(unsigned int key){
        if (inInterval(key, identifier, successor->getId()) || identifier == successor->getId()){
            return successor;
        } else {
            return successor->findSuccessor(key);
        }
    }
    
    void stabilize() {
        NodePtr x = successor->predecessor;
        if (x && inInterval(x->getId(),identifier, successor->getId())){
            successor = x;
        }
        
        successor->notify(shared_from_this());
    }
    
    void notify(NodePtr n){
        if (!predecessor || inInterval(n->getId(), predecessor->getId(), identifier)){
            predecessor = n;
        }
    }

    private:
        unsigned int identifier;
        NodePtr successor; // next node in the ring
        NodePtr predecessor;
        Node(unsigned int id) : identifier(id) {}
        
    bool inInterval(unsigned int key, unsigned int start, unsigned int end){
        if (start < end){
            return key > start && key <= end;
        } else {
            return key > start || key <= end;
        }
    }

};

int main() {
    auto node1 = Node::create(7);
    node1->join(nullptr);
    
    auto node2 = Node::create(15);
    node2->join(node1);
    
    auto node3 = Node::create(22);
    node3->join(node1);
    
    node1->stabilize();
    node2->stabilize();
    node3->stabilize();
    
    
    node1->printInfo();
    node2->printInfo();
    node3->printInfo();
    
    unsigned int key = 10;
    auto responsibleNode = node1->findSuccessor(key);
    std::cout <<"\n" << key << " is managed by node " << responsibleNode->getId() << std::endl;
    
    return 0;
    
}
