#include "ePhysics.h"
#include "ObjectTracking.h"
#include "RStorage.h"
#include <Exceptions.h>
#include <cstdint>
#include <cstdio>
#include <fstream>

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
    std::ifstream phys("OpenCL\\ePhysics.cl");
    std::ifstream sphr("OpenCL\\SphrKern.cl");


    //Replace this with something that works in release
    assert(phys ? true : false);
    assert(sphr ? true : false);

    std::string p(std::istreambuf_iterator<char>{phys}, {});
    std::string s(std::istreambuf_iterator<char>{sphr}, {});

    Sphere = cl::Program(context, s);
    FullColl = cl::Program(context, p);


    //Add error checking here
    //Sphere.build({ device });
    auto error = FullColl.build({ device });
    assert(error == CL_SUCCESS);


    queue = cl::CommandQueue{ context, device };
    
    //Write something that can automatically assign child processess to new THREADS instance
    tMain = std::make_unique<THREADS>(coreCount, 2);

    clGetDeviceInfo(device.get(), CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &clLocalMemSize, 0);
}

Physics::~Physics()
{
}

bool Physics::QueueCLBuffer(Object& obj){
  auto& pObject = (PhysicsObject&)obj;
  if(!pObject.model) return false;
  if(!pObject.model->clBuff.get()){
      pObject.model->clBuff =
          cl::Buffer(context, CL_MEM_READ_ONLY,
                     sizeof(std::array<ModelData::Vertex, 3>) *
                         pObject.model->uData->MappedVertices.size());
      pObject.clPositionBuff =
          cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(DirectX::XMFLOAT3));

      std::vector<DirectX::XMFLOAT3> WBone;
      std::vector<int> IndexOffset;
      WBone.resize(std::size(pObject.model->uData->bdata));
      for (auto b = 0; b < std::size(WBone); b++) {
        WBone[b] = pObject.model->uData->bdata[b].sphere.Center;
      }
      pObject.model->clBoneBuff =
          cl::Buffer(context, CL_MEM_READ_ONLY,
                     sizeof(DirectX::XMFLOAT3) * std::size(WBone));

      queue.enqueueWriteBuffer(pObject.model->clBoneBuff, CL_FALSE, 0,
                               sizeof(DirectX::XMFLOAT3) * std::size(WBone),
                               WBone.data());

      queue.enqueueWriteBuffer(pObject.model->clBuff, CL_FALSE, 0,
                               sizeof(std::array<ModelData::Vertex, 3>) *
                                   pObject.model->uData->MappedVertices.size(),
                               pObject.model->uData->MappedVertices.data());
    return true;
  }
  return false;
}

void Physics::Update() {

  auto& Instances = Tracker.GetActiveInstances();
  QueueMTX.lock();
  UINT Queued = 0;
  for (auto &Inst : Instances) {
    for (auto &Obj : Inst->GetPhysicsObjects()) {

      if (QueueCLBuffer(*Obj))
        Queued++;
    }
  }

  if (Queued > 0)
    queue.finish();
  QueueMTX.unlock();

  // std::for_each(trackedObjects.begin(), trackedObjects.end(), [this](auto& m)
  // {cGravity(&m); });
  for (auto &Inst : Instances) {
    pCollison(Inst->GetID());
  }
  
  pSpecReset();
  ticker.incCount();
  if (Clock.Peek() >= 1.0) {
    tsPrintBuffer::QueuePrintF(
        "Current avg. TPS: %s \n",
        {std::to_string(ticker.cGet() / Clock.Mark()).c_str()});
    ticker.reset();
  }
}

