// Fill out your copyright notice in the Description page of Project Settings.


#include "GarProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GarProjectile)

// Sets default values
AGarProjectile::AGarProjectile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	if(!RootComponent)
    {
        RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSceneComponent"));
    }

    if(!CollisionComponent)
    {
        // 単純なコリジョン表現として、球体を使用します。
        CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
        // 球体のコリジョン半径を設定します。
        CollisionComponent->InitCapsuleSize(15.0f, 0.0f);
        // コリジョン コンポーネントとなるルート コンポーネントを設定します。
        RootComponent = CollisionComponent;
    }

	if(!ProjectileMovementComponent)
    {
        // このコンポーネントを使用し、この発射物の動きを駆動します。
        ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
        ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
        ProjectileMovementComponent->InitialSpeed = 3000.0f;
        ProjectileMovementComponent->MaxSpeed = 3000.0f;
        ProjectileMovementComponent->bRotationFollowsVelocity = true;
        ProjectileMovementComponent->bShouldBounce = true;
        ProjectileMovementComponent->Bounciness = 0.3f;
        ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
    }
}

// Called when the game starts or when spawned
void AGarProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGarProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// 発射物の発射方向の速度を初期化する関数。
void AGarProjectile::FireInDirection(const FVector& ShootDirection)
{
	ProjectileMovementComponent->Velocity = ShootDirection * ProjectileMovementComponent->InitialSpeed;
}
