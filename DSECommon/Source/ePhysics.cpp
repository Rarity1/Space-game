#include "ObjectTracking.h"
#include "ePhysics.h"
#include "RStorage.h"
#include <CL/opencl.hpp>
#include <Exceptions.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include "Profiler.h"
#include "Threads.h"
//#pragma OPENCL EXTENSION cl_khr_d3d11_sharing : enable


Physics::Physics(const int& UpdateRate, class Tracker& Tracker) :
    Tracker(Tracker),
    urate(UpdateRate),
    GConst(6.67430 * pow(10, -11))

{
    coreCount = std::thread::hardware_concurrency();
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    assert(platforms.size() > 0);
    auto& platform = platforms.front();
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

    assert(devices.size() > 0);

    auto& device = devices.front();
    auto vendor = device.getInfo<CL_DEVICE_VENDOR>();
    auto version = device.getInfo<CL_DEVICE_VERSION>();


    context = cl::Context{ device };
    auto path = _CURRENTPATH;
    std::ifstream phys(path.string() + "\\" + "OpenCL\\ePhysics.cl");
    std::ifstream sphr(path.string() + "\\" + "OpenCL\\SphrKern.cl");


    //Replace this with something that works in release
    assert(phys ? true : false);
    assert(sphr ? true : false);

    std::string p(std::istreambuf_iterator<char>{phys}, {});
    std::string s(std::istreambuf_iterator<char>{sphr}, {});

    Sphere = cl::Program(context, s);
    FullColl = cl::Program(context, p);
    UploadQueue = cl::CommandQueue{context, devices.front()};
    //Add error checking here
    //Sphere.build({ device });
    FullColl.build({ device }) >> chk;

    
    //Write something that can automatically assign child processess to new THREADS instance
    tMain = std::make_unique<THREADS>(coreCount);

    clGetDeviceInfo(device.get(), CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &clLocalMemSize, 0);
}

Physics::~Physics()
{
  tMain.reset();
}

bool Physics::QueueCLBuffer(PhysicsObject *obj) {
  auto &pObject = *obj;
  auto bResource = pObject.GetResource();
  if (bResource) {

    if (pObject.GetModel()) {
      if (!pObject.WorkQ.get()) {
        pObject.clPositionBuff =
            cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(FLOAT3));
        pObject.WorkQ =
            std::make_unique<cl::CommandQueue>(context, devices.front());
        pObject.WorkQ->finish() >> chk;
      }
      if (!bResource->clBuff.get()) {
        std::vector<FLOAT3> WBone;
        std::vector<int> IndexOffset;
        auto &Model = *pObject.GetModel();
        WBone.resize(std::size(Model.bdata));
        for (auto b = 0; b < std::size(WBone); b++) {
          WBone[b] = Model.bdata[b].sphere.Center;
        }
        bResource->clBuff = cl::Buffer(context, CL_MEM_READ_ONLY,
                                       sizeof(std::array<Vertex, 3>) *
                                           Model.MappedVertices.size());
        bResource->clBoneBuff = cl::Buffer(context, CL_MEM_READ_ONLY,
                                           sizeof(FLOAT3) * std::size(WBone));
        UploadQueue.enqueueWriteBuffer(bResource->clBuff, CL_FALSE, 0,
                                       sizeof(std::array<Vertex, 3>) *
                                           Model.MappedVertices.size(),
                                       Model.MappedVertices.data());
        return true;
      }
    }
  }
  return false;
}

