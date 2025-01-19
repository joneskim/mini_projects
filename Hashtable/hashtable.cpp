#include <iostream>
#include <vector>
#include <string>
#include <utility>

class HashTable {
    private:
        std::vector<std::vector<std::pair<std::string, std::string> > > table;
        int size;

        int hashFunction(std::string key){
            int hash = 0;
            for (int i = 0; i < key.length(); i++){
                hash += key[i];
            }
            return hash % size;
        }

    public:
        HashTable(int size){
            this->size = size;
            table.resize(size);
        }

        void insert(std::string key, std::string value){
            int index = hashFunction(key);
            table[index].push_back(std::make_pair(key, value));
            display();

        }

        std::string search(std::string key){
            int index = hashFunction(key);
            for (int i = 0; i < table[index].size(); i++){
                if (table[index][i].first == key){
                    return table[index][i].second;
                }
            }
            return "Not found";
        }

        void remove(std::string key){
            int index = hashFunction(key);
            for (int i = 0; i < table[index].size(); i++){
                if (table[index][i].first == key){
                    table[index].erase(table[index].begin() + i);
                    return;
                }
            }
        }

        void display(){
            for (int i = 0; i < size; i++){
                std::cout << i;
                for (int j = 0; j < table[i].size(); j++){
                    std::cout << " --> " << table[i][j].first << " : " << table[i][j].second;
                }
                std::cout << std::endl;
            }
        }
};



int main(){

    HashTable ht(10);

    ht.insert("John", "Doe");
    ht.insert("Jane", "Doe");


    ht.display();

    std::cout << "Search Alice: " << ht.search("Alice") << std::endl;
    std::cout << "Search Bob: " << ht.search("Bob") << std::endl;
    std::cout << "Search Eve: " << ht.search("Eve") << std::endl;
    std::cout << "Search John: " << ht.search("John") << std::endl;
    std::cout << "Search Jane: " << ht.search("Jane") << std::endl;
    std::cout << "Search Charlie: " << ht.search("Charlie") << std::endl;
    std::cout << "Search Dave: " << ht.search("Dave") << std::endl;

    ht.remove("John");
    ht.remove("Jane");
    ht.remove("Eve");

    ht.display();

    return 0;



}