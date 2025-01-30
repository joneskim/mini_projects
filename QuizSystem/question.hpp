#ifndef QUESTION_H
#define QUESTION_H

#include <string>
#include <functional>

template<typename T>
class Question {

    public:
        Question(const std::string &question, const T &answer) : question(question), correctAnswer(answer) {
            checkAnswerFunc = [this](const T& answer) { return answer == correctAnswer; };
        };
    
        bool checkAnswer(const T& answer) const{
            return checkAnswerFunc(answer);
        };

        const std::string &getQuestion() const {
            return question;
        }

    private:
        std::string question;
        T correctAnswer;
        std::function<bool(const T&)> checkAnswerFunc;

};

#endif /* QUESTION_H */