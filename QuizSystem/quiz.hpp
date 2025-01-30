#ifndef QUIZ_H
#define QUIZ_H

#include <vector>
#include <iostream>
#include "question.hpp"

// 3. Quiz class
class Quiz {
public:
    // Add string-based question
    void addStringQuestion(const std::string& text, const std::string& correctAnswer) {
        questions_.emplace_back([=]() {
            std::cout << text << " ";
            std::string userAnswer;
            std::getline(std::cin, userAnswer);
            Question<std::string> q(text, correctAnswer);
            return q.checkAnswer(userAnswer);
        });
    }

    // Add integer-based question
    void addIntQuestion(const std::string& text, int correctAnswer) {
        questions_.emplace_back([=]() {
            std::cout << text << " ";
            int userAnswer;
            std::cin >> userAnswer;
            // Clear leftover newline
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            Question<int> q(text, correctAnswer);
            return q.checkAnswer(userAnswer);
        });
    }

    void start() {
        int score = 0;
        for (auto& fn : questions_) {
            if (fn()) {
                std::cout << "Correct!\n";
                ++score;
            } else {
                std::cout << "Incorrect.\n";
            }
        }
        std::cout << "Quiz finished! Your score: " << score
                  << "/" << questions_.size() << "\n";
    }

private:
    // Vector of callables that ask a question and return true if correct
    std::vector<std::function<bool()>> questions_;
};


#endif /* QUIZ_H */