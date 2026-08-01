#pragma once
#include "CWin.h"
#include "RStorage.h"
#include "Threads.h"
#include <DirectXMath.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <unordered_map>

class Object;
class CameraObject;
class RenderedObject;
class PhysicsObject;
struct CBVData {
  alignas(16) DirectX::XMFLOAT4X4 cbvMatrix;
  UINT Texture = 0;
  // do not use
  // UINT Padding[3];
};
// Unique Object ID
typedef uint64_t UOID;
typedef uint16_t InstID;
class Tracker {
  

public:
  class Instance {
    friend class Tracker;
    InstID instanceID = 0;
    // all Objects loaded in this instance
    std::list<Object *> InstanceObjects;
    // Self explanatory
    Object *activeCamera;
    std::unordered_map<UOID, Object *> Cameras;

    // umID to array of CBV data
    std::unordered_map<umID, CBVData *> instancedCBVData;

    // if an object is linked to a model it is at least a rendered object
    std::unordered_map<umID, std::list<Object *>> ModelLinkedObjects;

    // Physics Objects
    std::list<Object *> PhysicsObjects;
    std::unordered_map<UOID, bool> physEnabled;
    // parent tracker class

    UINT Count;
    std::atomic<bool> Active;
    std::mutex InstanceObjLock;
    std::unique_ptr<Object> uniqueOrigin;
  public:
    void AddObject(Object *obj);
    void LinkToModel(RenderedObject *obj, umID umID);
    void EnablePhysics(PhysicsObject *obj, UOID uOID);
    void MakeCamera(CameraObject *obj, UOID uOID);
    CameraObject *ActiveCamera(CameraObject *obj = nullptr);

    bool operator==(Instance &comp) { return comp.instanceID == instanceID; }

    Instance(InstID instanceID, Tracker &Parent, std::function<void()> func);
    Tracker &pTracker;
    const std::unordered_map<UINT, std::list<Object *>> &GetRenderObjects() {
      return ModelLinkedObjects;
    }
    const std::list<Object *> &GetPhysicsObjects() { return PhysicsObjects; }
    CBVData *&GetCBVPtr(umID ID) { return instancedCBVData[ID]; }
    CBVData **GetCBVPtrtoPtr(umID ID) {
      return std::addressof(instancedCBVData[ID]);
    }
    const InstID &GetID() { return instanceID; }
    //The Origin object
    Object* InstanceObject = nullptr;
  };
  friend class Tracker::Instance;

  Tracker(RStorage &rs) :  tTracker(std::make_unique<THREADS>((int)std::thread::hardware_concurrency())), storage(rs) {};
  ~Tracker() {};
  void lModel(RenderedObject *obj, umID umID) noexcept;
  // Returns ID of current model given UOID(Unique Object ID)
  RStorage::bmResource &GetModel(umID umID);
  // Creates an instance and returns it regardless of if the id given is used
  Instance &initInstance(InstID instanceID = 0,  std::function<void()> func = []{});
  // Returns new instance if instance isnt found
  Instance &getInstance(InstID instanceID);
  Object *initObject(
      Instance &oInstance, std::string name,
      std::function<void()> func = [] {});

  CameraObject *initCameraObject(
    Tracker::Instance& pInstance,
      std::string name, std::function<void()> func = [] {},
      DirectX::XMFLOAT3 initPos = {0, 0, 0},
      DirectX::XMFLOAT4 initRot = {1, 0, 0, 0},
      DirectX::XMFLOAT4 initUpDirection = {0, 0, 1, 0},
      RenderedObject *link = nullptr, bool Active = false);

  RenderedObject *initRenderObject(
      Instance &Instance, std::string name, std::function<void()> func = [] {},
      umID filebModelIndex = 0, float mScale = 1.0,
      DirectX::XMFLOAT3 initPos = {0, 0, 0},
      DirectX::XMFLOAT4 initRot = {0, 0, 0, 1});

  PhysicsObject *initPhysObject(
      Instance &Instance, std::string name, std::function<void()> func = [] {},
      umID filebModelIndex = 0, float mScale = 1.0,
      DirectX::XMFLOAT3 initPos = {0, 0, 0},
      DirectX::XMFLOAT4 initRot = {0, 0, 0, 1}, float mMass = 0.0,
      float mFriction = 0.01, DirectX::XMFLOAT3 initVelDir = {0, 0, 0},
      float initSpeed = 0);

  void unloadObject(UOID obj);
  bool IsRendered(Object &obj);
  bool IsPhysics(Object &obj);
  void MakeInstanceActive(Instance &inst);

  const std::list<Tracker::Instance *> &GetActiveInstances() {
    return ActiveInstances;
  };

private:
  std::unique_ptr<THREADS> tTracker;
  class IDAllocator {
    uint64_t cIDCount = 0;
    std::vector<UOID> FreeIDs;
    std::unordered_map<UOID, bool> UsedIDs;
    std::mutex IDMutex;

