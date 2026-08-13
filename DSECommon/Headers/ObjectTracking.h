#pragma once
#include "Exceptions.h"
#include "InputHandler.h"
#include "CommonStructs.h"
#include "ModelData.h"
#include "RStorage.h"
#include "Threads.h"
#include "ePhysics.h"
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



class Tracker {
  Exceptions::CheckerToken chk;
public:
  class Instance;
  Tracker(RStorage &rs, Input& Hndlr) : 
    InputHndlr(Hndlr),
    tTracker(std::make_unique<THREADS>((int)std::thread::hardware_concurrency())),
    storage(rs){};
  ~Tracker() {};

  // Returns ID of current model given UOID(Unique Object ID)
  inline RStorage::bmResource * GetResource(umID mID) {return storage.GetResource(mID);};
  inline ModelData * GetModel(umID mID) {return storage.GetModel(mID);};
  inline std::filesystem::path GetTexture(std::string Name){return storage.getTexture(Name);}
  // Creates an instance and returns it regardless of if the id given is used
  Instance &initInstance(std::function<void()> func = []{});
  // Returns nullptr if instance isnt found
  Instance* getInstance(UOID instanceID);

  std::shared_ptr<Object>& initObject(
      std::string name,
      std::function<void()> func = []{}, Instance* oInstance = nullptr);

  std::shared_ptr<Object>& initCameraObject(
      std::string name, std::function<void()> func = []{},
      FLOAT3 initPos = {0, 0, 0},
      FLOAT4 initRot = {1, 0, 0, 0},
      FLOAT4 initUpDirection = {0, 0, 1, 0},
      bool Active = false);

  std::shared_ptr<Object>& initRenderObject(
      Instance &Instance, std::string name, std::function<void()> func = []{},
      umID filebModelIndex = 0, float mScale = 1.0,
      FLOAT3 initPos = {0, 0, 0},
      FLOAT4 initRot = {0, 0, 0, 1});

  std::shared_ptr<Object>& initPhysObject(
      std::string name, std::function<void()> func = []{},
      umID filebModelIndex = 0, float mScale = 1.0,
      FLOAT3 initPos = {0, 0, 0},
      FLOAT4 initRot = {0, 0, 0, 1}, float mMass = 0.0,
      float mFriction = 0.01, FLOAT3 initVelDir = {0, 0, 0},
      float initSpeed = 0);

  void unloadObject(UOID obj);

  
  inline void loadModel(umID mID){storage.loadModel(mID);};
  bool isInstanceActive(UOID inst){return ActiveInstances.contains(inst);}
  void MakeInstanceActive(UOID id);
  const std::unordered_map<UOID, std::shared_ptr<Instance>> &GetActiveInstances() {
    return ActiveInstances;
  };
  Input& InputHndlr;
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
  
  // umID to array of CBV data

  
  // all Objects loaded in this instance
  std::unordered_map<UOID, std::shared_ptr<Object>> Objects;
  std::unordered_map<UOID, std::shared_ptr<Object>> RenderObjects;
  std::unordered_map<UOID, std::shared_ptr<Object>> CameraObjects;
  std::unordered_map<UOID, std::shared_ptr<Object>> PhysicsObjects;
  public:
  struct ModelLinkedViewBuffers{
    ModelLinkedViewBuffers(){};
    ModelLinkedViewBuffers(const ModelLinkedViewBuffers& old){
      ViewBuffLock.lock();
      ViewBuffer = old.ViewBuffer;
      LinkedObjects = old.LinkedObjects;
      FlaggedForUpdate = old.FlaggedForUpdate;
      ViewBuffLock.unlock();
    };
    ModelLinkedViewBuffers(ModelLinkedViewBuffers&& old){
      ViewBuffLock.lock();
      ViewBuffer = old.ViewBuffer;
      LinkedObjects = old.LinkedObjects;
      FlaggedForUpdate = std::move(old.FlaggedForUpdate);
      ViewBuffLock.unlock();
    };
    std::list<std::shared_ptr<Object>> LinkedObjects;
    CBVData * ViewBuffer = nullptr;
    bool FlaggedForUpdate = true;
    uint32_t vCount = 0;
    mutable std::mutex ViewBuffLock;
  };
  private:
  std::unordered_map<umID, ModelLinkedViewBuffers> ViewBuffers;
  public:
  const std::unordered_map<umID, ModelLinkedViewBuffers>& GetViewBuffers()const {return ViewBuffers;};
  CBVData* GetViewMatrix(umID ID, uint32_t Index){if(ViewBuffers.contains(ID)){return &ViewBuffers[ID].ViewBuffer[Index];}else {
    "WHYYYY" >> chk;
    return nullptr;
  }};
  CBVData** GetViewMatrixAddress(umID ID){if(ViewBuffers.contains(ID)){return std::addressof(ViewBuffers[ID].ViewBuffer);}else {
    "WHYYYY" >> chk;
    return nullptr;
  }};
  class Instance {
    friend class Tracker;
  