void Physics::Update() {
  auto& Instances = Tracker.GetActiveInstances();
  QueueMTX.lock();
  uint32_t Queued = 0;
  for (auto &Inst : Instances) {
    for (auto &Obj : Inst.second->GetInstanceObjects()) {
      auto pObj = Tracker.IsPhysics(Obj.second->GetID());
      if(pObj){
      if (QueueCLBuffer(pObj))
        Queued++;
      }

    }
  }
  if (Queued > 0)
    UploadQueue.finish();
  QueueMTX.unlock();
  // std::for_each(trackedObjects.begin(), trackedObjects.end(), [this](auto& m)
  // {cGravity(&m); });
  pCollison();
  ticker.incCount();
  if (Clock.Peek() >= 1.0) {
    tsPrintBuffer::QueuePrintF(
        "Current avg. TPS: {} \n",
    std::to_string(ticker.cGet() / Clock.Mark()));
    ticker.reset();
  }
}

/*
void Physics::cGravity(Object* obj) {
    
    if (obj->ParentObject != nullptr) {
        //obj->grav = fDirection(obj->mPos->position, obj->mworld->mPos->position);
        //obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(fDistance(obj->mPos->position, obj->mworld->mPos->position), 2))) * timer.Current();

        float mag = obj->speed + obj->gravpull;
        FLOAT3 dotpro = { 0,0,0 };
        auto Tempvel = obj->velDir;
        XMStoreFloat3(&Tempvel, XMLoadFloat3(&Tempvel) * obj->speed + XMLoadFloat4(&obj->grav) * obj->gravpull);
        XMStoreFloat3(&dotpro, XMVector3Dot(XMLoadFloat3(&Tempvel), XMLoadFloat3(&Tempvel)));
        obj->speed = sqrt(dotpro.x);
        XMStoreFloat3(&obj->velDir, XMLoadFloat3(&Tempvel) / obj->speed);
    } 
}


*/



int Physics::bIndex(std::vector<int> w, int bInd) {
    int Index = -1;
    for (auto i = 0; i < std::size(w); i++) {
        if (w[i] == bInd) {
            Index = i;
        }
    }
    return Index;
}

//This dont work ree
void Physics::CalProportionalSpeed(FLOAT4& VelDir1, FLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2) {
    float massScal1 = (Mass1 / (Mass1 + Mass2));
    float massScal2 = (Mass2 / (Mass1 + Mass2));
    massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
    massScal2 = massScal2 < 0.00001 ? 0 : massScal2;


    FLOAT4 RelDir =  VelDir2 * VSpeed2 * -massScal1 + VelDir1 * VSpeed1 * -massScal2;
    float RelDot = RelDir * RelDir;
    float mag = sqrt(RelDot);


    VelDir1 = (RelDir * -massScal2).Normal();
    VSpeed1 = mag * massScal2;

    VelDir2 = (RelDir * -massScal1).Normal();
    VSpeed2 = mag * massScal1;
}


//rewrite this so a triangle can be checked again against a different bone
std::function<void()> Physics::CheckVertexDirection(
    std::vector<uint32_t> &Result, ModelData &objudat,
    std::vector<std::atomic<bool>> &IndexChecked, uint32_t &BoneIndex,
    SphereCollider &CollSp,
    std::vector<std::array<Vertex, 3>> &Vertices) {
  return [&objudat, &IndexChecked, &BoneIndex, &CollSp, &Vertices, &Result]() {
    FLOAT3 bdirection =
        fDirection(objudat.bdata[BoneIndex].sphere.Center, CollSp.Center);

    std::for_each(
        objudat.bdata[BoneIndex].Indices.begin(),
        objudat.bdata[BoneIndex].Indices.end(),
        [&IndexChecked, &Result, &objudat, &bdirection, &Vertices](auto &e) {
          if (!IndexChecked[objudat.IndexToVertex[e]].load()) {
            if (0 <= (bdirection * Vertices[objudat.IndexToVertex[e]][0].normal)) {
              Result.emplace_back(objudat.IndexToVertex[e]);
            }
            IndexChecked[objudat.IndexToVertex[e]].store(true);
          }
        });
  };
}

