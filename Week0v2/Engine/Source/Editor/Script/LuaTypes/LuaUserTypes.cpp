#include "LuaUserTypes.h"

#include "Engine/World.h"
//#include "Developer/Lua/LuaActor.h"
#include "Script/LuaUtils/LuaBindMacros.h"
#include "Math/Color.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Math/Rotator.h"
#include "Script/LuaActor.h"

#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"

void LuaTypes::FBindLua<FColor>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FColor,

        // 생성자
        sol::constructors<FColor(), FColor(uint8, uint8, uint8, uint8)>(),

        // 멤버 변수
        LUA_BIND_MEMBER(&FColor::R),
        LUA_BIND_MEMBER(&FColor::G),
        LUA_BIND_MEMBER(&FColor::B),
        LUA_BIND_MEMBER(&FColor::A),

        // 연산자 오버로딩
        sol::meta_function::equal_to, &FColor::operator==,

        // 정적 상수 (읽기 전용 속성으로 바인딩)
        LUA_BIND_VAR(FColor::White),
        LUA_BIND_VAR(FColor::Black),
        LUA_BIND_VAR(FColor::Transparent),
        LUA_BIND_VAR(FColor::Red),
        LUA_BIND_VAR(FColor::Green),
        LUA_BIND_VAR(FColor::Blue),
        LUA_BIND_VAR(FColor::Yellow)
    );
}

void LuaTypes::FBindLua<FLinearColor>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FLinearColor,

        // Constructors
        sol::constructors<FLinearColor(), FLinearColor(float, float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FLinearColor::R),
        LUA_BIND_MEMBER(&FLinearColor::G),
        LUA_BIND_MEMBER(&FLinearColor::B),
        LUA_BIND_MEMBER(&FLinearColor::A),

        // Operators
        sol::meta_function::equal_to, &FLinearColor::operator==,
        sol::meta_function::multiplication, [](const FLinearColor& A, const FLinearColor& B) { return A * B; },
        sol::meta_function::addition, &FLinearColor::operator+,

        // Static constants
        LUA_BIND_VAR(FLinearColor::White),
        LUA_BIND_VAR(FLinearColor::Black),
        //LUA_BIND_VAR(FLinearColor::Transparent),
        LUA_BIND_VAR(FLinearColor::Red),
        LUA_BIND_VAR(FLinearColor::Green),
        LUA_BIND_VAR(FLinearColor::Blue)
        //LUA_BIND_VAR(FLinearColor::Yellow)
    );
}

void LuaTypes::FBindLua<FVector>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FVector,

        // Constructors
        sol::constructors<FVector(), FVector(float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FVector::X),
        LUA_BIND_MEMBER(&FVector::Y),
        LUA_BIND_MEMBER(&FVector::Z),

        // Operators
        sol::meta_function::equal_to, &FVector::operator==,
        sol::meta_function::addition, &FVector::operator+,
        sol::meta_function::subtraction, [](const FVector& A, const FVector& B) { return A - B; },
        sol::meta_function::multiplication, sol::overload(
            [](const FVector& v, float f) -> FVector { 
                return v * f;
            },
            [](float f, const FVector& v) -> FVector {
                return v * f;
            },
            [](const FVector& A, const FVector& B) { return A * B; }
        ) ,
        sol::meta_function::division, [](const FVector& A, const FVector& B) { return A / B; },
        

        // Utility functions
        //LUA_BIND_MEMBER(&FVector::Length),
       // LUA_BIND_MEMBER(&FVector::SquaredLength),
        LUA_BIND_MEMBER(&FVector::Normalize),
        //LUA_BIND_MEMBER(&FVector::IsNormalized),
        //LUA_BIND_MEMBER(&FVector::DotProduct),
        //LUA_BIND_MEMBER(&FVector::CrossProduct),

        // Static properties
        LUA_BIND_VAR(FVector::ZeroVector),
        LUA_BIND_VAR(FVector::OneVector),
        LUA_BIND_VAR(FVector::UpVector),
        LUA_BIND_VAR(FVector::ForwardVector),
        LUA_BIND_VAR(FVector::RightVector)
    );
}