    // Self explanatory
    // if an object is linked to a model it is at least a rendered object
    // Physics Objects
    // parent tracker class
    uint32_t Count;
    std::mutex InstanceObjLock;
    std::shared_ptr<Object> activeCamera;
    
  public:
    //The Origin object
    const std::shared_ptr<Object> InstanceObject;
    private:
    const UOID uOID;
    public:
    const UOID GetID(){return uOID;}
    Instance(Tracker &Parent, std::function<void()> func = []{});
    Tracker &pTracker;
    std::unordered_map<UOID, std::shared_ptr<Object>>& GetInstanceObjects();
    void SetActiveCamera(std::shared_ptr<Object> obj){activeCamera = obj;};
    CameraObject* GetActiveCamera() const{return (CameraObject*)activeCamera.get();};
    void AddObject(std::shared_ptr<Object>& obj);
    void MakeInstanceActive();
    bool operator==(Instance &comp) { return comp.uOID == uOID; }
  };
  const std::shared_ptr<Object>& GetObjectRef(UOID uoid){ return Objects[uoid];}
  void MakeRenderObj(UOID uOID);
  void MakePhysicsObj(UOID uOID);
  void MakeCameraObj(UOID uOID);
  inline CameraObject* IsCamera(UOID ID){return CameraObjects.contains(ID) ? (CameraObject*)CameraObjects[ID].get() : nullptr;};

  inline RenderedObject* IsRendered(UOID ID){return RenderObjects.contains(ID) ? (RenderedObject*)RenderObjects[ID].get() : nullptr;};

  inline PhysicsObject* IsPhysics(UOID ID){return PhysicsObjects.contains(ID) ? (PhysicsObject*)PhysicsObjects[ID].get() : nullptr;};
  CameraObject *IsCamera(Object *Obj);
  RenderedObject *IsRendered(Object *Obj);
  PhysicsObject *IsPhysics(Object *Obj);
  CameraObject *IsCamera(std::shared_ptr<Object>& Obj){return IsCamera(Obj.get());};
  RenderedObject *IsRendered(std::shared_ptr<Object>& Obj){return IsRendered(Obj.get());};
  PhysicsObject *IsPhysics(std::shared_ptr<Object>& Obj){return IsPhysics(Obj.get());};

  void LinkModel(UOID ID, umID umID);
  void unLinkModel(UOID ID);
  const std::list<umID>& GetModelList(){return storage.GetModelList();};
  uint32_t ModelObjectCount(umID umID) {
    if (ViewBuffers.contains(umID)) {
      return ViewBuffers[umID].LinkedObjects.size();
    } else
      return 0;
  };
  const std::unordered_map<umID, ModelLinkedViewBuffers>& ModelObjects()const{return ViewBuffers;};

  bool CBVFlagged(umID umID){
  if(ViewBuffers.contains(umID)){
    return ViewBuffers[umID].FlaggedForUpdate;
  }
  return false;
  };
  void CBVUnFlag(umID umID){
    if(ViewBuffers.contains(umID)){
      ViewBuffers[umID].ViewBuffLock.lock();
      ViewBuffers[umID].FlaggedForUpdate = false;
      ViewBuffers[umID].ViewBuffLock.unlock();

    }
  }
  private:
  RStorage &storage;
  IDAllocator idTracker;
  std::unordered_map<UOID, std::shared_ptr<Instance>> Instances;
  // Tracker Owns all objects no object will unload until removed from this map

  std::unordered_map<UOID, std::shared_ptr<Instance>> ActiveInstances;

