#include <condition_variable>
#include <mutex>
#include <deque>
#include <string>
#include <sstream>
  
// Thread-safe queue
class TSQueue {
private:
    // Underlying queue
    std::deque<std::string> m_queue;
  
    // mutex for thread synchronization
    std::mutex m_mutex;
  
    // Condition variable for signaling
    std::condition_variable m_cond;
  
public:
    // Pushes an element to the queue
    void push(std::string item)
    {
  
        // Acquire lock
        std::unique_lock<std::mutex> lock(m_mutex);
  
        // Add item
        m_queue.push_back(item);
  
        // Notify one thread that
        // is waiting
        m_cond.notify_one();
    }
  
    // Pops an element off the queue
    std::string pop()
    {
  
        // acquire lock
        std::unique_lock<std::mutex> lock(m_mutex);
  
        // wait until queue is not empty
        m_cond.wait(lock,
                    [this]() { return !m_queue.empty(); });
  
        // retrieve item
        std::string item = m_queue.front();
        m_queue.pop_front();
  
        // return item
        return item;
    }

    std::string str() 
    {   
        if (m_queue.empty())
            return "QUEUE EMPTY\n\n";

        std::ostringstream ss;
        ss << "Queue Contents:\n";
        for (auto s : m_queue) {
            ss << s << std::endl;
        }
        ss << std::endl;
        return ss.str();
    }

    bool empty()
    {
        return m_queue.empty();
    }
};