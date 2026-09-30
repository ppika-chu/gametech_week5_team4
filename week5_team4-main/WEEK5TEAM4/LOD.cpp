#include "LOD.h"
#include "Assets.h"
#include "Renderer.h"
#include "TMap.h"
#include "TArray.h"
#include <queue>
#include <utility>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <cstring>
#include <vector>

namespace
{
    // 위치(float 3개)를 비트 단위로 비교/해시한다.
    // seam 양쪽 정점은 같은 원본 위치(OBJ의 v)에서 나와서 비트까지 같다.
    static_assert(sizeof(FVector) == 12, "FVector에 패딩이 생기면 바이트 비교 깨짐");

    struct FPosHash
    {
        size_t operator()(const FVector& P) const
        {
            return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(&P), sizeof(FVector)));
        }
    };

    struct FPosEqual
    {
        bool operator()(const FVector& A, const FVector& B) const
        {
            return std::memcmp(&A, &B, sizeof(FVector)) == 0;
        }
    };
}

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

// bBorderLocked: 다른 섹션과 공유하는 위치의 정점.
//   섹션은 각자 따로 단순화되므로, 이 정점을 움직이거나 지우면 섹션 사이에 틈이 생긴다. 절대 건드리지 않는다.
// 같은 섹션 안의 UV/노말 seam(같은 위치에 있는 정점 복사본들)은 "위치 그룹"으로 묶어서,
//   복사본 전부를 각자 자기 UV 조각 안의 짝으로 동시에 합친다. 양쪽이 같은 점으로 가므로 틈이 생기지 않는다.
void SimplifySection(TArray<FVertex>& Verts, TArray<uint32>& Indices, const TArray<uint8>& bBorderLocked, int32 TargetTriCount)
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
    // seam 엣지도 UV 조각 기준으로는 경계라서 여기서 제약이 걸린다.
    // 덕분에 seam 선을 "따라가는" 축약은 싸고, seam 선을 "벗어나는" 축약은 비싸진다.
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

    // 1.7. 위치 그룹: 같은 위치에 있는 정점(seam 복사본)끼리 묶는다.
    // 그룹 하나 = 공간상의 점 하나. 축약은 그룹 단위로 해서 복사본이 따로 놀지 않게 한다.
    TArray<uint32> PosId;                   // 정점 -> 그룹 번호
    PosId.SetNum(VertCount);
    TArray<TArray<uint32>> GroupMembers;    // 그룹 -> 그 위치의 정점들
    TArray<uint8> bGroupLocked;             // 섹션 경계 정점이 섞여 있으면 1 (지울 수 없음, Keep으로만 가능)
    {
        std::unordered_map<FVector, uint32, FPosHash, FPosEqual> PosToGroup;
        PosToGroup.reserve(VertCount);

        for (int32 v = 0; v < VertCount; ++v)
        {
            auto [It, bInserted] = PosToGroup.try_emplace(Verts[v].Pos, static_cast<uint32>(GroupMembers.Num()));
            if (bInserted)
            {
                GroupMembers.Emplace();
                bGroupLocked.Add(0);
            }

            const uint32 G = It->second;
            PosId[v] = G;
            GroupMembers[G].Add(static_cast<uint32>(v));
            if (bBorderLocked[v]) bGroupLocked[G] = 1;
        }
    }
    const int32 GroupCount = GroupMembers.Num();

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
    TArray<uint32> bGroupAlive;       // Group Alive 여부
    bGroupAlive.SetNum(GroupCount);
    TArray<uint32> GroupVersion;      // Group Version 관리용 (그룹이 바뀌면 +1, 오래된 후보 무효화)
    GroupVersion.SetNum(GroupCount);

    for (int32 i = 0; i < TriCount; ++i) bTriAlive[i] = true;
    for (int32 i = 0; i < VertCount; ++i) bVertAlive[i] = true;
    for (int32 i = 0; i < GroupCount; ++i)
    {
        bGroupAlive[i] = true;
        GroupVersion[i] = 0;
    }

    // 3. 후보
    struct FCandidate
    {
        uint32 Remove, Keep;        // Remove 쪽 그룹을 Keep 쪽 그룹으로 합친다
        FVector Target;
        double Cost;
        uint32 VerRemove, VerKeep;
        bool bMoveKeep;             // true: 일반 정점끼리, Keep을 QEM 최적 위치로 옮긴다
                                    // false: 그룹 축약, Keep은 제자리 (seam/경계 위치 보존)
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

    // 복사본 없는 일반 정점: 섹션 경계도 seam도 아니라 자유롭게 옮겨도 된다.
    auto IsPlainGroup = [&](uint32 G) { return GroupMembers[G].Num() == 1 && !bGroupLocked[G]; };

    // 일반 정점끼리: 기존 QEM 방식 (Keep을 최적 위치로 옮긴다)
    auto MakeOptimalCandidate = [&](uint32 A, uint32 B) -> FCandidate
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
        return {B, A, Target, Cost, GroupVersion[PosId[B]], GroupVersion[PosId[A]], true};
    };

    // 그룹 축약: Remove 그룹의 복사본 전부를 Keep 그룹 위치로 합친다. Keep은 움직이지 않는다.
    // 비용은 두 그룹의 복사본 Quadric을 모두 더해서 계산한다 (seam 양쪽 면을 다 고려).
    auto MakeGroupCandidate = [&](uint32 RemoveV, uint32 KeepV) -> FCandidate
    {
        const uint32 GR = PosId[RemoveV], GK = PosId[KeepV];

        FQuadric Q;
        for (uint32 V : GroupMembers[GR]) Q = Q + Quadrics[V];
        for (uint32 V : GroupMembers[GK]) Q = Q + Quadrics[V];

        const FVector Target = Verts[KeepV].Pos;
        return {RemoveV, KeepV, Target, Q.Error(Target), GroupVersion[GR], GroupVersion[GK], false};
    };

    auto PushCandidates = [&](uint32 A, uint32 B)
    {
        const uint32 GA = PosId[A], GB = PosId[B];
        if (GA == GB) return;

        if (IsPlainGroup(GA) && IsPlainGroup(GB))
        {
            Heap.push(MakeOptimalCandidate(A, B));
            return;
        }

        // seam이나 섹션 경계가 끼면 한쪽 그룹을 다른 쪽 자리로 통째로 합친다.
        // 방향은 둘 다 후보로 넣고 비용이 싼 쪽이 먼저 뽑힌다. 섹션 경계 그룹은 지울 수 없다.
        if (!bGroupLocked[GA]) Heap.push(MakeGroupCandidate(A, B));
        if (!bGroupLocked[GB]) Heap.push(MakeGroupCandidate(B, A));
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
                PushCandidates(A, B);
        }
    }

    // RemoveIdx를 KeepIdx로 합친다: 인덱스 교체, 퇴화 삼각형 제거, Quadric 합산
    int32 CurrentTriCount = TriCount;
    auto RedirectVertex = [&](uint32 KeepIdx, uint32 RemoveIdx)
    {
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

        Quadrics[KeepIdx] = Quadrics[KeepIdx] + Quadrics[RemoveIdx];
        bVertAlive[RemoveIdx] = false;
    };

    // KeepIdx 주변이 바뀌었으니 이웃과의 후보를 다시 넣는다
    auto PushNeighbors = [&](uint32 KeepIdx)
    {
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
            if (bVertAlive[N]) PushCandidates(KeepIdx, N);
        }
    };

    // 4. 메인 루프
    constexpr uint32 NoPartner = 0xFFFFFFFFu;
    std::vector<std::pair<uint32, uint32>> Pairs;   // (Remove 복사본, 짝이 되는 Keep 복사본)

    while (!Heap.empty() && CurrentTriCount > TargetTriCount)
    {
        FCandidate Top = Heap.top();
        Heap.pop();

        const uint32 GR = PosId[Top.Remove], GK = PosId[Top.Keep];
        if (!bGroupAlive[GR] || !bGroupAlive[GK]) continue;
        if (Top.VerRemove != GroupVersion[GR] || Top.VerKeep != GroupVersion[GK]) continue;
        if (!bVertAlive[Top.Remove] || !bVertAlive[Top.Keep]) continue;

        if (Top.bMoveKeep)
        {
            // 일반 정점끼리 (기존 로직)
            uint32 KeepIdx = Top.Keep, RemoveIdx = Top.Remove;

            // 뒤집힘 검사 (뒤집혔으면 축약 제외)
            if (WouldFlip(KeepIdx, RemoveIdx, Top.Target) || WouldFlip(RemoveIdx, KeepIdx, Top.Target))
                continue;

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

            RedirectVertex(KeepIdx, RemoveIdx);
            Verts[KeepIdx].Pos = Top.Target;

            bGroupAlive[GR] = false;
            ++GroupVersion[GK];

            PushNeighbors(KeepIdx);
            continue;
        }

        // 그룹 축약
        // Remove 그룹의 복사본마다, 엣지로 연결된 Keep 그룹 복사본(= 같은 UV 조각 안의 짝)을 찾는다.
        // 짝이 없는 복사본이 하나라도 있으면 그 복사본만 제자리에 남아 틈이 생기므로 포기한다.
        Pairs.clear();
        bool bOk = true;
        for (uint32 P : GroupMembers[GR])
        {
            uint32 Partner = NoPartner;
            for (int32 TriIdx : VertTris[P])
            {
                if (!bTriAlive[TriIdx]) continue;
                for (int32 k = 0; k < 3; ++k)
                {
                    const uint32 V = Indices[TriIdx*3+k];
                    if (PosId[V] != GK) continue;

                    if (Partner == NoPartner) Partner = V;
                    else if (Partner != V) bOk = false;     // 짝이 둘 이상이면 어느 쪽으로 합칠지 모호
                }
            }

            if (Partner == NoPartner) bOk = false;
            if (!bOk) break;
            Pairs.push_back({P, Partner});
        }
        if (!bOk || Pairs.empty()) continue;

        // 뒤집힘 검사: 움직이는 건 Remove 복사본들뿐이다 (Keep은 제자리)
        bool bFlip = false;
        for (const auto& [P, K] : Pairs)
        {
            if (WouldFlip(P, K, Top.Target)) { bFlip = true; break; }
        }
        if (bFlip) continue;

        // Keep 복사본은 위치도 속성(UV 등)도 그대로라, seam 양쪽이 같은 점에 붙은 채로 남는다.
        for (const auto& [P, K] : Pairs)
        {
            RedirectVertex(K, P);
        }

        bGroupAlive[GR] = false;
        ++GroupVersion[GK];

        for (const auto& [P, K] : Pairs)
        {
            PushNeighbors(K);
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
    // 위치 -> 그 위치를 쓰는 섹션 수. 2 이상이면 섹션 경계다.
    // 섹션은 각자 따로 단순화되므로 경계 위치는 절대 건드리지 않는다.
    // (같은 섹션 안의 UV/노말 seam은 SimplifySection이 위치 그룹으로 처리한다)
    std::unordered_map<FVector, uint32, FPosHash, FPosEqual> PosSectionCount;
    for (const FStaticMeshSection& Section : SrcSections)
    {
        std::unordered_set<FVector, FPosHash, FPosEqual> SeenInSection;
        for (uint32 i = 0; i < Section.IndexCount; ++i)
        {
            const FVector& P = SrcVertices[SrcIndices[Section.FirstIndex + i]].Pos;
            if (SeenInSection.insert(P).second)
            {
                ++PosSectionCount[P];
            }
        }
    }

    FMeshLOD LOD;
    TArray<FVertex> OutVertices;
    TArray<uint32> OutIndices;

    for (const FStaticMeshSection& Section : SrcSections)
    {
        TMap<uint32, uint32> GlobalToLocal;
        TArray<FVertex> LocalVertices;
        TArray<uint32> LocalIndices;
        TArray<uint8> LocalBorderLocked;

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
                    LocalBorderLocked.Add(PosSectionCount.find(SrcVertices[GlobalIdx].Pos)->second > 1 ? 1 : 0);
                    GlobalToLocal.Add(GlobalIdx, LocalIdx);
                }
                LocalIndices.Add(LocalIdx);
            }
        }

        int32 TargetTriCount = (std::max)(1, static_cast<int32>(TriSection * TargetTriangleRatio));
        SimplifySection(LocalVertices, LocalIndices, LocalBorderLocked, TargetTriCount);

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
