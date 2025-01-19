# Checking whether a specific key exists in std::map or std::unordered_map

map.find(x) searches the map for the key x and returns an iterator. A valid iterator points to the key-value pair if the key exists, otherwise the key is not found, it returns map.end()


Therefore, (map.find(x) != map.end()) returns true if key exists (iterator is valid), or false if key does not exist (iterator is invalid)

Does not alter the map


# How iomanip works in CPP

the most commonly used manipulators are
std::setw(n) -> sets the width of the next output field to n characters. Useful for aligning columns in tabular output. Does not persist; must be used before each output.

std::setfill(c) -> fills empty spaces in the output with the specified character `c`. works in conjuction with std::setw

```cpp
std::cout << std::setfill('0') << std::setw(5) << 42 << '\n';
```

other useful ones 
- std::left and std::right -> aligns output to the left or right within the specified width 

- std::fixed and std::scientific -> controlls output format of floating point numbers
    - std::fixedL outputs numbers in fixed-point notation
    - std::scientific: outputs numbers in scientific notation

- std::setprecision(n)

- std::hex, std::dec, std::oct


