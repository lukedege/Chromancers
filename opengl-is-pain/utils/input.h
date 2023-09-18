#pragma once

#include <functional>
#include <vector>
#include <unordered_set>

namespace utils::graphics::opengl
{
    struct KeyCallbacks
    {
        std::vector<std::function<void()>> onPressed_callbacks;
        std::vector<std::function<void()>> onRelease_callbacks;
    };

    // Simplistic simpleton class to abstract and manage input from glfw
    class Input
    {
    private:
        KeyCallbacks keys[1024];
        std::unordered_set<int> pressed_keys;

        Input() {}
    public:
        
        Input(Input& other) = delete;
        void operator=(const Input&) = delete;

        static Input& instance()
        {
            static Input instance;
            return instance;
        }

        void add_onPressed_callback(int key, std::function<void()> callback)
        {
            keys[key].onPressed_callbacks.push_back(callback);
        }

        void add_onRelease_callback(int key, std::function<void()> callback)
        {
            keys[key].onRelease_callbacks.push_back(callback);
        }

        void press_key(int key)
        {
            pressed_keys.insert(key);
        }

        void release_key(int key)
        {
            for (auto& callback : keys[key].onRelease_callbacks)
            {
                callback();
            }
            pressed_keys.erase(key);
        }

        void process_pressed_keys()
        {
            for (auto& key : pressed_keys)
            {
                for (auto& callback : keys[key].onPressed_callbacks)
                {
                    callback();
                }
            }
        }
    };
};