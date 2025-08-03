#include <iostream>
#include <coroutine>
#include <list>
#include <random>
#include <vector>
#include <string>

class Board {
public:
    class Awaitor {
    public:
        Awaitor(Board& board) : board_(board) {}
        bool await_ready() const { return false; }
        void await_suspend(std::coroutine_handle<> handle) {
            handle_ = handle;
            board_.pushAwaitor(this);
        }
        int await_resume() const { return result_; }
        void notify(int result) {
            result_ = result;
            handle_.resume();
        }
    private:
        Board& board_;
        std::coroutine_handle<> handle_;
        int result_{};
    };

    Awaitor roll() {
        return Awaitor{*this};
    }

    void run() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 6);

        while (!awaitors_.empty()) {
            std::vector<Awaitor*> round;
            round.reserve(awaitors_.size());
            while (!awaitors_.empty()) {
                round.push_back(popAwaitor());
            }
            for (auto* awaitor : round) {
                int dice_roll = dist(gen);
                awaitor->notify(dice_roll);
            }
        }
    }

    void pushAwaitor(Awaitor* awaitor) {
        awaitors_.push_back(awaitor);
    }
    Awaitor* popAwaitor() {
        auto* awaitor = awaitors_.front();
        awaitors_.pop_front();
        return awaitor;
    }
private:
    std::list<Awaitor*> awaitors_;
};

class PlayerTask {
public:
    struct promise_type {
        PlayerTask get_return_object() { return {}; }
        std::suspend_never initial_suspend() const { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
    };
};

PlayerTask player_proc(const std::string& name, Board& board) {
    std::cout << "[ " << name << " ] starts." << std::endl;
    int distance = 10;
    while (distance > 0) {
        int roll = co_await board.roll();
        distance -= roll;
        if (distance < 0) distance = 0;
        std::cout << "[ " << name << " ] got: " << roll
                  << ", now distance = " << distance << std::endl;
    }
    std::cout << "[ " << name << " ] finishes." << std::endl;
}

int main() {
    Board board;
    player_proc("\033[31mRed\033[0m", board);
    player_proc("\033[32mGreen\033[0m", board);
    player_proc("\033[34mBlue\033[0m", board);
    player_proc("\033[33mYellow\033[0m", board);

    board.run();
    return 0;
}
