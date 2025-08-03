#include <iostream>
#include <future>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

class Board {
public:
    int roll() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 6);
        return dist(gen);
    }
};

struct Sync {
    std::mutex mtx;
    std::condition_variable cv;
    int total;
    int round = 0;
    std::atomic<bool> finished{false};
};

void player_proc(const std::string& name, Board& board, Sync& sync, std::vector<int>& distances, int idx) {
    while (!sync.finished.load()) {
        std::unique_lock<std::mutex> lock(sync.mtx);
        sync.cv.wait(lock, [&sync, idx]() { return sync.round == idx || sync.finished.load(); });

        if (sync.finished.load()) break;

        int roll = board.roll();
        distances[idx] -= roll;
        std::cout << "[ " << name << " ] got: " << roll
                      << ", now distance = " << (distances[idx] > 0 ? distances[idx] : 0) << std::endl;
        if (distances[idx] <= 0) {
            std::cout << "[ " << name << " ] finishes." << std::endl;
            sync.finished.store(true);
            sync.cv.notify_all();
            break;
        } 

        sync.round = (sync.round + 1) % sync.total;
        sync.cv.notify_all();
    }
}

int main() {
    Board board;
    const int player_count = 4;
    std::vector<std::string> names = {
        "\033[31mRed\033[0m",
        "\033[32mGreen\033[0m",
        "\033[34mBlue\033[0m",
        "\033[33mYellow\033[0m"
    };
    std::vector<int> distances(player_count, 10);
    Sync sync;
    sync.total = player_count;
    sync.round = 0;

    std::vector<std::future<void>> players;
    for (int i = 0; i < player_count; ++i) {
        players.push_back(std::async(std::launch::async, player_proc, names[i], std::ref(board), std::ref(sync), std::ref(distances), i));
    }

    for (auto& fut : players) {
        fut.get();
    }
    return 0;
}
