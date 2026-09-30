#include "LOD.h"
#include "Assets.h"
#include "Renderer.h"
#include "TMap.h"
#include "TArray.h"
#include <queue>
#include <utility>
#include <set>
#include <map>

struct FQuadric
{
    double q[10] = {}; 

    // P는 평면 위의 점 N은 평면 위의 법선
    void AddPlane(const FVector& P, const FVector& N)
    {
        double a = N.x, b = N.y, c = N.z;
        double d = -(a*P.x + b*P.y + c*P.z);
        q[0] += a*a; q[1] += a*b; q[2] += a*c; q[3] += a*d;
        q[4] += b*b; q[5] += b*c; q[6] += b*d; q[7] += c*c;
        q[8] += c*d; q[9] += d*d;
    }

    FQuadric operator+(const FQuadric& Other) const
    {
        FQuadric R;
        for (int i = 0; i < 10; ++i) R.q[i] = q[i] + Other.q[i];
        return R;
    }

    double Error(const FVector& V) const
    {
        double x = V.x, y = V.y, z = V.z;
        return q[0]*x*x + 2*q[1]*x*y + 2*q[2]*x*z + 2*q[3]*x + q[4]*y*y + 2*q[5]*y*z + 2*q[6]*y + q[7]*z*z + 2*q[8]*z + q[9];
    }

    // 최적점 구하기
    bool SolveOptimalPosition(FVector& OutV) const
    {
        double a = q[0], b = q[1], c = q[2];
        double d = q[1], e = q[4], f = q[5];
        double g = q[2], h = q[5], i = q[7];

        double det = a*(e*i-f*h) - b*(d*i-f*g) + c*(d*h-e*g);

        // 특이행렬이면 false return
        if (fabs(det) < 1e-8) return false;

        double bx = -q[3], by = -q[6], bz = -q[8];

        double detX = bx*(e*i-f*h) - b*(by*i-bz*f) + c*(by*h-bz*e);
        double detY = a*(by*i-bz*f) - bx*(d*i-f*g) + c*(d*bz-by*g);
        double detZ = a*(e*bz-by*h) - b*(d*bz-by*g) + bx*(d*h-e*g);

        OutV.x = static_cast<float>(detX / det);
        OutV.y = static_cast<float>(detY / det);
        OutV.z = static_cast<float>(detZ / det);

        return true;
    }
};

