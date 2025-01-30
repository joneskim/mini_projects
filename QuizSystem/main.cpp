#include <iostream>
#include "question.hpp"
#include "quiz.hpp"

int main() {
    Quiz quiz;
    quiz.addIntQuestion("What is 2 + 2?", 4);
    quiz.addStringQuestion("What is the capital of France?", "Paris");
    quiz.addIntQuestion("What is 3 * 5?", 15);
    quiz.start();
    return 0;
}