void LuaTypes::FBindLua<FVector2D>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FVector2D,

        // Constructors
        sol::constructors<FVector2D(), FVector2D(float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FVector2D::x),
        LUA_BIND_MEMBER(&FVector2D::y),

        // Operators
        //sol::meta_function::equal_to, &FVector2D::operator==,
        sol::meta_function::addition, &FVector2D::operator+,
        sol::meta_function::subtraction, &FVector2D::operator-,
        sol::meta_function::multiplication, &FVector2D::operator*,
        sol::meta_function::division, &FVector2D::operator/

        // Utility functions
        // LUA_BIND_MEMBER(&FVector2D::Length),
        // LUA_BIND_MEMBER(&FVector2D::LengthSquared),
        // LUA_BIND_MEMBER(&FVector2D::Normalize),
        // LUA_BIND_MEMBER(&FVector2D::IsNormalized),
        // LUA_BIND_MEMBER(&FVector2D::DotProduct),

        // Static properties
        //LUA_BIND_VAR(FVector2D::ZeroVector)
        // LUA_BIND_VAR(FVector2D::UnitVector)
    );
}

void LuaTypes::FBindLua<FVector4>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FVector4,

        // Constructors
        sol::constructors<FVector4(), FVector4(float, float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FVector4::x),
        LUA_BIND_MEMBER(&FVector4::y),
        LUA_BIND_MEMBER(&FVector4::z),
        LUA_BIND_MEMBER(&FVector4::w)
    );
}

void LuaTypes::FBindLua<FRotator>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FRotator,

        // Constructors
        sol::constructors<FRotator(), FRotator(float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FRotator::Pitch),
        LUA_BIND_MEMBER(&FRotator::Yaw),
        LUA_BIND_MEMBER(&FRotator::Roll),

        // Operators
        sol::meta_function::equal_to, &FRotator::operator==,
        sol::meta_function::addition, &FRotator::operator+,
        sol::meta_function::subtraction, [](const FRotator& A, const FRotator& B) { return A - B; },
        sol::meta_function::multiplication, &FRotator::operator*,

        // Utility functions
        LUA_BIND_MEMBER(&FRotator::Normalize),
        LUA_BIND_MEMBER(&FRotator::GetNormalized)
        // LUA_BIND_MEMBER(&FRotator::GetInverse),

        // Static properties
        // LUA_BIND_VAR(FRotator::ZeroRotator)
    );
}

void LuaTypes::FBindLua<FQuat>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FQuat,

        // Constructors
        sol::constructors<FQuat(), FQuat(float, float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FQuat::x),
        LUA_BIND_MEMBER(&FQuat::y),
        LUA_BIND_MEMBER(&FQuat::z),
        LUA_BIND_MEMBER(&FQuat::w),

        // Utility functions
        LUA_BIND_MEMBER(&FQuat::Normalize),
        LUA_BIND_MEMBER(&FQuat::IsNormalized),
        LUA_BIND_MEMBER(&FQuat::RotateVector)
        // LUA_BIND_MEMBER(&FQuat::GetAxisX),
        // LUA_BIND_MEMBER(&FQuat::GetAxisY),
        // LUA_BIND_MEMBER(&FQuat::GetAxisZ),

        // Static functions
        // LUA_BIND_VAR(FQuat::Identity)
    );
}

void LuaTypes::FBindLua<FMatrix>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FMatrix,

        // Constructors
        sol::constructors<FMatrix()>(),

        // Matrix operations
        // LUA_BIND_MEMBER(&FMatrix::Determinant),
        // LUA_BIND_MEMBER(&FMatrix::Inverse),
        // LUA_BIND_MEMBER(&FMatrix::Transpose),
        // LUA_BIND_MEMBER(&FMatrix::TransformVector),
        // LUA_BIND_MEMBER(&FMatrix::TransformPosition),

        // Static functions
        LUA_BIND_VAR(FMatrix::Identity)
    );
}