/*
void Physics::cGravity(Object* obj) {
    using namespace DirectX;
    if (obj->ParentObject != nullptr) {
        //obj->grav = fDirection(obj->mPos->position, obj->mworld->mPos->position);
        //obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(fDistance(obj->mPos->position, obj->mworld->mPos->position), 2))) * timer.Current();

        float mag = obj->speed + obj->gravpull;
        DirectX::XMFLOAT3 dotpro = { 0,0,0 };
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
void Physics::CalProportionalSpeed(DirectX::XMFLOAT4& VelDir1, DirectX::XMFLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2) {
    using namespace DirectX;
    float massScal1 = (Mass1 / (Mass1 + Mass2));
    float massScal2 = (Mass2 / (Mass1 + Mass2));
    massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
    massScal2 = massScal2 < 0.00001 ? 0 : massScal2;

    XMFLOAT4 TempVelD1 = VelDir1;
    float TempVSpeed1 = VSpeed1;

    XMFLOAT4 RelDir;
    XMStoreFloat4(&RelDir, XMLoadFloat4(&VelDir2) * VSpeed2 * -massScal1 + XMLoadFloat4(&VelDir1)*VSpeed1* -massScal2);
    XMFLOAT4 RelDot;
    XMStoreFloat4(&RelDot, XMVector3Dot(XMLoadFloat4(&RelDir), XMLoadFloat4(&RelDir)));
    float mag = sqrt(RelDot.x);


    XMStoreFloat4(&VelDir1, XMVector3Normalize(XMLoadFloat4(&RelDir) * -massScal2));
    VSpeed1 = mag * massScal2;

    XMStoreFloat4(&VelDir2, XMVector3Normalize(XMLoadFloat4(&RelDir) * -massScal1));
    VSpeed2 = mag * massScal1;
}

std::function<void()> Physics::CheckVertexDirection(
  std::vector<int> &Result,
    ModelData &objudat, std::vector<std::atomic<bool>> &IndexChecked,
    unsigned short &BoneIndex,
    DirectX::BoundingSphere &CollSp,
    std::vector<std::array<ModelData::Vertex, 3>> &Vertices) {
  return [&objudat, &IndexChecked, &BoneIndex, &CollSp,
          &Vertices, &Result]() {
    DirectX::XMFLOAT4 bdirection = fDirection(
        objudat.bdata[BoneIndex].sphere.Center, CollSp.Center);
    Result.reserve(objudat.bdata[BoneIndex].Indices.size());
    std::for_each(
        objudat.bdata[BoneIndex].Indices.begin(),
        objudat.bdata[BoneIndex].Indices.end(),
        [&IndexChecked, &Result, &objudat, &bdirection, &Vertices](auto &e) {
          if (!IndexChecked[objudat.mIndex[e]].load()) {
            using namespace DirectX;
            if (XMVector3Greater(
                    XMVector3Dot(
                        XMLoadFloat4(&bdirection),
                        XMLoadFloat3(
                            &Vertices[objudat.mIndex[e]][0].normal)),
                    XMVectorZero())) {
              IndexChecked[objudat.mIndex[e]].store(true);
              Result.emplace_back(objudat.mIndex[e]);
            }
          }
        });
  };
}

std::function<void()> Physics::CheckVertexDirection(
  std::vector<int> &Result,
    ModelData &objudat, std::vector<bool> &IndexChecked,
    unsigned short &BoneIndex,
    DirectX::BoundingSphere &CollSp,
    std::vector<std::array<ModelData::Vertex, 3>> &Vertices) {
  return [&objudat, &IndexChecked, &BoneIndex, &CollSp,
          &Vertices, &Result]() {
    DirectX::XMFLOAT4 bdirection = fDirection(
        objudat.bdata[BoneIndex].sphere.Center, CollSp.Center);
    Result.reserve(objudat.bdata[BoneIndex].Indices.size());
    std::for_each(
        objudat.bdata[BoneIndex].Indices.begin(),
        objudat.bdata[BoneIndex].Indices.end(),
        [&IndexChecked, &Result, &objudat, &bdirection, &Vertices](auto &e) {
          if (!IndexChecked[objudat.mIndex[e]]) {
            using namespace DirectX;
            if (XMVector3Greater(
                    XMVector3Dot(
                        XMLoadFloat4(&bdirection),
                        XMLoadFloat3(
                            &Vertices[objudat.mIndex[e]][0].normal)),
                    XMVectorZero())) {
              IndexChecked[objudat.mIndex[e]] = true;
              Result.emplace_back(objudat.mIndex[e]);
            }
          }
        });
  };
}

Physics::WORKINDI Physics::ProcCollide(Object &obj, Object &obj2,
                                       DirectX::XMFLOAT3 &objpos,
                                       DirectX::XMFLOAT3 &obj2pos,
                                       DirectX::XMFLOAT4 &dir, float &dist) {
  using namespace DirectX;
  XMFLOAT3 pos2;
  XMStoreFloat3(&pos2, XMLoadFloat4(&dir) * dist);

  auto &objudat = *((PhysicsObject&)obj).model->uData;
  auto &obj2udat = *((PhysicsObject&)obj2).model->uData;
  auto &objbdata = objudat.bdata;
  auto &obj2bdata = obj2udat.bdata;
  auto &MappedVert1 = objudat.MappedVertices;
  auto &MappedVert2 = obj2udat.MappedVertices;

  std::vector<std::array<uint16_t, 2>> WorkData;
  //WorkData.reserve(obj2bdata.size() * objbdata.size());

  for (auto &b2 : obj2bdata) {
    BoundingSphere sph2 = b2.sphere;
    XMStoreFloat3(&sph2.Center,
                  XMLoadFloat3(&sph2.Center) + XMLoadFloat3(&pos2));
    for (auto &b : objbdata) {
      if (b.sphere.Intersects(sph2)) {
        WorkData.emplace_back(std::array<uint16_t, 2>{b.bIndex, b2.bIndex});
      }
    }
  }
  WORKINDI Result;
  if (WorkData.size() == 0)
    return Result;
  Result.Indices.reserve(MappedVert1.size() + MappedVert2.size());

  std::vector<std::vector<int>> bdata1Indices(WorkData.size());
  std::vector<std::vector<int>> bdata2Indices(WorkData.size());
  std::vector<UINT> offset1(WorkData.size());
  std::vector<UINT> offset2(WorkData.size());
  std::vector<std::atomic<bool>> objCheck1(MappedVert1.size());
  std::vector<std::atomic<bool>> objCheck2(MappedVert2.size());
  
  std::vector<THREADS::WRef> refs1;
  std::vector<THREADS::WRef> refs2;

  for (auto WDIndex = 0; WDIndex < WorkData.size(); WDIndex++) {
    std::array<DirectX::BoundingSphere, 2> CollSp;
    using namespace DirectX;
    CollSp[0] = objudat.bdata[WorkData[WDIndex][0]].sphere;
    CollSp[1] = obj2udat.bdata[WorkData[WDIndex][1]].sphere;
    XMStoreFloat3(&CollSp[0].Center,
                  XMLoadFloat3(&CollSp[0].Center) - XMLoadFloat3(&pos2));

    XMStoreFloat3(&CollSp[1].Center,
                  XMLoadFloat3(&CollSp[1].Center) + XMLoadFloat3(&pos2));

    auto &W1Indices = bdata1Indices[WDIndex];
    auto &W2Indices = bdata2Indices[WDIndex];
    refs1.emplace_back(tMain->gPushWork(
        CheckVertexDirection(W1Indices, objudat, objCheck1,
                             WorkData[WDIndex][0], CollSp[1], MappedVert1)
                            ));
    refs2.emplace_back(tMain->gPushWork(
        CheckVertexDirection(W2Indices, obj2udat, objCheck2,
                             WorkData[WDIndex][1], CollSp[0], MappedVert2)
                            ));
  }
  std::vector<int> objind(MappedVert1.size(), 0);
  std::vector<int> obj2ind(MappedVert2.size(), 0);
  std::vector<int> WorkIndi1;
  std::vector<int> WorkIndi2;

  WorkIndi1.reserve(MappedVert1.size());
  WorkIndi2.reserve(MappedVert2.size());
  tMain->gEndWork(refs1);
  tMain->gEndWork(refs2);
  for (auto& indices : bdata1Indices) {
    WorkIndi1.append_range(indices);
  }
  for (auto& indices : bdata2Indices) {
    WorkIndi2.append_range(indices);
  }

  Result.Position = pos2;
  Result.Indices = WorkIndi1;
  Result.wWorkCount = WorkIndi1.size();
  Result.Indices.append_range(WorkIndi2);
  Result.tWorkCount = WorkIndi2.size();
  return Result;
}

//rewrite to only process loaded and tracked models
void Physics::pCollison(umID instanceID) {
  auto &tInstance = Tracker.getInstance(instanceID);
  std::list<Object *> ObjectsToProcess = tInstance.GetPhysicsObjects();
  std::vector<THREADS::WRef> refs;

  for (auto &tO0 : ObjectsToProcess) {
    auto tO = (PhysicsObject *)tO0;
    refs.emplace_back(tMain->gPushWork([this, tO, &ObjectsToProcess]() {
      using namespace DirectX;
      auto &obj = *(PhysicsObject *)tO;
      if (!obj.loadedModel)
        return;
      XMFLOAT3 Pos1 = obj.mPos.Get();
      auto sph1 = obj.model->uData->Sphere;

      std::vector<collstruct> wCollModels;
      std::vector<collstruct> LCollModels;
      LCollModels.reserve(ObjectsToProcess.size());

      for (auto &tO20 : ObjectsToProcess) {
        auto tO2 = (PhysicsObject *)tO20;
        if (*tO2 != obj) {
          if (!tO2->loadedModel)
            break;
          XMFLOAT3 Pos2 = tO2->mPos.Get();
          auto sph2 = tO2->model->uData->Sphere;
          auto tdist = fDirection(Pos1, Pos2);
          XMStoreFloat3(&sph2.Center,
                        XMLoadFloat4(&tdist) * fDistance(Pos1, Pos2));
          if (sph2.Intersects(sph1)) {
            LCollModels.emplace_back(collstruct(&obj, tO2));
          }
        }
      }
      std::vector<bool> lCheck(LCollModels.size(), true);
      cmMtx.lock();
      if (CollModels.size() != 0) {
        std::for_each(CollModels.begin(), CollModels.end(),
                      [&lCheck, &LCollModels](auto &c) {
                        for (auto l = 0; l < lCheck.size(); l++) {
                          if (lCheck[l]) {
                            if (LCollModels[l] == c)
                              lCheck[l] = false;
                          }
                        }
                      });
        for (auto i = 0; i < lCheck.size(); i++) {
          if (lCheck[i]) {
            CollModels.emplace_back(LCollModels[i]);
            wCollModels.emplace_back(LCollModels[i]);
          }
        }
      } else {
        CollModels.append_range(LCollModels);
        wCollModels = LCollModels;
      }
      cmMtx.unlock();

      std::vector<THREADS::WRef> refs;
      refs.resize(wCollModels.size());

      for (auto i = 0; i < wCollModels.size(); i++) {

        refs[i] = tMain->gPushWork([this, &wCollModels, i, &obj, &Pos1]() {
          std::array<int, 2> wSize = {0, 0};

          XMFLOAT3 MoveD1{0, 0, 0};
          XMFLOAT3 MoveD2{0, 0, 0};
          XMFLOAT3 Zero{0, 0, 0};
          auto tQueue = cl::CommandQueue{context, devices.front()};
          auto &obj2 = *(PhysicsObject *)wCollModels[i].obj2;
          XMFLOAT3 Pos2 = obj2.mPos.Get();
          BoundingSphere sph2;
          sph2 = obj2.model->uData->Sphere;
          auto dir = fDirection(Pos1, Pos2);
          auto dist = fDistance(Pos1, Pos2);

          WORKINDI WorkIndi = ProcCollide(obj, obj2, Pos1, Pos2, dir, dist);

          if (WorkIndi.wWorkCount != 0 && WorkIndi.tWorkCount != 0) {

            wSize[0] = WorkIndi.wWorkCount;
            wSize[1] = WorkIndi.tWorkCount;
            std::vector<RETURNDATA> retdata(wSize[0]);
            cl::Buffer PositionBuffer(context, CL_MEM_READ_ONLY,
                                      sizeof(XMFLOAT3));
            cl::Buffer IndexBuffer(context, CL_MEM_READ_ONLY,
                                   WorkIndi.Indices.size() * sizeof(int));

            cl::Kernel kerns(FullColl, "coll");

            tQueue.enqueueWriteBuffer(PositionBuffer, CL_FALSE, 0,
                                      sizeof(XMFLOAT3), &WorkIndi.Position);
            tQueue.enqueueWriteBuffer(IndexBuffer, CL_FALSE, 0,
                                      WorkIndi.Indices.size() * sizeof(cl_int),
                                      WorkIndi.Indices.data());
            size_t retdatSize = sizeof(RETURNDATA) * retdata.size();
            cl::Buffer rbuffer(context, CL_MEM_READ_WRITE, retdatSize);
            cl_float fillbuff = 0.0;
            tQueue.enqueueFillBuffer(rbuffer, fillbuff, 0, retdatSize);
            tQueue.finish();

            kerns.setArg(0, obj.model->clBuff);
            kerns.setArg(1, obj2.model->clBuff);
            kerns.setArg(2, PositionBuffer);
            kerns.setArg(3, IndexBuffer);
            kerns.setArg(4, rbuffer);

            size_t wsize[2] = {(size_t)WorkIndi.wWorkCount,
                               (size_t)WorkIndi.tWorkCount};
            size_t offsize[2] = {0, (size_t)WorkIndi.wWorkCount};
            auto error = CL_SUCCESS;

            error =
                clEnqueueNDRangeKernel(tQueue.get(), kerns.get(), 2, offsize,
                                       wsize, nullptr, 0, NULL, NULL);
            assert(error == CL_SUCCESS);

            tQueue.finish();
            error = tQueue.enqueueReadBuffer(
                rbuffer, CL_TRUE, 0, sizeof(RETURNDATA) * retdata.size(),
                retdata.data());
            assert(error == CL_SUCCESS);
            tQueue.finish();

            auto dir = fDirection(Pos1, Pos2);
            auto dist = fDistance(Pos1, Pos2);
            XMFLOAT3 RelPos;
            XMStoreFloat3(&RelPos, XMLoadFloat4(&dir) * dist);

            int coutn = 0;
            for (auto &r : retdata) {
              if (r.dist[0] != 0 && r.dist[1] != 0) {
                // XMFLOAT3 isectDir;

                // XMStoreFloat3(&isectDir,XMVector3Cross(XMLoadFloat3(&obj.model->uData->MappedVertices[r.index[0]][0].normal),
                // XMLoadFloat3(&obj2.model->uData->MappedVertices[r.index[1]][0].normal)));

                XMStoreFloat3(
                    &MoveD1, (XMLoadFloat3(&MoveD1) +
                              (XMLoadFloat3(&obj.model->uData
                                                 ->MappedVertices[r.index[0]][0]
                                                 .normal) *
                               r.dist[0])));
                XMStoreFloat3(
                    &MoveD2, (XMLoadFloat3(&MoveD2) +
                              (XMLoadFloat3(&obj2.model->uData
                                                 ->MappedVertices[r.index[1]][0]
                                                 .normal) *
                               r.dist[1])));

                /*
                                        DebugMTX.lock();
                std::cout << "Model: " + obj.model->name + " Vert: " +
                std::to_string(r.index[0]) + " Pos: {" +
                std::to_string(obj.model->uData->MappedVertices[r.index[0]][r.index[0]
                % 3].position.x) + ", " +
                    std::to_string(obj.model->uData->MappedVertices[r.index[0]][r.index[0]
                % 3].position.y) +", " +
                std::to_string(obj.model->uData->MappedVertices[r.index[0]][r.index[0]
                % 3].position.z) + "} " << std::endl; std::cout << "Normal: {" +
                std::to_string(obj.model->uData->MappedVertices[r.index[0]][0].normal.x)
                + ", " +
                    std::to_string(obj.model->uData->MappedVertices[r.index[0]][0].normal.y)
                + ", " +
                std::to_string(obj.model->uData->MappedVertices[r.index[0]][0].normal.z)
                + "} " << std::endl; auto objvert0 =
                obj.model->uData->MappedVertices[r.index[0]][0].position; auto
                objvert1 =
                obj.model->uData->MappedVertices[r.index[0]][1].position; auto
                objvert2 =
                obj.model->uData->MappedVertices[r.index[0]][2].position;

                std::cout << "Mod1 Vert0 pos': {" + std::to_string(objvert0.x) +
                ", " + std::to_string(objvert0.y) + ", " +
                std::to_string(objvert0.z) + "} " << std::endl; std::cout <<
                "Mod1 Vert0 pos': {" + std::to_string(objvert1.x) + ", " +
                    std::to_string(objvert1.y) + ", " +
                std::to_string(objvert1.z) + "} " << std::endl; std::cout <<
                "Mod1 Vert0 pos': {" + std::to_string(objvert2.x) + ", " +
                    std::to_string(objvert2.y) + ", " +
                std::to_string(objvert2.z) + "} " << std::endl;

                std::cout << "Model2: " + obj2.model->name + " Vert: " +
                std::to_string(r.index[1]) + " Pos: {" +
                std::to_string(obj2.model->uData->MappedVertices[r.index[1]][r.index[1]
                % 3].position.x) + ", " +
                    std::to_string(obj2.model->uData->MappedVertices[r.index[1]][r.index[1]
                % 3].position.y) + ", " +
                std::to_string(obj2.model->uData->MappedVertices[r.index[1]][r.index[1]
                % 3].position.z) + "} " << std::endl; auto obj2vert0 =
                obj2.model->uData->MappedVertices[r.index[1]][0].position; auto
                obj2vert1 =
                obj2.model->uData->MappedVertices[r.index[1]][1].position; auto
                obj2vert2 =
                obj2.model->uData->MappedVertices[r.index[1]][2].position;

                std::cout << "Mod2 Vert0 pos': {" + std::to_string(obj2vert0.x)
                + ", " + std::to_string(obj2vert0.y) + ", " +
                std::to_string(obj2vert0.z) + "} " << std::endl; std::cout <<
                "Mod2 Vert0 pos': {" + std::to_string(obj2vert1.x) + ", " +
                    std::to_string(obj2vert1.y) + ", " +
                std::to_string(obj2vert1.z) + "} " << std::endl; std::cout <<
                "Mod2 Vert0 pos': {" + std::to_string(obj2vert2.x) + ", " +
                    std::to_string(obj2vert2.y) + ", " +
                std::to_string(obj2vert2.z) + "} " << std::endl; std::cout <<
                "Mod 2 rel pos: {" + std::to_string(RelPos.x) + ", " +
                    std::to_string(RelPos.y) + ", " + std::to_string(RelPos.z) +
                "} " << std::endl; std::cout << "Normal: {" +
                std::to_string(obj2.model->uData->MappedVertices[r.index[1]][0].normal.x)
                + ", " +
                    std::to_string(obj2.model->uData->MappedVertices[r.index[1]][0].normal.y)
                + ", " +
                std::to_string(obj2.model->uData->MappedVertices[r.index[1]][0].normal.z)
                + "} " << std::endl;



                DebugMTX.unlock();
                */

                coutn++;
              }
            }
            if (coutn != 0) {
              float massScal1 = (obj.mass / (obj.mass + obj2.mass));
              float massScal2 = (obj2.mass / (obj.mass + obj2.mass));
              massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
              massScal2 = massScal2 < 0.00001 ? 0 : massScal2;

              XMStoreFloat3(&MoveD1,
                            (XMLoadFloat3(&MoveD1) / (float)coutn) * massScal2);
              XMStoreFloat3(&MoveD2,
                            (XMLoadFloat3(&MoveD2) / (float)coutn) * massScal1);

              XMFLOAT3 Zero(0, 0, 0);

              // Fixxx thissss
              // CalProportionalSpeed(obj->velDir, obj2->velDir, obj->speed,
              // obj2.speed, obj->mass, obj2.mass);
              obj.Move(fDirection(Zero, MoveD2), fDistance(Zero, MoveD2));
              obj2.Move(fDirection(Zero, MoveD1), fDistance(Zero, MoveD1));
            }
          }
        });
      }
      tMain->gEndWork(refs);
    }));
  };
  tMain->gEndWork(refs);
}

void Physics::pSpecReset() {
    //Do stuff here to free memory when Collision calculations are over.
    cmMtx.lock();
    CollModels.resize(0);
    //CollModels.shrink_to_fit();
    cmMtx.unlock();
}




float Physics::fDistance(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2) {
    DirectX::XMFLOAT4 Result{ 0,0,0,0 };
    DirectX::XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    DirectX::XMStoreFloat4(&Result, DirectX::XMVectorSubtract(XMLoadFloat3(&pos2), XMLoadFloat3(&pos1)));
    DirectX::XMStoreFloat3(&Dist, DirectX::XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
    d = sqrt(Dist.x);
    return d;
}

DirectX::XMFLOAT4 Physics::fDirection(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2) {
    using namespace DirectX;
    DirectX::XMFLOAT4 Result{ 0,0,0,0 };
    DirectX::XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    DirectX::XMStoreFloat4(&Result, XMLoadFloat3(&pos2) - XMLoadFloat3(&pos1));
    XMStoreFloat3(&Dist, DirectX::XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
    d = sqrt(Dist.x);
    if (d > 0) {
        XMStoreFloat4(&Result, XMLoadFloat4(&Result) / d);
        return Result;
    }
    return { 0,0,0,0 };
}

