#ifndef QUESTION_H
#define QUESTION_H

#include <string>
#include <functional>

template <typename T>
concept Comparable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
};

// 2. Question class template
template <Comparable T>
class Question {
public:
    Question(const std::string& text, T correctAnswer)
        : text_(text), correctAnswer_(correctAnswer) {
        evaluator_ = [this](const T& userAnswer) {
            return userAnswer == correctAnswer_;
        };
    }

    bool checkAnswer(const T& userAnswer) const { return evaluator_(userAnswer); }
    const std::string& getText() const { return text_; }

private:
    std::string text_;
    T correctAnswer_;
    std::function<bool(const T&)> evaluator_;
};

#endif /* QUESTION_H */