void LuaTypes::FBindLua<UObject>::Bind(sol::table& Table)
{
    Table.new_usertype<UObject>("UObject" // 타입 이름 명시
       , sol::no_constructor
       , LUA_BIND_MEMBER(&UObject::GetName)
   );
}

// void LuaTypes::FBindLua<ALuaActor>::Bind(sol::table& Table)
// {
//     Table.Lua_NewUserType(
//         ALuaActor,
//
//         // UObject 메서드
//         LUA_BIND_MEMBER(&ALuaActor::Duplicate),
//         LUA_BIND_MEMBER(&ALuaActor::GetFName),
//         LUA_BIND_MEMBER(&ALuaActor::GetName),
//         //LUA_BIND_MEMBER(&ALuaActor::GetOuter),
//         LUA_BIND_MEMBER(&ALuaActor::GetWorld),
//         LUA_BIND_MEMBER(&ALuaActor::GetUUID),
//         LUA_BIND_MEMBER(&ALuaActor::GetClass),
//
//         // AActor 메서드
//         LUA_BIND_MEMBER(&ALuaActor::GetActorLocation),
//         LUA_BIND_MEMBER(&ALuaActor::GetActorRotation),
//         LUA_BIND_MEMBER(&ALuaActor::GetActorScale),
//
//         LUA_BIND_MEMBER(&ALuaActor::GetActorForwardVector),
//         LUA_BIND_MEMBER(&ALuaActor::GetActorRightVector),
//         LUA_BIND_MEMBER(&ALuaActor::GetActorUpVector),
//
//         LUA_BIND_MEMBER(&ALuaActor::SetActorLocation),
//         LUA_BIND_MEMBER(&ALuaActor::SetActorRotation),
//         LUA_BIND_MEMBER(&ALuaActor::SetActorScale),
//
//         LUA_BIND_MEMBER(&ALuaActor::GetRootComponent),
//         LUA_BIND_MEMBER(&ALuaActor::SetRootComponent),
//         LUA_BIND_MEMBER(&ALuaActor::GetOwner),
//         LUA_BIND_MEMBER(&ALuaActor::SetOwner),
//
//         LUA_BIND_MEMBER(&ALuaActor::Destroy),
//         LUA_BIND_MEMBER(&ALuaActor::IsActorBeingDestroyed)
//     );
// }

void LuaTypes::FBindLua<AActor>::Bind(sol::table& Table)
{
    
    Table.Lua_NewUserType(
        AActor,

        sol::base_classes, sol::bases<UObject>(),
        // UObject 메서드
        LUA_BIND_MEMBER(&AActor::Duplicate),
        LUA_BIND_MEMBER(&AActor::GetFName),
        LUA_BIND_MEMBER(&AActor::GetName),
        //LUA_BIND_MEMBER(&ALuaActor::GetOuter),
        LUA_BIND_MEMBER(&AActor::GetWorld),
        LUA_BIND_MEMBER(&AActor::GetUUID),
        LUA_BIND_MEMBER(&AActor::GetClass),

        // AActor 메서드
        LUA_BIND_MEMBER(&AActor::GetActorLocation),
        LUA_BIND_MEMBER(&AActor::GetActorRotation),
        LUA_BIND_MEMBER(&AActor::GetActorScale),

        LUA_BIND_MEMBER(&AActor::GetActorForwardVector),
        LUA_BIND_MEMBER(&AActor::GetActorRightVector),
        LUA_BIND_MEMBER(&AActor::GetActorUpVector),

        LUA_BIND_MEMBER(&AActor::SetActorLocation),
        LUA_BIND_MEMBER(&AActor::SetActorRotation),
        LUA_BIND_MEMBER(&AActor::SetActorScale),

        LUA_BIND_MEMBER(&AActor::GetRootComponent),
        LUA_BIND_MEMBER(&AActor::SetRootComponent),
        LUA_BIND_MEMBER(&AActor::GetOwner),
        LUA_BIND_MEMBER(&AActor::SetOwner),
        LUA_BIND_MEMBER(&AActor::AddComponentByName),

        LUA_BIND_MEMBER(&AActor::Destroy),
        LUA_BIND_MEMBER(&AActor::IsActorBeingDestroyed),
        "GetComponentByClass",
        // 람다 함수 정의: 첫 인자는 객체 자신(self), 다음 인자는 Lua에서 전달될 값
        [](const AActor& self, const std::string& componentClassName) -> USceneComponent* {
            // Lua에서 받은 std::string을 FString으로 변환 (FString 구현에 따라 다름)
            FString classNameFString(componentClassName.c_str()); // 또는 다른 변환 방식 사용

            // 명시적으로 원하는 오버로딩 버전 호출
            return Cast<USceneComponent>(self.GetComponentByClass(classNameFString));
        }
    );
}

