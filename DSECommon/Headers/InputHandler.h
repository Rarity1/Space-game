
#pragma once
#include "Threads.h"
#include <bitset>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <unordered_map>

#ifndef DLL
#ifdef DESCDLL
#define DLL __declspec( dllexport )
#else
#define DLL __declspec( dllimport )
#endif
#endif

class DLL Input {
  THREADS thread;
  
  static constexpr unsigned int nKeys = 256u;
  std::mutex dispatchMTX;
public:
  Input();
  ~Input();
  class Event {
  public:
    enum class Type { Press, Release, Character };
    Event( unsigned char code, Type type) noexcept : code(code), type(type) {}
    const unsigned char code;
    const Type type;
  };
  private:
  std::unordered_map<unsigned char, std::list<std::pair<std::function<void()>, Event::Type>>> dispatchFunctions;

  public:
  struct dispatchID{
    std::_List_iterator<std::pair<std::function<void()>, Event::Type>> ID;
    unsigned char key;
    Event::Type type;
    dispatchID(){
      Empty.store(true);
    };
    dispatchID(const dispatchID& old){
      ID = old.ID;
      key = old.key;
      type = old.type;
      Empty.store(old.Empty.load());
    }
    dispatchID& operator=(const dispatchID& old){
      ID = old.ID;
      key = old.key;
      type = old.type;
      Empty.store(old.Empty.load());
      return *this;
    };
    std::atomic<bool> Empty;
  };
  class Mouse {};
  class Keyboard {
    friend class Input;
    static constexpr unsigned int nKeys = 256u;
    std::bitset<nKeys> CurrentKeyState;
    std::queue<Event> keybuffer;
    std::queue<char> charbuffer;
    std::mutex BufferLock;
  public:
    Keyboard() = default;
    Keyboard(const Keyboard &) = delete;
    Keyboard &operator=(const Keyboard &) = delete;
    // key event stuff
    bool KeyIsPressed(unsigned char keycode) const noexcept;
    void FlushKey();
    void FlushChar();
    // char event stuff
    const bool CharIsEmpty() noexcept;
    void EmptyBuffers() noexcept;
    // autorepeat control
    void DLL UpdateKey(Event::Type Direction, unsigned char keycode) noexcept;
    void DLL ClearState() noexcept;
  };
  Keyboard keyboard;
  Mouse mouse;
  dispatchID linkEvent(unsigned char key, std::function<void()> dispatchFunc, Input::Event::Type type = Input::Event::Type::Release);
  void unlinkEvent(dispatchID key);
  
 void DispatchInputEvents();
};