void SimplifySection(TArray<FVertex>& Verts, TArray<uint32>& Indices, int32 TargetTriCount)
{
    const int32 VertCount = Verts.Num();
    int32 TriCount = Indices.Num() / 3;

    if (TriCount <= TargetTriCount) return;

    TArray<FQuadric> Quadrics;
    Quadrics.SetNum(VertCount);

    // 1. 정점 Quadric 초기화
    for (int32 t=0; t<TriCount; ++t)
    {
        int32 i0 = Indices[3*t], i1 = Indices[3*t+1], i2 = Indices[3*t+2];
        const FVector& P0 = Verts[i0].Pos;
        const FVector& P1 = Verts[i1].Pos;
        const FVector& P2 = Verts[i2].Pos;

        FVector Normal = FVector::cross(P1-P0, P2-P0);
        float Len = Normal.Length();
        if (Len < 1e-8f) continue;
        Normal *= (1 / Len);
        
        Quadrics[i0].AddPlane(P0, Normal);
        Quadrics[i1].AddPlane(P1, Normal);
        Quadrics[i2].AddPlane(P2, Normal);
    }
    
    // 1.5. 경계 엣지 탐지
    {
        std::map<std::pair<uint32,uint32>, int32> EdgeUseCount;
        for (int t = 0; t < TriCount; ++t)
        {
            uint32 Tri[3] = { Indices[t*3+0], Indices[t*3+1], Indices[t*3+2] };
            for (int32 k = 0; k < 3; ++k)
            {
                uint32 A = Tri[k], B = Tri[(k+1)%3];
                if (A>B) std::swap(A, B);
                ++EdgeUseCount[{A, B}];
            }
        }
        
        constexpr float BoundaryWeight = 100.0f;
        for (int t = 0; t < TriCount; ++t)
        {
            uint32 Tri[3] = { Indices[t*3+0], Indices[t*3+1], Indices[t*3+2] };
            const FVector P0 = Verts[Tri[0]].Pos, P1 = Verts[Tri[1]].Pos, P2 = Verts[Tri[2]].Pos;
            FVector FaceNormal = FVector::cross(P1-P0, P2-P0);
            float FaceLen = FaceNormal.Length();
            if (FaceLen < 1e-8f) continue;
            FaceNormal *= (1.0f/FaceLen);

            for (int32 k=0; k<3; ++k)
            {
                uint32 A = Tri[k], B = Tri[(k+1)%3];
                uint32 EdgeA = A, EdgeB = B;
                if (EdgeA>EdgeB) std::swap(EdgeA,EdgeB);
                if (EdgeUseCount[{EdgeA, EdgeB}]!=1) continue;
                
                const FVector& PA = Verts[A].Pos;
                const FVector& PB = Verts[B].Pos;
                FVector EdgeDir = PB - PA;
                float EdgeLen = EdgeDir.Length();
                if (EdgeLen < 1e-8f) continue;
                EdgeDir *= (1.0f/EdgeLen);

                FVector ConstraintNormal = FVector::cross(EdgeDir, FaceNormal);
                float ConstraintLen = ConstraintNormal.Length();
                if (ConstraintLen < 1e-8f) continue;
                ConstraintNormal *= (1.0f / ConstraintLen);

                // Quadrics에 가중치 넣어서 에러값 비싸게.
                FQuadric BoundaryQ;
                BoundaryQ.AddPlane(PA, ConstraintNormal);
                for (int32 i = 0; i < 10; ++i) BoundaryQ.q[i] *= BoundaryWeight;

                Quadrics[A] = Quadrics[A] + BoundaryQ;
                Quadrics[B] = Quadrics[B] + BoundaryQ;
            }
        }
    }

    // 2. 정점별 인접 삼각형 목록
    TArray<TArray<int32>> VertTris;
    VertTris.SetNum(VertCount);
    for (int32 i=0; i<TriCount; i++)
        for (int32 j=0; j<3; j++)
            VertTris[Indices[3*i+j]].Add(i);

    TArray<uint32> bTriAlive;         // Triangle Alive 여부
    bTriAlive.SetNum(TriCount);
    TArray<uint32> bVertAlive;        // Vertex Alive 여부
    bVertAlive.SetNum(VertCount);
    TArray<uint32> VertVersion;     // Vertex Version 관리용
    VertVersion.SetNum(VertCount);

    for (int32 i = 0; i < TriCount; ++i) bTriAlive[i] = true;    
    for (int32 i = 0; i < VertCount; ++i)
    {
        bVertAlive[i] = true;
        VertVersion[i] = 0;
    }

    // 3. 후보
    struct FCandidate
    {
        uint32 A, B;
        FVector Target;
        double Cost;
        uint32 VerA, VerB;
    };

    auto Cmp = [](const FCandidate& L, const FCandidate& R) { return L.Cost > R.Cost; };

    std::priority_queue<FCandidate, std::vector<FCandidate>, decltype(Cmp)> Heap(Cmp);

    // MovigVertex가 NewPos로 옮겨질 때
    // 그 정점 주변 삼각형의 법선이 뒤집히는지 검사.
    auto WouldFlip = [&](uint32 MovingVertex, uint32 OtherVertex, const FVector& NewPos) -> bool
    {

        // 움직일 정점과 인접한 삼각형 순회하기
        for (int32 TriIdx : VertTris[MovingVertex])
        {
            if (!bTriAlive[TriIdx]) continue;
            uint32 Tri[3] = { Indices[TriIdx*3+0], Indices[TriIdx*3+1], Indices[TriIdx*3+2] };

            // 없어질 정점이면 검사 pass
            if (Tri[0]==OtherVertex || Tri[1]==OtherVertex || Tri[2]==OtherVertex) continue;


            // 기존 삼각형의 법선 벡터 구하기
            const FVector P0 = Verts[Tri[0]].Pos, P1 = Verts[Tri[1]].Pos, P2 = Verts[Tri[2]].Pos;
            FVector OldNormal = FVector::cross(P1-P0, P2-P0);
            const float OldLenSq = OldNormal.LengthSquared();

            if (OldLenSq < 1e-8f) continue;

            // 새로운 삼각형의 법선 벡터 구하기
            const FVector NP0 = (Tri[0] == MovingVertex) ? NewPos : P0;
            const FVector NP1 = (Tri[1] == MovingVertex) ? NewPos : P1;
            const FVector NP2 = (Tri[2] == MovingVertex) ? NewPos : P2;

            FVector NewNormal = FVector::cross(NP1-NP0, NP2-NP0);
            const float NewLenSq = NewNormal.LengthSquared();

            if (NewLenSq < 1e-8f) continue;

            // 원래 삼각형과 새로운 삼각형의 내적으로 음수면 뒤집힌 거임.
            // 0이면 꺾인 거인데, Threshold 0.2로 두어서 거의 납작해도 뒤집힌 거로 판정.
            const float Dot = FVector::dot(OldNormal, NewNormal);
            
            // 제곱근 연산은 비싸므로 제곱으로 처리
            if (Dot < 0.0f || Dot * Dot < 0.02f * OldLenSq * NewLenSq)  return true;
        }
        return false;
    };

    auto MakeCandidate = [&](uint32 A, uint32 B) -> FCandidate
    {
        if (A>B) std::swap(A, B);
        FQuadric Q = Quadrics[A] + Quadrics[B];

        const FVector Mid = (Verts[A].Pos + Verts[B].Pos) * 0.5f;
        const float EdgeLenSq = (Verts[A].Pos - Verts[B].Pos).LengthSquared();
        
        FVector Target;
        if (!Q.SolveOptimalPosition(Target))
            Target = Mid;
        else if ((Target - Mid).LengthSquared() > EdgeLenSq * 4.0f)
            Target = Mid;

        double Cost = Q.Error(Target);
        return {A, B, Target, Cost, VertVersion[A], VertVersion[B]};
    };

    std::set<std::pair<uint32, uint32>> SeenEdges;

    for (int32 t =  0; t < TriCount; ++t)
    {
        uint32 Tri[3] = { Indices[t*3+0], Indices[t*3+1], Indices[t*3+2] };

        for (int32 k = 0; k < 3; ++k)
        {
            uint32 A = Tri[k], B = Tri[(k+1)%3];
            if (A>B) std::swap(A, B);
            if (SeenEdges.insert({A,B}).second)
                Heap.push(MakeCandidate(A, B));
        }
    }

    // 4. 메인 루프
    int32 CurrentTriCount = TriCount;
    while (!Heap.empty() && CurrentTriCount > TargetTriCount)
    {
        FCandidate Top = Heap.top();
        Heap.pop();

        if(Top.VerA != VertVersion[Top.A] || Top.VerB != VertVersion[Top.B]) continue;
        if(!bVertAlive[Top.A] || !bVertAlive[Top.B]) continue;

        // 뒤집힘 검사 (뒤집혔으면 축약 제외)
        if (WouldFlip(Top.A, Top.B, Top.Target) || WouldFlip(Top.B, Top.A, Top.Target)) 
            continue;

        uint32 KeepIdx = Top.A, RemoveIdx = Top.B;

        // UV / Normal / Color 보간
        // Target의 원래 위치(비율) 구해서 그 비율로 속성 mix
        {
            const FVector OldA = Verts[KeepIdx].Pos;
            const FVector OldB = Verts[RemoveIdx].Pos;
            const FVector AB = OldB - OldA;
            const float LenSq = AB.LengthSquared();

            float T = 0.5f;
            if (LenSq > 1e-12f)
            {
                // Target을 AB에 투영하여 위치 확인
                T = FVector::dot(Top.Target - OldA, AB) / LenSq;
                T = FMath::Max(0.0f, FMath::Min(1.0f, T));  // Clamp
            }

            Verts[KeepIdx].Normal = Verts[KeepIdx].Normal * (1.0f - T) + Verts[RemoveIdx].Normal * T;
            Verts[KeepIdx].Normal.Normalize();
            Verts[KeepIdx].Color = Verts[KeepIdx].Color * (1.0f - T) + Verts[RemoveIdx].Color * T;
            Verts[KeepIdx].Tex = Verts[KeepIdx].Tex * (1.0f - T) + Verts[RemoveIdx].Tex * T;
        }

        for (uint32 TriIdx : VertTris[RemoveIdx])
        {
            if (!bTriAlive[TriIdx]) continue;
            uint32* Tri = &Indices[TriIdx*3];
            bool bTouchesKeep = (Tri[0]==KeepIdx || Tri[1]==KeepIdx || Tri[2]==KeepIdx);
            for (int32 k = 0; k < 3; ++k)
                if (Tri[k]==RemoveIdx) Tri[k] = KeepIdx;

            if (Tri[0]==Tri[1] || Tri[1]==Tri[2] || Tri[2]==Tri[0])
            {
                bTriAlive[TriIdx] = false;
                --CurrentTriCount;
            }
            else if (!bTouchesKeep)
            {
                VertTris[KeepIdx].Add(TriIdx);
            }
        }

        Verts[KeepIdx].Pos = Top.Target;
        Quadrics[KeepIdx] = Quadrics[KeepIdx] + Quadrics[RemoveIdx];
        bVertAlive[RemoveIdx] = false;
        ++VertVersion[KeepIdx];

        std::set<uint32> Neighbors;
        for (uint32 TriIdx : VertTris[KeepIdx])
        {
            if (!bTriAlive[TriIdx]) continue;
            uint32* Tri = &Indices[TriIdx*3];
            for (int32 k = 0; k<3; ++k)
                if (Tri[k] != KeepIdx) Neighbors.insert(Tri[k]);
        }

        for (uint32 N : Neighbors)
        {
            if (bVertAlive[N]) Heap.push(MakeCandidate(KeepIdx, N));
        }
        }

        // 5. Alive Tri만 다시 저장
        TArray<uint32> FinalIndices;
        for (int32 t=0; t<TriCount; ++t)
        {
            if (bTriAlive[t])
            {
                FinalIndices.Add(Indices[t*3+0]);
                FinalIndices.Add(Indices[t*3+1]);
                FinalIndices.Add(Indices[t*3+2]);
            }
        }
        Indices = std::move(FinalIndices);
}