void LuaTypes::FBindLua<UActorComponent>::Bind(sol::table& engineTable)
{
    engineTable.new_usertype<UActorComponent>("UActorComponent" // 타입 이름 명시
       , sol::no_constructor
       , sol::base_classes, sol::bases<UObject>()
       , "IsActive", &UActorComponent::IsActive // 멤버 함수 직접 추가
   );
}

void LuaTypes::FBindLua<USceneComponent>::Bind(sol::table& engineTable)
{
    // LUA_BIND_MEMBER 매크로가 실제 멤버를 이름과 함께 추가한다고 가정하고 직접 풀어서 작성
    // 만약 매크로가 단순 이름 지정이 아니었다면 그에 맞게 수정 필요
    engineTable.new_usertype<USceneComponent>("USceneComponent" // 타입 이름 명시
        , sol::no_constructor
        , sol::base_classes, sol::bases<UActorComponent>() // 상속 명시
        // 멤버들 직접 바인딩 (LUA_BIND_MEMBER를 풀어서)
        , "GetForwardVector", &USceneComponent::GetForwardVector
        , "GetRightVector", &USceneComponent::GetRightVector
        , "GetUpVector", &USceneComponent::GetUpVector
        , "AddLocation", &USceneComponent::AddLocation
        , "AddRotation", &USceneComponent::AddRotation
        , "AddScale", &USceneComponent::AddScale
        , "GetWorldLocation", &USceneComponent::GetWorldLocation
        , "GetWorldRotation", &USceneComponent::GetWorldRotation
        , "GetWorldScale", &USceneComponent::GetWorldScale
        , "GetLocalLocation", &USceneComponent::GetLocalLocation
        , "GetLocalScale", &USceneComponent::GetLocalScale
    );
}

void LuaTypes::FBindLua<UStaticMeshComponent>::Bind(sol::table& engineTable)
{
    engineTable.new_usertype<UStaticMeshComponent>("UStaticMeshComponent" // 타입 이름 명시
        , sol::no_constructor
        , sol::base_classes, sol::bases<USceneComponent>() // 상속 명시
        // 멤버 직접 바인딩
        , "GetNumMaterials", &UStaticMeshComponent::GetNumMaterials
        // ... 필요하다면 다른 멤버들 ...
    );
}



