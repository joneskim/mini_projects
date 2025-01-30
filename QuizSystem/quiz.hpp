#ifndef QUIZ_H
#define QUIZ_H

#include <vector>
#include <iostream>
#include "question.h"

class Quiz {
    public:
        template <typename T>
        void addQuestion(const std::string &question, const T &answer);

        void runQuiz();

    private:
        // we will store questions in a vector
        std::vector<std::function<bool()>> questionHandlers;
}

#endif /* QUIZ_H */