FMeshLOD LOD::BuildQEMLOD(const TArray<FVertex>& SrcVertices,
                     const TArray<uint32>& SrcIndices,
                     const TArray<FStaticMeshSection>& SrcSections,
                     float TargetTriangleRatio, URenderer& InRenderer)
{
    FMeshLOD LOD;
    TArray<FVertex> OutVertices;
    TArray<uint32> OutIndices;

    for (const FStaticMeshSection& Section : SrcSections)
    {
        TMap<uint32, uint32> GlobalToLocal;
        TArray<FVertex> LocalVertices;
        TArray<uint32> LocalIndices;

        uint32 TriSection = Section.IndexCount / 3;
        for (uint32 i = 0; i<TriSection; ++i)
        {
            for (int j = 0; j<3; ++j)
            {
                uint32 GlobalIdx = SrcIndices[Section.FirstIndex + i*3 + j];
                uint32* Found = GlobalToLocal.Find(GlobalIdx);
                uint32 LocalIdx;

                if (Found) LocalIdx = *Found;
                else
                {
                    LocalIdx = static_cast<uint32>(LocalVertices.Num());
                    LocalVertices.Add(SrcVertices[GlobalIdx]);
                    GlobalToLocal.Add(GlobalIdx, LocalIdx);
                }
                LocalIndices.Add(LocalIdx);
            }
        }

        int32 TargetTriCount = (std::max)(1, static_cast<int32>(TriSection * TargetTriangleRatio));
        SimplifySection(LocalVertices, LocalIndices, TargetTriCount);

        FStaticMeshSection NewSection = Section;

        NewSection.FirstIndex = OutIndices.Num();
        const uint32 BaseVertex = OutVertices.Num();

        for (const FVertex& V : LocalVertices) OutVertices.Add(V);  
        for (uint32 Idx : LocalIndices) OutIndices.Add(BaseVertex + Idx);

        NewSection.IndexCount = OutIndices.Num() - NewSection.FirstIndex;
        
        LOD.Sections.Add(NewSection);
        
    }

    LOD.Vertices = OutVertices;
    LOD.VertexBuffer = InRenderer.CreateVertexBuffer(OutVertices.Data(), OutVertices.Num());
    LOD.IndexBuffer = InRenderer.CreateIndexBuffer(OutIndices.Data(), OutIndices.Num());
    
    return LOD;
}