void LuaTypes::FBindLua<UWorld>::Bind(sol::table& engineTable)
{
// {
//     if (!engineTable["EWorldType"].valid()) { // 이미 바인딩되었는지 확인
//         engineTable.new_enum("EWorldType",
//            "None", EWorldType::None,
//            "Game", EWorldType::Game,
//            "Editor", EWorldType::Editor,
//            "PIE", EWorldType::PIE, // Play In Editor
//            "EditorPreview", EWorldType::EditorPreview,
//            "Inactive", EWorldType::Inactive
//        );
//     }
//
//      engineTable.new_usertype<UWorld>("World",
//         // 생성자 미노출 - World는 보통 엔진에 의해 관리됩니다.
//         sol::no_constructor,
//     
//         // --- 기본 클래스 ---
//         sol::bases<UObject>(), // UObject로부터 상속
//     
//         // --- 씬/레벨 관리 ---
//         "LoadLevel", &UWorld::LoadLevel,               // void(const FString& LevelName)
//         "LoadScene", &UWorld::LoadScene,               // void(const FString& FileName)
//         "SaveScene", &UWorld::SaveScene,               // void(const FString& FileName)
//         "ReloadScene", &UWorld::ReloadScene,           // void(const FString& FileName)
//         // "ClearScene", &UWorld::ClearScene,          // 잠재적으로 위험할 수 있음? 주의해서 노출하세요.
//     
//         // --- 액터 관리 ---
//         // SpawnActorByName이 Lua에서 액터를 스폰하는 가장 친화적인 방법입니다.
//         "SpawnActorByName", &UWorld::SpawnActorByName, // AActor*(const FString& ActorName, bool bCallBeginPlay)
//         "DestroyActor", &UWorld::DestroyActor,         // bool(AActor* ThisActor)
//     
//         // --- 액터 쿼리 ---
//         // GetActors는 TArray<AActor*>를 반환합니다. 헬퍼를 사용하거나 sol2의 자동 변환에 의존하세요.
//         "GetActors", [](UWorld& self, sol::this_state ts) { // 헬퍼와 함께 람다 사용
//              return ConvertActorArrayToLuaTable(ts, self.GetActors());
//          },
//         // "GetActors", &UWorld::GetActors, // 대안: 직접 바인딩 시도 (작동할 수도 있음)
//     
//     
//         // --- 월드 정보 ---
//         "IsPIEWorld", &UWorld::IsPIEWorld,             // bool() const
//         "GetLevel", &UWorld::GetLevel,                 // ULevel*() const (ULevel 바인딩 필요)
//         "WorldType", sol::readonly(&UWorld::WorldType), // EWorldType::Type (읽기 전용 권장)
//     
//     
//         // --- 에디터/선택 기능 (나중에 "Editor" 테이블로 옮기는 것이 좋을 수 있음) ---
//         "GetEditorPlayer", &UWorld::GetEditorPlayer,   // AEditorPlayer*() const (AEditorPlayer 바인딩 필요)
//          // GetSelectedActors는 TSet<AActor*>를 반환합니다. 헬퍼를 사용하세요.
//         "GetSelectedActors", [](UWorld& self, sol::this_state ts) {
//              return ConvertActorSetToLuaTable(ts, self.GetSelectedActors());
//          },
//         "SetSelectedActor", &UWorld::SetSelectedActor, // void(AActor* InActor)
//         "AddSelectedActor", &UWorld::AddSelectedActor, // void(AActor* InActor)
//         "ClearSelectedActors", &UWorld::ClearSelectedActors, // void()
//         "DuplicateSeletedActors", &UWorld::DuplicateSeletedActors, // void()
//         "DuplicateSeletedActorsOnLocation", &UWorld::DuplicateSeletedActorsOnLocation, // void()
//     
//         // 기즈모 관련 (USceneComponent/UTransformGizmo 바인딩 필요)
//         "GetPickingGizmo", &UWorld::GetPickingGizmo,   // USceneComponent*() const
//         "SetPickingGizmo", &UWorld::SetPickingGizmo,   // void(UObject* Object)
//         "LocalGizmo", sol::readonly(&UWorld::LocalGizmo) // UTransformGizmo* (읽기 전용 권장)
//     
//     
//     );
}


