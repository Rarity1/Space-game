#include "InputHandler.h"
#include <algorithm>
#include <iterator>

Input::Input():thread(THREADS((int)std::thread::hardware_concurrency())){};
Input::~Input(){};

bool Input::Keyboard::KeyIsPressed(unsigned char keycode) const noexcept
{
	return CurrentKeyState[keycode];
}

const bool Input::Keyboard::CharIsEmpty() noexcept
{
  BufferLock.lock();
  auto result = charbuffer.empty();
  BufferLock.unlock();
	return result;
}

void Input::Keyboard::FlushKey()
{
  BufferLock.lock();
	keybuffer = std::queue<Event>();
  BufferLock.unlock();
}

void Input::Keyboard::FlushChar()
{
   BufferLock.lock();
	charbuffer = std::queue<char>();
   BufferLock.unlock();
}

void Input::Keyboard::EmptyBuffers() noexcept {
  BufferLock.lock();
  keybuffer = std::queue<Event>();
  charbuffer = std::queue<char>();
  BufferLock.unlock();
}

void Input::Keyboard::UpdateKey(Event::Type Direction,
                                unsigned char keycode) noexcept {
  using PType = Input::Event::Type;
  BufferLock.lock();
  switch (Direction) {
  case (PType::Press):
    CurrentKeyState[keycode] = true;
    keybuffer.push(Input::Event(keycode, PType::Press));
    break;
  case (PType::Release):
    CurrentKeyState[keycode] = false;
    keybuffer.push(Input::Event(keycode, PType::Release));
    break;
  case (PType::Character):
    charbuffer.push(keycode);
    break;
  }
  BufferLock.unlock();
}

void Input::Keyboard::ClearState() noexcept
{
  BufferLock.lock();
	CurrentKeyState.reset();
  BufferLock.unlock();
}

Input::dispatchID Input::linkEvent(unsigned char key, std::function<void()> dispatchFunc, Input::Event::Type type){
dispatchID Result;
dispatchMTX.lock();
if(dispatchFunctions.contains(key)){
  dispatchFunctions[key].push_back({dispatchFunc, type});
  Result.ID =  std::prev(dispatchFunctions[key].end());
}else{
dispatchFunctions.insert(std::pair{key, std::list{std::pair{dispatchFunc, type}}});
Result.ID =  std::prev(dispatchFunctions[key].end());
}
dispatchMTX.unlock();
Result.key = key;
Result.Empty.store(false);
return Result;
}

void Input::unlinkEvent(dispatchID key){
  dispatchMTX.lock();
  dispatchFunctions[key.key].erase(key.ID);
  if(dispatchFunctions[key.key].empty()){
    dispatchFunctions.erase(key.key);
  }
  dispatchMTX.unlock();
}

void Input::DispatchInputEvents() {

  std::vector<THREADS::WRef> WorkRefs;
  keyboard.BufferLock.lock();
  while (!keyboard.keybuffer.empty()) {
    
    auto event = keyboard.keybuffer.front();
    keyboard.keybuffer.pop();
    
    auto key = event.code;
    dispatchMTX.lock();
    for (auto &func : dispatchFunctions[key]) {
      WorkRefs.emplace_back(thread.gPushWork([&func, event] {
        if(event.type == func.second){
          func.first();
        }
      }));
    }
    dispatchMTX.unlock();
  }
  keyboard.BufferLock.unlock();

  thread.gEndWork(WorkRefs);
}