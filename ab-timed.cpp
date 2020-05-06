#include <condition_variable>
#include <chrono>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include "ab-timed.h"
#include "connect15.h"

double simplest_eval(const State& s)
{
    return (rand() % 2) ? 1 : -1;
}

struct WorkQueue {
    std::vector<std::thread> workers_;
    std::deque<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;

    explicit WorkQueue(int n) {
        for (int i=0; i < n; ++i) {
            workers_.emplace_back([this]() {
                while (true) {
                    std::unique_lock<std::mutex> lk(mtx_);
                    while (tasks_.empty()) {
                        cv_.wait(lk);
                    }
                    auto f = std::move(tasks_.front());
                    tasks_.pop_front();
                    lk.unlock();
                    printf("WorkQueue is running a task\n");
                    f();
                }
            });
        }
    }
    void schedule(std::function<void()> f) {
        printf("WorkQueue is adding a task\n");
        std::unique_lock<std::mutex> lk(mtx_);
        tasks_.push_back(std::move(f));
        printf("WorkQueue is done adding a task\n");
        lk.unlock();
        cv_.notify_one();
    }
};

static WorkQueue g_workQueue(1);

using Deadline = std::chrono::steady_clock::time_point;

using Move = int;

using Result = std::pair<double, int>;

struct Task : std::enable_shared_from_this<Task> {
    std::condition_variable cv_;
    std::mutex mtx_;
    Result result_ = { INT_MIN, 0 };

    template<class Callable>
    void spawn_thread(Callable f) {
        g_workQueue.schedule(f);
    }

    template<class Predicate, class Do>
    void when(Predicate p, Do f) {
        spawn_thread([this, p, f, keepalive = shared_from_this()]() {
            std::unique_lock<std::mutex> lk(mtx_);
            while (!p()) {
                this->cv_.wait(lk);
            }
            lk.unlock();
            f();
        });
    }

    template<class Predicate, class Do>
    void tailcall_when(Predicate p, Do f) {
        std::unique_lock<std::mutex> lk(mtx_);
        while (!p()) {
            this->cv_.wait(lk);
        }
        lk.unlock();
        f();
    }

    template<class Do>
    void with_notification(Do f) {
        std::unique_lock<std::mutex> lk(mtx_);
        f();
        lk.unlock();
        cv_.notify_all();
    }

    void got_one_subresult() { do_got_one_subresult(); }
    void evaluate_and_notify() { do_evaluate_and_notify(); }

private:
    virtual void do_got_one_subresult() = 0;
    virtual void do_evaluate_and_notify() = 0;
};

struct ExpectCardTask : Task {
    LeafEvaluationFunction eval_;
    State s_;
    int waiting_for_subresults_ = 0;
    std::shared_ptr<Task> parent_task_ = nullptr;
    std::vector<std::shared_ptr<Task>> subtasks_;
    std::vector<int> weights_;
    Deadline deadline_;

    explicit ExpectCardTask(std::shared_ptr<Task> parent, LeafEvaluationFunction e, State s, Deadline d) :
        eval_(e), s_(s), parent_task_(parent), deadline_(d) {}

private:
    void do_got_one_subresult() override {
        this->waiting_for_subresults_ -= 1;
    }

    void set_and_notify(double v, int m) {
        this->result_ = {v, m};
        parent_task_->with_notification([&]() {
            parent_task_->got_one_subresult();
        });
    }

    void do_evaluate_and_notify() override;

    void combine_subresults() {
        assert(this->waiting_for_subresults_ <= 0);
        printf("ExpectCardTask : Combining %d subresults...\n", (int)subtasks_.size());
        Result r = { INT_MIN, 0 };
        double sum = 0;
        int count = 0;
        assert(subtasks_.size() == weights_.size());
        for (int i=0; i < subtasks_.size(); ++i) {
            sum += subtasks_[i]->result_.first;
            count += weights_[i];
        }
        set_and_notify(-sum / count, 0);
    }
};