Physics::WORKINDI Physics::ProcCollide(PhysicsObject &obj, PhysicsObject &obj2) {
  auto realpos1 = obj.mPos.Get();
  auto realpos2 = obj2.mPos.Get();
  auto dir = fDirection(realpos1, realpos2);
  auto dist = fDistance(realpos1, realpos2);

  FLOAT3 RelPositon = dir*dist;

  auto &objudat = *Tracker.GetModel(((PhysicsObject&)obj).ModelID);
  auto &obj2udat = *Tracker.GetModel(((PhysicsObject&)obj2).ModelID);
  auto &objbdata = objudat.bdata;
  auto &obj2bdata = obj2udat.bdata;
  auto &MappedVert1 = objudat.MappedVertices;
  auto &MappedVert2 = obj2udat.MappedVertices;
  struct WorDat{
    std::array<uint32_t, 2> BoneIndices;
    std::vector<uint32_t> Indices1;
    std::vector<uint32_t> Indices2;
  };
  std::vector<WorDat> WorkData;
  //WorkData.reserve(obj2bdata.size() * objbdata.size());

  for (auto &b2 : obj2bdata) {
    SphereCollider sph2 = b2.sphere;
    sph2.Center = sph2.Center + RelPositon;
    for (auto &b : objbdata) {
      if (fDistance(b.sphere.Center, sph2.Center) <= b.sphere.Radius + sph2.Radius) {
        WorkData.emplace_back(WorDat{{b.bIndex, b2.bIndex}, {}, {}});
      }
    }
  }
  WORKINDI Result;
  if (WorkData.size() == 0)
    return Result;
  Result.Indices.reserve(MappedVert1.size() + MappedVert2.size());

  std::vector<std::atomic<bool>> objCheck1(MappedVert1.size());
  std::vector<std::atomic<bool>> objCheck2(MappedVert2.size());
  
  std::vector<THREADS::WRef> refs1;
  std::vector<THREADS::WRef> refs2;

  for (auto& WD : WorkData) {
    std::array<SphereCollider, 2> CollSp;
    CollSp[0] = objudat.bdata[WD.BoneIndices[0]].sphere;
    CollSp[1] = obj2udat.bdata[WD.BoneIndices[1]].sphere;
    CollSp[0].Center =
                  CollSp[0].Center - RelPositon;

    CollSp[1].Center =
                  CollSp[1].Center + RelPositon;

    auto &W1Indices = WD.Indices1;
    auto &W2Indices = WD.Indices2;
    W1Indices.reserve(objudat.MappedVertices.size());
    W2Indices.reserve(obj2udat.MappedVertices.size());
    //refs1.emplace_back(tMain->gPushWork(
        CheckVertexDirection(W1Indices, objudat, objCheck1,
                             WD.BoneIndices[0], CollSp[1], MappedVert1)();
                            //));
    //refs2.emplace_back(tMain->gPushWork(
        CheckVertexDirection(W2Indices, obj2udat, objCheck2,
                             WD.BoneIndices[1], CollSp[0], MappedVert2)();
                            //));
  }
  std::vector<uint32_t> WorkIndi1;
  std::vector<uint32_t> WorkIndi2;

  WorkIndi1.reserve(MappedVert1.size());
  WorkIndi2.reserve(MappedVert2.size());
  //tMain->gEndWork(refs1);
  //tMain->gEndWork(refs2);
  for (auto& wd : WorkData) {
    WorkIndi1.append_range(wd.Indices1);
  }
  for (auto& wd : WorkData) {
    WorkIndi2.append_range(wd.Indices2);
  }

  Result.Position = RelPositon;
  Result.Indices = WorkIndi1;
  Result.wWorkCount = WorkIndi1.size();
  Result.Indices.append_range(WorkIndi2);
  Result.tWorkCount = WorkIndi2.size();
  return Result;
}