  public:
    IDAllocator(UOID IDCount = 1200) : cIDCount(IDCount) {
      IDMutex.lock();
      cIDCount = cIDCount == 0 ? 1 : cIDCount;
      FreeIDs.reserve(cIDCount);
      FreeIDs.resize(cIDCount);
      std::iota(FreeIDs.begin(), FreeIDs.end(), 0);
      std::reverse(FreeIDs.begin(), FreeIDs.end());
      IDMutex.unlock();
    }
    ~IDAllocator() = default;
    UOID AllocID() {
      IDMutex.lock();
      UOID Result = FreeIDs.back();
      FreeIDs.pop_back();
      if (FreeIDs.size() <= cIDCount * 0.5) {
        std::vector<UOID> extendIDs(cIDCount);
        std::iota(extendIDs.begin(), extendIDs.end(), cIDCount);
        std::reverse(extendIDs.begin(), extendIDs.end());
        extendIDs.append_range(FreeIDs);
        cIDCount = (cIDCount + cIDCount);
        FreeIDs = std::move(extendIDs);
      }
      UsedIDs[Result] = true;
      IDMutex.unlock();
      return Result;
    }
    void FreeID(UOID &ID) {
      IDMutex.lock();
      if (UsedIDs.find(ID) != UsedIDs.end()) {
        UsedIDs.erase(ID);
        FreeIDs.push_back(ID);
      }
      IDMutex.unlock();
    }
  };
  RStorage &storage;
  IDAllocator idTracker;
  std::unordered_map<InstID, std::unique_ptr<Instance>> Instances;
  // Tracker Owns all objects no object will unload until removed from this map
  std::unordered_map<UOID, std::unique_ptr<Object>> TrackedObjects;
  std::list<Tracker::Instance *> ActiveInstances;
  std::unordered_map<InstID, std::atomic<bool> *> isInstanceActive;
  Object* initOriginObject(Instance* oInstance,
                            std::function<void()> func);
};
class relposVect {
protected:
  std::unique_ptr<DirectX::XMFLOAT3> position;
  DirectX::XMFLOAT3 lastposition = {0, 0, 0};
  DirectX::XMFLOAT4 rotation{0, 0, 0, 1};
  std::mutex posMtx;

public:
  relposVect() : position(std::make_unique<DirectX::XMFLOAT3>(0, 0, 0)) {};
  relposVect(DirectX::XMFLOAT3 initPos)
      : position(std::make_unique<DirectX::XMFLOAT3>(initPos)) {};
  relposVect(DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT4 initRot)
      : position(std::make_unique<DirectX::XMFLOAT3>(initPos)),
        rotation(initRot) {};
  ~relposVect() = default;
  relposVect(relposVect &&old) noexcept {
    position = std::move(old.position);
    lastposition = std::move(old.lastposition);
    rotation = std::move(old.rotation);
  };
  relposVect(const relposVect &old) {
    position = std::make_unique<DirectX::XMFLOAT3>(*old.position);
    lastposition = old.lastposition;
    rotation = old.rotation;
  };
  relposVect &operator=(const relposVect &old) {
    *position = *old.position;
    lastposition = old.lastposition;
    rotation = old.rotation;
    return *this;
  }
  const DirectX::XMFLOAT3 Move(DirectX::XMFLOAT3 &NewPos);
  const DirectX::XMFLOAT3 Get();
  const DirectX::XMFLOAT3 GetLast();
  const DirectX::XMFLOAT4 GetRotation();
  void SetRotation(DirectX::XMFLOAT4& Rotation);
  const DirectX::XMFLOAT3 *GetPtr() { return position.get(); };
};

class CameraPosition : public relposVect{

  DirectX::XMFLOAT4 upDirection = {0, 0, 1, 0};
  public:
  CameraPosition(DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT4 initRot, DirectX::XMFLOAT4 UpDirection):
  relposVect(initPos, initRot),
  upDirection(UpDirection)
  {};
  const DirectX::XMFLOAT4 GetUpDirection();
  void SetUpDirection(DirectX::XMFLOAT4 UpDirection);
};

class Object {

  friend class Tracker;
  friend class Engine;

protected:
  std::string name = "";
  UOID uOID;
  std::function<void()> Script;
  //Check if all parents are not this object. Recursively
  Object *Parent = nullptr;
  std::unordered_map<UOID, Object *> Children;
  Tracker::Instance *LinkedInstance;
  bool isOrigin = false;
public:
//Origin constructor dont use
  Object(UOID uOID, std::function<void()> func = [] {},
      Tracker::Instance *pInstance = nullptr);
//Use this one
  Object(
      std::string name, UOID uOID, std::function<void()> func = [] {},
      Tracker::Instance *pInstance = nullptr);
  virtual ~Object() = default;
  bool operator==(const Object &comparison) {
    if (uOID != comparison.uOID)
      return false;
    return true;
  }
  bool operator!=(const Object &comparison) {
    if (uOID == comparison.uOID)
      return false;
    return true;
  }
  //Returns previous parent. Nullptr if no parent
  Object* AddParent(Object* obj){
    Object* oldParent = nullptr;
    Parent = obj;
    if(oldParent){
      oldParent->Children.erase(uOID);
    }
    Parent = obj;
    Parent->Children.insert({uOID, this});
    return oldParent;
  }
  Object* GetParent();
  Object* RemoveParent(){
   auto oldParent = Parent;
   Parent = nullptr;
   return oldParent;
  };

