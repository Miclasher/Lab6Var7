#include <iostream>
#include <coroutine>
#include <random>

struct Controller;

struct Task {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type h;

    Task(handle_type h) : h(h) {}
    ~Task() { if (h) h.destroy(); }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& other) noexcept : h(other.h) { other.h = nullptr; }

    struct promise_type {
        std::coroutine_handle<> continuation = std::noop_coroutine();

        Task get_return_object() {
            return Task{ handle_type::from_promise(*this) };
        }

        std::suspend_always initial_suspend() { return {}; }

        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(handle_type h) noexcept {
                return h.promise().continuation;
            }
            void await_resume() noexcept {}
        };

        FinalAwaiter final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }

        auto await_transform(int n);
    };
};

Task CoroutineA(int n);

struct Controller {
    int value;

    bool await_ready() { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        if (value % 2 == 0) {
            Task t = CoroutineA(value);
            t.h.promise().continuation = caller;
            std::coroutine_handle<> next = t.h;

            t.h = nullptr;
            return next;
        }
        else {
            return caller;
        }
    }

    void await_resume() {}
};

auto Task::promise_type::await_transform(int n) {
    return Controller{ n };
}

Task CoroutineA(int n) {
    std::cout << "Event (Even): " << n << std::endl;
    co_return;
}

Task Generator() {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dist(1, 256);

    for (int i = 0; i < 10; ++i) {
        int n = dist(gen);
        co_await n;
    }
}

int main() {
    auto gen = Generator();
    gen.h.resume();
    return 0;
}