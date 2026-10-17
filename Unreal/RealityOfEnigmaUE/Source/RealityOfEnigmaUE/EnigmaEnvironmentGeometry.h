#pragma once
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

namespace Scenery {
// Shared mesh authoring primitives; terrain and organic surfaces carry smooth normals and explicit UVs.
struct Mesh {
 TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;TArray<FLinearColor> C;TArray<FProcMeshTangent> Tangents;
 void FinishForUnreal(){for(int I=0;I<T.Num();I+=3)Swap(T[I+1],T[I+2]);}
 int FacingErrors() const {int Errors=0;for(int I=0;I<T.Num();I+=3){FVector Cross=FVector::CrossProduct(V[T[I+1]]-V[T[I]],V[T[I+2]]-V[T[I]]);FVector Normal=N[T[I]]+N[T[I+1]]+N[T[I+2]];if(FVector::DotProduct(Cross,Normal)>0.01)Errors++;}return Errors;}
 void Vertex(FVector P,FVector Normal,FVector2D Tex,FVector Tangent=FVector::XAxisVector){V.Add(P);N.Add(Normal);UV.Add(Tex);Tangents.Add(FProcMeshTangent(Tangent,false));}
 void Tri(FVector A,FVector B,FVector D){
  int I=V.Num();FVector Normal=FVector::CrossProduct(B-A,D-A).GetSafeNormal();
  bool X=FMath::Abs(Normal.X)>FMath::Abs(Normal.Z)&&FMath::Abs(Normal.X)>FMath::Abs(Normal.Y),Z=FMath::Abs(Normal.Z)>=FMath::Abs(Normal.X)&&FMath::Abs(Normal.Z)>=FMath::Abs(Normal.Y);
  FVector Tangent=X?FVector::YAxisVector:FVector::XAxisVector;Tangent=(Tangent-Normal*FVector::DotProduct(Tangent,Normal)).GetSafeNormal();
  for(auto P:{A,B,D})Vertex(P,Normal,(Z?FVector2D(P.X,P.Y):X?FVector2D(P.Y,P.Z):FVector2D(P.X,P.Z))*.002,Tangent);
  T.Append({I,I+1,I+2});
 }
 void Quad(FVector A,FVector B,FVector D,FVector E){Tri(A,B,D);Tri(A,D,E);}
 void Box(FVector P,FVector Size,FRotator R=FRotator::ZeroRotator){
  FVector Q[8];for(int I=0;I<8;I++)Q[I]=P+R.RotateVector(FVector(I&1?Size.X:-Size.X,I&2?Size.Y:-Size.Y,I&4?Size.Z:-Size.Z)*.5);
  Quad(Q[0],Q[2],Q[3],Q[1]);Quad(Q[4],Q[5],Q[7],Q[6]);Quad(Q[0],Q[1],Q[5],Q[4]);Quad(Q[2],Q[6],Q[7],Q[3]);Quad(Q[0],Q[4],Q[6],Q[2]);Quad(Q[1],Q[3],Q[7],Q[5]);
 }
 void Tube(FVector A,FVector B,float R0,float R1,int Sides=12){
  FVector Axis=(B-A).GetSafeNormal(),U,W;Axis.FindBestAxisVectors(U,W);W=-W;int Start=V.Num();float Length=(B-A).Size();
  for(int J=0;J<2;J++)for(int I=0;I<=Sides;I++){float Angle=I*2*PI/Sides;FVector D=U*FMath::Cos(Angle)+W*FMath::Sin(Angle);Vertex((J?B:A)+D*(J?R1:R0),(D+Axis*((R0-R1)/FMath::Max(Length,1.f))).GetSafeNormal(),FVector2D(I*1.f/Sides,J*Length*.005f),-U*FMath::Sin(Angle)+W*FMath::Cos(Angle));}
  for(int I=0;I<Sides;I++){int K=Start+I,L=K+Sides+1;T.Append({K,K+1,L+1,K,L+1,L});}
  for(int I=0;I<Sides;I++){float X=I*2*PI/Sides,Y=(I+1)*2*PI/Sides;Tri(A,A+(U*FMath::Cos(Y)+W*FMath::Sin(Y))*R0,A+(U*FMath::Cos(X)+W*FMath::Sin(X))*R0);Tri(B,B+(U*FMath::Cos(X)+W*FMath::Sin(X))*R1,B+(U*FMath::Cos(Y)+W*FMath::Sin(Y))*R1);}
 }
 void Arch(FVector P,float Radius,float Width,float Depth,float Start=0,float End=PI,int Segments=36){
  for(int I=0;I<Segments;I++){float A=FMath::Lerp(Start,End,I*1.f/Segments),B=FMath::Lerp(Start,End,(I+1.f)/Segments);FVector Q[8];
   for(int J=0;J<8;J++){float Angle=J&1?B:A,R=J&2?Radius+Width:Radius;Q[J]=P+FVector(FMath::Cos(Angle)*R,J&4?Depth*.5:-Depth*.5,FMath::Sin(Angle)*R);}
   Quad(Q[0],Q[1],Q[3],Q[2]);Quad(Q[4],Q[6],Q[7],Q[5]);Quad(Q[0],Q[4],Q[5],Q[1]);Quad(Q[2],Q[3],Q[7],Q[6]);Quad(Q[0],Q[2],Q[6],Q[4]);Quad(Q[1],Q[5],Q[7],Q[3]);}
 }
 void Vine(const TArray<FVector>& Points,float R0,float R1,int Sides=12){
  int Start=V.Num();FVector PreviousU=FVector::ZeroVector;for(int J=0;J<Points.Num();J++){
   FVector Axis=(Points[FMath::Min(J+1,Points.Num()-1)]-Points[FMath::Max(J-1,0)]).GetSafeNormal(),U,W;Axis.FindBestAxisVectors(U,W);
   if(J>0){FVector Transported=PreviousU-Axis*FVector::DotProduct(PreviousU,Axis);if(!Transported.IsNearlyZero())U=Transported.GetSafeNormal();}
   W=FVector::CrossProduct(Axis,U);PreviousU=U;
   float Radius=FMath::Lerp(R0,R1,J*1.f/(Points.Num()-1));
   for(int I=0;I<=Sides;I++){float A=I*2*PI/Sides;FVector D=U*FMath::Cos(A)+W*FMath::Sin(A);Vertex(Points[J]+D*Radius,D,FVector2D(I*1.f/Sides,J*.25f),-U*FMath::Sin(A)+W*FMath::Cos(A));}
  }
  for(int J=0;J<Points.Num()-1;J++)for(int I=0;I<Sides;I++){int K=Start+J*(Sides+1)+I,L=K+Sides+1;T.Append({K,K+1,L+1,K,L+1,L});}
 }
 void Ring(FVector P,float Radius,float Width,float Height,int Segments=72){
  for(int I=0;I<Segments;I++){float A=I*2*PI/Segments,B=(I+1)*2*PI/Segments;FVector Q[8];for(int J=0;J<8;J++){float Angle=J&1?B:A,R=J&2?Radius+Width:Radius;Q[J]=P+FVector(FMath::Cos(Angle)*R,FMath::Sin(Angle)*R,J&4?Height:0);}
   Quad(Q[4],Q[5],Q[7],Q[6]);Quad(Q[0],Q[4],Q[6],Q[2]);Quad(Q[1],Q[3],Q[7],Q[5]);Quad(Q[2],Q[6],Q[7],Q[3]);}
 }
 void Lathe(FVector P,const TArray<FVector2D>& Profile,int Sides=24){
  for(int J=1;J<Profile.Num();J++)for(int I=0;I<Sides;I++){float A=I*2*PI/Sides,B=(I+1)*2*PI/Sides;auto At=[&](FVector2D Q,float Angle){return P+FVector(FMath::Cos(Angle)*Q.X,FMath::Sin(Angle)*Q.X,Q.Y);};Quad(At(Profile[J-1],A),At(Profile[J-1],B),At(Profile[J],B),At(Profile[J],A));}
 }
 void Rock(FVector P,FVector Size,int Seed){
  FRandomStream Rand(Seed);constexpr int S=14;FVector R[5][S];
  for(int J=0;J<5;J++)for(int I=0;I<S;I++){float A=I*2*PI/S,Scale=Rand.FRandRange(.85f,1.12f)*(J==4?.38f:J==0?.85f:1.f);R[J][I]=P+FVector(FMath::Cos(A)*Size.X*Scale,FMath::Sin(A)*Size.Y*Scale,Size.Z*J*.25+Rand.FRandRange(-.045f,.045f)*Size.Z);}
  for(int I=0;I<S;I++){int K=(I+1)%S;for(int J=0;J<4;J++)Quad(R[J][I],R[J][K],R[J+1][K],R[J+1][I]);Tri(P+FVector(0,0,Size.Z),R[4][I],R[4][K]);}
 }
 void Leaf(FVector P,float Angle,float Length,float Width,float Lift){
  FVector D(FMath::Cos(Angle),FMath::Sin(Angle),0),W(-D.Y,D.X,0);int Start=V.Num();constexpr int Steps=7;
  for(int J=0;J<=Steps;J++)for(int I=0;I<3;I++){
   float U=J*1.f/Steps,Side=I-1.f,Spread=FMath::Pow(FMath::Sin(U*PI),.8f)*Width;
   FVector Q=P+D*(Length*U)+W*Side*Spread+FVector(0,0,Lift*U+FMath::Sin(U*PI)*(Length*.13f-Side*Side*Length*.08f));
   FVector Normal=FVector::CrossProduct(D+FVector(0,0,Lift/FMath::Max(Length,1.f)),W).GetSafeNormal();Vertex(Q,Normal,FVector2D(I*.5f,U),D);
  }
  for(int J=0;J<Steps;J++)for(int I=0;I<2;I++){int K=Start+J*3+I;T.Append({K,K+3,K+4,K,K+4,K+1});}
 }
 void Ellipsoid(FVector P,FVector Size,int Sides=40,int Rings=20){
  int Start=V.Num();for(int J=0;J<=Rings;J++)for(int I=0;I<=Sides;I++){
   float A=I*2*PI/Sides,B=J*PI/Rings;FVector D(FMath::Cos(A)*FMath::Sin(B),FMath::Sin(A)*FMath::Sin(B),FMath::Cos(B));
   Vertex(P+D*Size,(D/Size).GetSafeNormal(),FVector2D(I*1.f/Sides,J*1.f/Rings),FVector(-FMath::Sin(A),FMath::Cos(A),0));}
  for(int J=0;J<Rings;J++)for(int I=0;I<Sides;I++){int K=Start+J*(Sides+1)+I,L=K+Sides+1;T.Append({K,L,K+1,K+1,L,L+1});}
 }
 float GroundHeight(float X,float Y,int Seed) const {
  float R=FMath::Max(FMath::Abs(X)/1150.f,FMath::Abs(Y)/620.f);float Blend=FMath::SmoothStep(1.f,2.8f,R);
  return -3+Blend*(FMath::PerlinNoise2D(FVector2D(X+Seed,Y-Seed)*.00065)*140+FMath::Sin(X*.0013f+Y*.0008f)*35);
 }
 void Terrain(int Seed,float Width=8800,float Depth=7200,bool Flooded=false){
  constexpr int NX=64,NY=54;int Start=V.Num();
  for(int J=0;J<=NY;J++)for(int I=0;I<=NX;I++){float X=(I*1.f/NX-.5f)*Width,Y=(J*1.f/NY-.65f)*Depth,Z=GroundHeight(X,Y,Seed);if(Flooded)Z-=FMath::SmoothStep(1.f,2.f,FMath::Max(FMath::Abs(X)/1250.f,FMath::Abs(Y)/800.f))*240;
   FVector Normal(GroundHeight(X-10,Y,Seed)-GroundHeight(X+10,Y,Seed),GroundHeight(X,Y-10,Seed)-GroundHeight(X,Y+10,Seed),20);
   Vertex(FVector(X,Y,Z),Normal.GetSafeNormal(),FVector2D(X,Y)*.002);}
  for(int J=0;J<NY;J++)for(int I=0;I<NX;I++){int K=Start+J*(NX+1)+I,L=K+NX+1;T.Append({K,K+1,L+1,K,L+1,L});}
 }
 void Ribbon(FVector A,FVector B,float Width){
  FVector Side=FVector::CrossProduct((B-A).GetSafeNormal(),FVector::YAxisVector).GetSafeNormal()*Width;int I=V.Num();FVector Normal=FVector::CrossProduct(Side,B-A).GetSafeNormal();
  Vertex(A-Side,Normal,FVector2D(0,0));Vertex(A+Side,Normal,FVector2D(1,0));Vertex(B+Side,Normal,FVector2D(1,1));Vertex(B-Side,Normal,FVector2D(0,1));T.Append({I,I+1,I+2,I,I+2,I+3});
 }
};
}