  // Update important data and run linked scripts
  // Maybe cascade to Child objects?
  virtual void Update() { Script(); };
  // Make sure current object is not a child of itself!
  void UpdateChildren() {
    Update();
    for (auto &child : Children) {
      child.second->UpdateChildren();
    }
  }
  UOID GetID() { return uOID; }
};

class CameraObject : public Object {
  friend class Graphics;
  std::atomic<bool> isActive;
  std::atomic<bool> freeCam;
  
  // DirectX::XMFLOAT4 forwardDirect = {1, 0, 0, 0};
  DirectX::XMFLOAT4X4 cMatrix;
  RenderedObject *linkedObject = nullptr;

public:
  CameraPosition cPos;
  CameraObject(Object &obj, DirectX::XMFLOAT3 initPos = {0, 0, 0},
               DirectX::XMFLOAT4 initRot = {1, 0, 0, 0},
               DirectX::XMFLOAT4 initUpDirection = {0, 0, 1, 0},
               RenderedObject *link = nullptr, bool Active = false);
  CameraObject(
      std::string name, UOID uOID, std::function<void()> func = [] {},
      Tracker::Instance *pInstance = nullptr,
      DirectX::XMFLOAT3 initPos = {0, 0, 0},
      DirectX::XMFLOAT4 initRot = {1, 0, 0, 0},
      DirectX::XMFLOAT4 initUpDirection = {0, 0, 1, 0},
      RenderedObject *link = nullptr, bool Active = false);
  // Returns previous active camera. Returns nullptr if first camera set
  CameraObject *MakeActive() {
    auto old = LinkedInstance->ActiveCamera(this);
    return old;
  };
  void LinkTo(RenderedObject* obj);
  void Update() override;
  bool isFree(){
    return freeCam.load();
  };
  void ToggleFreedom(){
    freeCam.store(freeCam.load() ? false : true);
  }
  RenderedObject* GetLinked(){
    return linkedObject;
  }
  void Rotate(float& Pitch, float& Yaw, float& Roll);
};

class RenderedObject : public Object {
  friend class Tracker;
  friend class Physics;
  friend class Engine;
  friend class Graphics;

protected:
  bool loadedModel = false;
  std::atomic<bool> isHidden;
  // This pointer is handled by rstorage
  RStorage::bmResource *model;
  UINT CBVIndex = 0;
  float scale = 1;

public:
  relposVect mPos;
  RenderedObject(Object &obj, RStorage::bmResource *model = nullptr,
                 umID filebModelIndex = 0, float mScale = 1,
                 DirectX::XMFLOAT3 initPos = {0, 0, 0},
                 DirectX::XMFLOAT4 initRot = {0, 0, 0, 1});
  RenderedObject(
      std::string name, UOID uOID, std::function<void()> func = [] {},
      Tracker::Instance *pInstance = nullptr,
      RStorage::bmResource *model = nullptr, umID filebModelIndex = 0,
      float mScale = 1, DirectX::XMFLOAT3 initPos = {0, 0, 0},
      DirectX::XMFLOAT4 initRot = {0, 0, 0, 1});
  RenderedObject(const RenderedObject &obj)
      : Object(obj), loadedModel(obj.loadedModel),
        isHidden(obj.isHidden.load()), model(obj.model), CBVIndex(obj.CBVIndex),
        scale(obj.scale) {};
};

class PhysicsObject : public RenderedObject {
  friend class Tracker;
  friend class Physics;
  friend class Engine;

protected:
  std::vector<DirectX::XMFLOAT4> pDir;
  float gravpull = 0;
  DirectX::XMFLOAT4 grav{0, 0, 0, 0};
  float mass = 1;
  float friction = 0;
  DirectX::XMFLOAT3 velDir{0, 0, 0};
  float speed = 0;
  std::atomic<bool> updated;
  std::atomic<bool> Collision;
  std::mutex PhysicsUpdate;
  // Instanced buffer specific to object for physics calculations
  cl::Buffer clPositionBuff;
  // Only important for gravity/ loading reasons. Dont impliment until necessary
public:
  PhysicsObject(RenderedObject &obj, float mMass = 1, float mFriction = 0,
                DirectX::XMFLOAT3 initVelDir = {0, 0, 0}, float initSpeed = 0);
  PhysicsObject(
      std::string name, UOID uOID, std::function<void()> func = [] {},
      Tracker::Instance *pInstance = nullptr,
      RStorage::bmResource *model = nullptr, umID filebModelIndex = 0,
      float mScale = 1, DirectX::XMFLOAT3 initPos = {0, 0, 0},
      DirectX::XMFLOAT4 initRot = {0, 0, 0, 1}, float mMass = 1,
      float mFriction = 0, DirectX::XMFLOAT3 initVelDir = {0, 0, 0},
      float initSpeed = 0);
  void Move(DirectX::XMFLOAT4 Dir, float Dist = 1);
  void CollReset();
  bool CollCheck();
  void Update() override;
};
