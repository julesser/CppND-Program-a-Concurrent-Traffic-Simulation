#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <future>

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // SOLVED FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> uLock(this->_mutex);
    _cond.wait(uLock, [this]
               { return !this->_queue.empty(); });

    T msg = std::move(this->_queue.front());
    _queue.pop_front();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // SOLVED FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    std::unique_lock<std::mutex> uLock(this->_mutex);
    this->_queue.emplace_back(std::move(msg));
    this->_cond.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    this->_MessageQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // SOLVED FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        if (this->_MessageQueue->receive() == TrafficLightPhase::green)
        {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // SOLVED FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // SOLVED FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    // init random duration
    static std::random_device dvc;
    static std::mt19937 eng(dvc());
    static std::uniform_int_distribution<> distr(4, 6);

    std::unique_lock<std::mutex> uLock(this->_mutex);
    std::cout << "TrafficLight #" << _id << "::cycleThroughPhase: thread id = " << std::this_thread::get_id() << std::endl;
    uLock.unlock();

    // init variables
    double cycleDuration = distr(eng); // duration of a single simulation cycle in second, is radomly chosen
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;

    // init stop watch
    lastUpdate = std::chrono::system_clock::now();
    while (true)
    {
        // compute time difference to stop watch
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastUpdate).count();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (timeSinceLastUpdate >= cycleDuration)
        {
            if (this->_currentPhase == TrafficLightPhase::red)
            {
                this->_currentPhase = TrafficLightPhase::green;
            }
            else
            {
                this->_currentPhase = TrafficLightPhase::red;
            }

            auto addTrafficLightPhase = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _MessageQueue, std::move(this->_currentPhase));
            addTrafficLightPhase.get();

            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();

            cycleDuration = distr(eng);
        }
    }
}