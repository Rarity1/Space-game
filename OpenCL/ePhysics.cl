typedef struct XMFLOAT4{
    float x;
    float y;
    float z;
    float w;
}XMFLOAT4;

typedef struct XMFLOAT3{
    float x;
    float y;
    float z;
}XMFLOAT3;

typedef struct Tri{
    XMFLOAT3 one;
    XMFLOAT3 two;
    XMFLOAT3 thr;
}Tri;

typedef struct WORKDATA{
	XMFLOAT4 dir;
    float dist;
    int Index;
    int Index2;
}WORKDATA;

typedef struct MODEL{
	XMFLOAT3* vects;
}MODEL;

typedef struct RETURNDATA{
    int Index;
    int Index2;
    bool coll;
}RETURNDATA;

void kernel coll(global const WORKDATA* data, global const MODEL* WModel, global const MODEL* TModel, global RETURNDATA* retdat){
    int id = get_global_id(0);
}