// rewrite to only process loaded and tracked models
void Physics::pCollison() {
  std::vector<THREADS::WRef> refs;
  auto instances = Tracker.GetActiveInstances();
  for (auto &instance : instances) {
    auto &ObjectsToProcess = instance.second->GetInstanceObjects();
    CollModels.reserve(ObjectsToProcess.size());
    for (auto &tO0 : ObjectsToProcess) {
      auto Object = Tracker.IsPhysics(tO0.second.get());
      if (Object) {
        auto &obj = *Object;
        if (!obj.GetModel()) {
            break;
        }
        for (auto &vert : obj.GetModel()->MappedVertices) {
          vert[0].test = vert[0].test == 1 ? 0 : vert[0].test;
          vert[1].test = vert[1].test == 1 ? 0 : vert[1].test;
          vert[2].test = vert[2].test == 1 ? 0 : vert[2].test;
        }
        refs.emplace_back(tMain->gPushWork([this, &obj, &ObjectsToProcess]() {


          FLOAT3 Pos1 = obj.mPos.Get();
          auto &sph1 = obj.GetModel()->Sphere;

          for (auto &tO20 : ObjectsToProcess) {
            auto Object2 = Tracker.IsPhysics(tO20.second.get());
            if (Object2) {
              auto &obj2 = *Object2;
              if (obj2 != obj) {
                CollFlag cll = CollFlag(obj.uOID, obj2.uOID);
                FLOAT3 Pos2 = obj2.mPos.Get();
                auto sph2 = obj2.GetModel()->Sphere;
                auto tdist = fDirection(Pos1, Pos2);
                tdist = tdist * fDistance(Pos1, Pos2);
                sph2.Center = {tdist.x, tdist.y, tdist.z};
                bool AddColl = false;
                if (fDistance(sph1.Center, sph2.Center) <=
                  sph1.Radius + sph2.Radius) {
                    AddColl = true;
                }
                if (AddColl) {
                  cmMtx.lock();
                  if (!CollModels.contains(cll)) {
                    CollModels.insert(std::pair(cll, true));
                  } else {
                    CollModels[cll].store(true);
                  }
                  cmMtx.unlock();
                }
              }
            }
          }
        }));
      }
    }
  }
  tMain->gEndWork(refs);
  std::vector<THREADS::WRef> Wrefs;
  Wrefs.reserve(CollModels.size());

  cmMtx.lock();
  for (auto &interaction : CollModels) {
    auto &bol = interaction.second;
    auto& cModel = interaction.first;
    if (bol.load()) {
      //Wrefs.emplace_back(tMain->gPushWork([this, &cModel, &bol]() {
        
        auto &ID1 = cModel.obj;
        auto &ID2 = cModel.obj2;
        std::array<int, 2> wSize = {0, 0};
        FLOAT3 MoveD1{0, 0, 0};
        FLOAT3 MoveD2{0, 0, 0};
        FLOAT3 Zero{0, 0, 0};
        auto &obj = *Tracker.IsPhysics(ID1);
        auto &obj2 = *Tracker.IsPhysics(ID2);
        WORKINDI WorkIndi = ProcCollide(obj, obj2);

        if (WorkIndi.wWorkCount != 0 && WorkIndi.tWorkCount != 0) {
          auto &Queue = obj.WorkQ;
          obj.QueueMutex.lock();
          wSize[0] = WorkIndi.wWorkCount;
          wSize[1] = WorkIndi.tWorkCount;
          std::vector<RETURNDATA> retdata(wSize[0]);
          cl::Buffer IndexBuffer(context, CL_MEM_READ_ONLY,
                                 WorkIndi.Indices.size() * sizeof(int));
          cl::Kernel kerns(FullColl, "coll");
          Queue->enqueueWriteBuffer(obj.clPositionBuff, CL_FALSE, 0, sizeof(FLOAT3),
                                    &WorkIndi.Position);
          Queue->enqueueWriteBuffer(IndexBuffer, CL_FALSE, 0,
                                    WorkIndi.Indices.size() * sizeof(cl_int),
                                    WorkIndi.Indices.data());
          size_t retdatSize = sizeof(RETURNDATA) * retdata.size();
          cl::Buffer rbuffer(context, CL_MEM_READ_WRITE, retdatSize);
          cl_float fillbuff = 0.0;
          Queue->enqueueFillBuffer(rbuffer, fillbuff, 0, retdatSize);
          Queue->finish() >> chk;

          kerns.setArg(0, obj.GetResource()->clBuff);
          kerns.setArg(1, obj2.GetResource()->clBuff);
          kerns.setArg(2, obj.clPositionBuff);
          kerns.setArg(3, IndexBuffer);
          kerns.setArg(4, rbuffer);

          size_t wsize[2] = {(size_t)WorkIndi.wWorkCount,
                             (size_t)WorkIndi.tWorkCount};
          size_t offsize[2] = {0, (size_t)WorkIndi.wWorkCount};

          clEnqueueNDRangeKernel(Queue->get(), kerns.get(), 2, offsize,
                                         wsize, nullptr, 0, NULL, NULL) >> chk;

          Queue->finish()>>chk;
          Queue->enqueueReadBuffer(rbuffer, CL_TRUE, 0,
                                           sizeof(RETURNDATA) * retdata.size(),
                                           retdata.data());
          Queue->finish()>>chk;
          obj.QueueMutex.unlock();

          // auto dir = fDirection(Pos1, Pos2);
          // auto dist = fDistance(Pos1, Pos2);
          // FLOAT3 RelPos = dir * dist;

          int coutn = 0;
          for (auto &r : retdata) {
            if (r.dist[0] != 0 && r.dist[1] != 0) {
              auto& vert = obj.GetModel()->MappedVertices[r.index[0]];
              auto& vert2 = obj2.GetModel()->MappedVertices[r.index[1]];
              vert[0].test = 1;
              vert[1].test = 1;
              vert[2].test = 1;
              vert2[0].test = 1;
              vert2[1].test = 1;
              vert2[2].test = 1;
              auto normal = obj.GetModel()->MappedVertices[r.index[0]][0].normal;
              auto normal2 = obj2.GetModel()->MappedVertices[r.index[1]][0].normal;
              
              auto dir = fDirection(normal, normal2);

              MoveD1 = MoveD1 +
                       dir *
                           -0.5;
              MoveD2 = MoveD2 +
                       dir *
                           0.5;
              coutn++;
            }
          }
          if (coutn != 0) {
            float massScal1 = (obj.mass / (obj.mass + obj2.mass));
            float massScal2 = (obj2.mass / (obj.mass + obj2.mass));
            massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
            massScal2 = massScal2 < 0.00001 ? 0 : massScal2;

            MoveD1 = MoveD1 / (float)coutn * massScal2;
            MoveD2 = MoveD2 / (float)coutn * massScal1;

            FLOAT3 Zero(0, 0, 0);

            // Fixxx thissss
            // CalProportionalSpeed(obj->velDir, obj2->velDir, obj->speed,
            // obj2.speed, obj->mass, obj2.mass);
            obj.Move(fDirection(Zero, MoveD2), fDistance(Zero, MoveD2));
            obj2.Move(fDirection(Zero, MoveD1), fDistance(Zero, MoveD1));
          }
        }
        bol.store(false);
      //}));
    }
  }
  cmMtx.unlock();
  //tMain->gEndWork(Wrefs);
  ClearCollisionQueue();
}

void Physics::ClearCollisionQueue(){
  std::vector<CollFlag> erase;
  cmMtx.lock();
  for(auto& Queue : CollModels){
    if(!Queue.second.load()){
      if(!Tracker.IsPhysics(Queue.first.obj) && !Tracker.IsPhysics(Queue.first.obj)){
        erase.emplace_back(Queue.first);
      }
    }
  }
  for(auto& er: erase){
    CollModels.erase(er);
  }
  cmMtx.unlock();
};