  std::shared_ptr<Object>& initOriginObject(std::function<void()> func = []{});
};
class relposVect {
protected:
  std::unique_ptr<FLOAT3> position;
  FLOAT3 lastposition = {0, 0, 0};
  FLOAT4 rotation{0, 0, 0, 1};
  std::mutex posMtx;
public:
  relposVect() : position(std::make_unique<FLOAT3>(0, 0, 0)), posMtx() {};
  relposVect(FLOAT3 initPos)
      : position(std::make_unique<FLOAT3>(initPos)) {};
  relposVect(FLOAT3 initPos, FLOAT4 initRot)
      : position(std::make_unique<FLOAT3>(initPos)),
        rotation(initRot) {};
  ~relposVect() = default;
  relposVect(relposVect &&old) noexcept {
    position = std::move(old.position);
    lastposition = std::move(old.lastposition);
    rotation = std::move(old.rotation);
  };
  relposVect(const relposVect &old) {
    position = std::make_unique<FLOAT3>(*old.position);
    lastposition = old.lastposition;
    rotation = old.rotation;
  };
  relposVect &operator=(const relposVect &old) {
    *position = *old.position;
    lastposition = old.lastposition;
    rotation = old.rotation;
    return *this;
  }
  const FLOAT3 Move(FLOAT3 &NewPos);
  const FLOAT3 Get();
  const FLOAT3 GetLast();
  const FLOAT4 GetRotation();
  void SetRotation(FLOAT4& Rotation);
  const FLOAT3 *GetPtr() { return position.get(); };
};

class CameraPosition : public relposVect{

  FLOAT4 upDirection = {0, 0, 1, 0};
  public:
  CameraPosition(FLOAT3 initPos, FLOAT4 initRot, FLOAT4 UpDirection):
  relposVect(initPos, initRot),
  upDirection(UpDirection)
  {};
  const FLOAT4 GetUpDirection();
  void SetUpDirection(FLOAT4 UpDirection);
};

class Object {

  friend class Tracker;
  friend class Engine;

protected:
  std::string name = "";
  const UOID uOID;
  //function to run
  std::function<void()> Script;
  mutable std::mutex Scriptlk;
  std::shared_ptr<Object> Parent;
  std::unordered_map<UOID, std::shared_ptr<Object>> Children;
  Tracker& pTracker;
  bool isOrigin = false;
public:
  Object(
      std::string name, UOID uOID,
      Tracker& tracker);
  Object(const Object& old):name(old.name),
  uOID(old.uOID),Parent(old.Parent),
  Children(old.Children),pTracker(old.pTracker),
  isOrigin(old.isOrigin){
    old.Scriptlk.lock();
    Scriptlk.lock();
    Script = old.Script;
    Scriptlk.unlock();
    old.Scriptlk.unlock();
  };
  Object(Object&& old):name(std::move(old.name)),
  uOID(std::move(old.uOID)),Parent(std::move(old.Parent)),
  Children(std::move(old.Children)),pTracker(old.pTracker),
  isOrigin(std::move(old.isOrigin)){
    old.Scriptlk.lock();
    Scriptlk.lock();
    Script = std::move(old.Script);
    Scriptlk.unlock();
    old.Scriptlk.unlock();
  };
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
  Object *AddParent(const std::shared_ptr<Object> &obj) {
    Object *oldParent = nullptr;
    if (obj) {
      if (Parent) {
        Parent->Children.erase(uOID);
        oldParent = Parent.get();
      }
      Parent = obj;
      Parent->Children.insert({uOID, pTracker.GetObjectRef(uOID)});
    }

    return oldParent;
  }
  std::shared_ptr<Object> GetParent();
  std::unordered_map<UOID, std::shared_ptr<Object>>& GetChildren(){return Children;};
  std::shared_ptr<Object> RemoveParent() {
    auto oldParent = Parent;
    if (Parent) {
      Parent->Children.erase(uOID);
    }
    Parent = {};
    return oldParent;
  };

  // Update important data and run linked scripts
  // Maybe cascade to Child objects?
  void linkScript(std::function<void()> func){
    Scriptlk.lock();
    Script = func;
    Scriptlk.unlock();
  }
  void* getScript(){
    return &Script;
  }
  virtual void Update() { 
    Scriptlk.lock();
    if(Script){
      Script(); 
    }
    Scriptlk.unlock();
   };
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
  protected:
  std::atomic<bool> isActive = false;
  std::atomic<bool> freeCam = true;
  