struct PickMoveTask : Task {
    LeafEvaluationFunction eval_;
    State s_;
    int waiting_for_subresults_ = 0;
    std::shared_ptr<Task> parent_task_ = nullptr;
    std::promise<Result> root_promise_;
    std::vector<std::shared_ptr<Task>> subtasks_;
    Deadline deadline_;

    explicit PickMoveTask(std::shared_ptr<Task> parent, LeafEvaluationFunction e, State s, Deadline d) :
        eval_(e), s_(s), parent_task_(parent), deadline_(d) {}

    explicit PickMoveTask(std::promise<Result> parent, LeafEvaluationFunction e, State s, Deadline d) :
        eval_(e), s_(s), root_promise_(std::move(parent)), deadline_(d) {}

private:
    void do_got_one_subresult() override {
        this->waiting_for_subresults_ -= 1;
    }

    void set_and_notify(double v, int m) {
        this->result_ = {v, m};
        if (parent_task_) {
            parent_task_->with_notification([&]() {
                parent_task_->got_one_subresult();
            });
        } else {
            root_promise_.set_value({v, m});
        }
    }

    void do_evaluate_and_notify() override {
        if (s_.is_tie_game()) {
            set_and_notify(eval_(s_), 0);
            return;
        }
        if (std::chrono::steady_clock::now() >= deadline_) {
            set_and_notify(eval_(s_), 0);
            return;
        }
        int who = s_.active_player();
        int columns = s_.count_columns();
        if (columns <= 1) columns = 0;
        for (int m = -1; m <= columns; ++m) {
            State next = s_;
            if (next.apply_in_place_without_drawing(m)) {
                set_and_notify(INT_MAX, m);
                return;
            }
            subtasks_.push_back(std::make_shared<ExpectCardTask>(shared_from_this(), eval_, next, deadline_));
        }
        waiting_for_subresults_ = subtasks_.size();
        for (auto&& t : subtasks_) {
            spawn_thread([t] { t->evaluate_and_notify(); });
        }
        tailcall_when([self = shared_from_this()]() {
            return (self->waiting_for_subresults_ <= 0);
        }, [self = shared_from_this()]() {
            self->combine_subresults();
        });
    }

    void combine_subresults() {
        assert(this->waiting_for_subresults_ <= 0);
        printf("PickMoveTask : Combining %d subresults...\n", (int)subtasks_.size());
        Result r = { INT_MIN, 0 };
        for (auto&& sub : subtasks_) {
            r = std::max(r, sub->result_);
        }
        set_and_notify(r.first, r.second);
    }
};

void ExpectCardTask::do_evaluate_and_notify()
{
    if (std::chrono::steady_clock::now() >= deadline_) {
        set_and_notify(eval_(s_), 0);
        return;
    }
    Color who = s_.active_player();
    State next = s_;
    for (int v = 1; v <= 7; ++v) {
        int weight = s_.count_unseen_cards(who, v);
        assert(0 <= weight && weight <= 2);
        if (weight != 0) {
            next.draw_this_card(who, v);
            subtasks_.push_back(std::make_shared<PickMoveTask>(shared_from_this(), eval_, next, deadline_));
            weights_.push_back(weight);
        }
    }
    waiting_for_subresults_ = subtasks_.size();
    for (auto&& t : subtasks_) {
        spawn_thread([t] { t->evaluate_and_notify(); });
    }
    tailcall_when([this, keepalive = shared_from_this()]() {
        return (this->waiting_for_subresults_ <= 0);
    }, [this, keepalive = shared_from_this()]() {
        this->combine_subresults();
    });
}


Result recursively_evaluate(LeafEvaluationFunction eval, const State& s, std::chrono::milliseconds timeout)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    std::promise<Result> root;
    std::future<Result> result = root.get_future();
    auto head = std::make_shared<PickMoveTask>(std::move(root), eval, s, deadline);
    puts("cpA");
    head->evaluate_and_notify();
    puts("cpB");
    return result.get();
}