  // FLOAT4 forwardDirect = {1, 0, 0, 0};
  FLOAT4X4 cMatrix;
  std::shared_ptr<Object> linkedObject;
  EngineTime ucontrolClock;
  EngineTime inputDelay;
  Input::dispatchID dispID;
public:
  CameraPosition cPos;
  
  CameraObject(const CameraObject&) = delete;
  CameraObject(
      Object& obj,
      FLOAT3 initPos = {0, 0, 0},
      FLOAT4 initRot = {1, 0, 0, 0},
      FLOAT4 initUpDirection = {0, 0, 1, 0},
      bool Active = false);
  CameraObject(
      std::string name, UOID uOID,
      Tracker& tracker,
      FLOAT3 initPos = {0, 0, 0},
      FLOAT4 initRot = {1, 0, 0, 0},
      FLOAT4 initUpDirection = {0, 0, 1, 0},
      bool Active = false);
  // Returns previous active camera. Returns nullptr if first camera set
  void MakeActive(Tracker::Instance& Instance) {
    AddParent(Instance.InstanceObject);
    Instance.SetActiveCamera(pTracker.GetObjectRef(uOID));
  };
  void LinkTo(const std::shared_ptr<Object>& obj);
  void Update() override;
  bool isFree(){
    return freeCam.load();
  };
  void ToggleFreedom(){
    freeCam.store(freeCam.load() ? false : true);
  }
  std::shared_ptr<Object> GetLinked(){
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
  std::atomic<bool> isHidden;
  // This pointer is handled by rstorage
  std::_List_iterator<std::shared_ptr<Object>> CBVRef;
  uint32_t CBVIndex = 0;
  std::atomic<bool> hasModel;
  umID ModelID = 0;
  float scale = 1;
public:
  relposVect mPos;
  RenderedObject(std::string name, UOID uOID,
      Tracker& tracker, float mScale = 1,
                 FLOAT3 initPos = {0, 0, 0}, FLOAT4 initRot = {0, 0, 0, 1});
  RenderedObject(Object& old, float mScale = 1, FLOAT3 initPos = {0, 0, 0},
                 FLOAT4 initRot = {0, 0, 0, 1});
  RenderedObject(const RenderedObject &obj)
      : Object((const Object)obj), isHidden(obj.isHidden.load()),
        CBVRef(obj.CBVRef), scale(obj.scale) {};
  void LinkToModel(umID umID);
  void loadModel() noexcept;
  ModelData* GetModel(){return pTracker.GetModel(ModelID);};
  RStorage::bmResource* GetResource(){return pTracker.GetResource(ModelID);};
};

class PhysicsObject : public RenderedObject {
  friend class Tracker;
  friend class Physics;
  friend class Engine;

protected:
  std::vector<FLOAT3> pDir;
  float gravpull = 0;
  FLOAT4 grav{0, 0, 0, 0};
  float mass = 1;
  float friction = 0;
  FLOAT3 velDir{0, 0, 0};
  float speed = 0;
  std::atomic<bool> updated;
  std::atomic<bool> Collision;
  std::mutex PhysicsUpdate;
  // Instanced buffer specific to object for physics calculations
  cl::Buffer clPositionBuff;
  std::unique_ptr<cl::CommandQueue> WorkQ;
  std::mutex QueueMutex;
  // Only important for gravity/ loading reasons. Dont impliment until necessary
public:
PhysicsObject(
      RenderedObject& obj,
      float mScale = 1, FLOAT3 initPos = {0, 0, 0},
      FLOAT4 initRot = {0, 0, 0, 1}, float mMass = 1,
      float mFriction = 0, FLOAT3 initVelDir = {0, 0, 0},
      float initSpeed = 0);
  PhysicsObject(
      std::string name, UOID uOID,
      Tracker& tracker,
      float mScale = 1, FLOAT3 initPos = {0, 0, 0},
      FLOAT4 initRot = {0, 0, 0, 1}, float mMass = 1,
      float mFriction = 0, FLOAT3 initVelDir = {0, 0, 0},
      float initSpeed = 0);
  void Move(FLOAT4 Dir, float Dist = 1);
  void CollReset();
  bool CollCheck();
  void Update() override;
};
