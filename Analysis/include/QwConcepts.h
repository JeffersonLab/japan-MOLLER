/**********************************************************\
* File: QwConcepts.h                                       *
*                                                          *
* Architectural enforcement concepts for JAPAN-MOLLER     *
* analysis framework following the Dual-Operator Pattern  *
*                                                          *
* This file defines C++20 concepts to enforce proper      *
* implementation of delegator methods and architectural    *
* patterns as outlined in copilot-instructions.md         *
*                                                          *
* For C++ standards < 20, validation is disabled and      *
* static_asserts always pass to maintain compatibility.   *
\**********************************************************/

#ifndef __QWCONCEPTS__
#define __QWCONCEPTS__

// System headers
#include <type_traits>

// Forward declarations for classes used in concepts
class VQwDataElement;
class VQwHardwareChannel;
class VQwSubsystem;
class VQwDataHandler;
class QwSubsystemArray;
class QwSubsystemArrayParity;

// Check for C++20 concept support - can be overridden externally
// External override: Define QW_CONCEPTS_AVAILABLE=0 to disable concepts even in C++20
// External override: Define QW_CONCEPTS_AVAILABLE=1 to enable (requires C++20 support)
#ifndef QW_CONCEPTS_AVAILABLE
    #if __cplusplus >= 202002L
        #if __has_include(<concepts>)
            #include <concepts>
            #define QW_CONCEPTS_AVAILABLE 1
        #else
            #define QW_CONCEPTS_AVAILABLE 0
        #endif
    #else
        // For C++17 and earlier, always disable concepts regardless of compiler support
        #define QW_CONCEPTS_AVAILABLE 0
    #endif
#else
    // QW_CONCEPTS_AVAILABLE was defined externally, respect that setting
    #if QW_CONCEPTS_AVAILABLE && __cplusplus >= 202002L && __has_include(<concepts>)
        #include <concepts>
        // Keep QW_CONCEPTS_AVAILABLE as 1
    #else
        // Either disabled externally or C++20/concepts not available
        #undef QW_CONCEPTS_AVAILABLE
        #define QW_CONCEPTS_AVAILABLE 0
    #endif
#endif

// Forward declarations
class VQwDataElement;
class VQwHardwareChannel;
class VQwSubsystem;

namespace QwArchitecture {

#if QW_CONCEPTS_AVAILABLE
//==============================================================================
// C++20 CONCEPTS FOR DUAL-OPERATOR PATTERN ENFORCEMENT
//==============================================================================

/**
 * @brief Concept to check if a type has a type-specific UpdateErrorFlag method
 * 
 * This concept verifies that a class implements UpdateErrorFlag with its own type,
 * which is required for the type-specific version in the Dual-Operator Pattern.
 */
template<typename T>
concept HasTypeSpecificUpdateErrorFlag = requires(T& obj, const T& other) {
    { obj.UpdateErrorFlag(&other) } -> std::same_as<void>;
};

/**
 * @brief Concept to check if a type has a polymorphic UpdateErrorFlag delegator
 * 
 * This concept verifies that a class implements the delegator method that bridges
 * from VQwDataElement signature to the type-specific signature.
 */
template<typename T>
concept HasPolymorphicUpdateErrorFlag = requires(T& obj, const VQwDataElement* base_ptr) {
    { obj.UpdateErrorFlag(base_ptr) } -> std::same_as<void>;
};

/**
 * @brief Concept to check if a type properly implements the Dual-Operator Pattern
 * for UpdateErrorFlag methods.
 * 
 * According to copilot instructions, classes that implement type-specific methods
 * must also provide delegator methods for polymorphic dispatch.
 */
template<typename T>
concept ImplementsDualOperatorUpdateErrorFlag = 
    HasTypeSpecificUpdateErrorFlag<T> && 
    HasPolymorphicUpdateErrorFlag<T>;

//==============================================================================
// CONCEPTS FOR ARITHMETIC OPERATORS
//==============================================================================

/**
 * @brief Concept for type-specific arithmetic operators
 * 
 * Validates that a class implements all required type-specific arithmetic operations
 * as specified in copilot-instructions.md
 */
template<typename T>
concept HasTypeSpecificArithmetic = requires(T& obj, const T& other) {
    { obj += other } -> std::same_as<T&>;
    { obj -= other } -> std::same_as<T&>;
    { obj.Sum(other, other) } -> std::same_as<void>;
    { obj.Difference(other, other) } -> std::same_as<void>;
    { obj.Ratio(other, other) } -> std::same_as<T&>;
};

/**
 * @brief Concept for polymorphic arithmetic operators (for VQwDataElement derivatives)
 * 
 * Validates delegator methods that bridge from base class signatures to type-specific implementations
 */
template<typename T>
concept HasPolymorphicArithmetic = requires(T& obj, const VQwDataElement& base) {
    { obj += base } -> std::same_as<VQwDataElement&>;
    { obj -= base } -> std::same_as<VQwDataElement&>;
    { obj.Sum(base, base) } -> std::same_as<void>;
    { obj.Difference(base, base) } -> std::same_as<void>;
    { obj.Ratio(base, base) } -> std::same_as<VQwDataElement&>;
};

/**
 * @brief Complete Dual-Operator Pattern for arithmetic operations
 */
template<typename T>
concept ImplementsDualOperatorArithmetic = 
    HasTypeSpecificArithmetic<T> && 
    HasPolymorphicArithmetic<T>;

//==============================================================================
// CONCEPTS FOR EVENT CUTS AND DIAGNOSTICS
//==============================================================================

/**
 * @brief Concept for type-specific event cuts and diagnostics
 * 
 * Validates that a class implements all required type-specific event cuts and diagnostics
 * as specified in copilot-instructions.md
 * Note: Using standard C++ types to avoid ROOT dependencies in concepts
 */
template<typename T>
concept HasTypeSpecificEventCutsAndDiagnostics = requires(T& obj, const T* other) {
    { obj.SetSingleEventCuts(0U, 0.0, 0.0, 0.0) } -> std::same_as<void>;
    { obj.CheckForBurpFail(other) } -> std::same_as<bool>;
};

/**
 * @brief Concept for polymorphic event cuts and diagnostics (for VQwDataElement derivatives)
 * 
 * Validates delegator methods that bridge from base class signatures to type-specific implementations
 */
template<typename T>
concept HasPolymorphicEventCutsAndDiagnostics = requires(T& obj, const VQwDataElement* base) {
    { obj.SetSingleEventCuts(0U, 0.0, 0.0, 0.0) } -> std::same_as<void>;
    { obj.CheckForBurpFail(base) } -> std::same_as<bool>;
};

/**
 * @brief Complete Dual-Operator Pattern for event cuts and diagnostics
 */
template<typename T>
concept ImplementsDualOperatorEventCutsAndDiagnostics = 
    HasTypeSpecificEventCutsAndDiagnostics<T> &&
    HasPolymorphicEventCutsAndDiagnostics<T>;

//==============================================================================
// SPECIALIZED BASE CLASS CONCEPTS
//==============================================================================

/**
 * @brief Concept for specialized bases that need polymorphic UpdateErrorFlag
 * 
 * Some hierarchies introduce specialized abstract bases (VQwBPM, VQwBCM, VQwClock)
 * that are used polymorphically by container code and must expose virtual hooks.
 */
template<typename T>
concept SpecializedBaseWithPolymorphicUpdateErrorFlag = std::is_abstract_v<T> && 
    requires(T& obj, const T* typed_ptr) {
        { obj.UpdateErrorFlag(typed_ptr) } -> std::same_as<void>;
    };

/**
 * @brief Concept for specialized bases with CheckForBurpFail support
 * 
 * Validates that specialized bases like VQwClock, VQwBPM, VQwBCM provide
 * the necessary virtual hooks for polymorphic dispatch
 */
template<typename T>
concept SpecializedBaseWithPolymorphicCheckForBurpFail = std::is_abstract_v<T> &&
    requires(T& obj, const T* typed_ptr) {
        { obj.CheckForBurpFail(typed_ptr) } -> std::same_as<bool>;
    };

/**
 * @brief Complete specialized base pattern validation
 */
template<typename T>
concept ImplementsSpecializedBasePattern = 
    SpecializedBaseWithPolymorphicUpdateErrorFlag<T> &&
    SpecializedBaseWithPolymorphicCheckForBurpFail<T>;

//==============================================================================
// CONTAINER DELEGATION PATTERN CONCEPTS
//==============================================================================

/**
 * @brief Concept for container classes using the Container-Delegation Pattern
 * 
 * Container classes should use single operator versions and delegate to
 * contained objects, avoiding virtual operator inheritance issues.
 * This applies to subsystem arrays and similar collection classes.
 * 
 * The pattern requires:
 * 1. Type-specific operators that return the container type
 * 2. Should NOT have virtual operators with VQwDataElement base signatures
 * 
 * Critical distinction from Dual-Operator Pattern:
 * - Container classes use type-specific operators (e.g., QwSubsystemArrayParity += QwSubsystemArrayParity)
 * - Contained objects use VQwSubsystem operators (for subsystem containers) or VQwDataElement operators (for data element containers)
 * - Container classes should NEVER use VQwDataElement operators directly
 */
template<typename T>
concept ImplementsContainerDelegationPattern = requires(T& obj, const T& other) {
    // Container should have type-specific operators only
    { obj += other } -> std::same_as<T&>;
    { obj -= other } -> std::same_as<T&>;
    { obj.Sum(other, other) } -> std::same_as<void>;
};

/**
 * @brief Concept for polymorphic subsystem pattern
 * 
 * Regular subsystems (non-container) should implement VQwSubsystem polymorphic operators
 * for integration with the framework's polymorphic dispatch system.
 * This applies to classes like QwOmnivore that act as subsystem implementations.
 */
template<typename T>
concept ImplementsPolymorphicSubsystemPattern = requires(T& obj, VQwSubsystem* other) {
    // Subsystem should have polymorphic operators with VQwSubsystem base
    { obj += other } -> std::same_as<VQwSubsystem&>;
    { obj -= other } -> std::same_as<VQwSubsystem&>;
    { obj.Sum(other, other) } -> std::same_as<void>;
};

/**
 * @brief Helper to determine if a class name suggests it's a container
 * 
 * Uses compile-time template matching to identify container classes.
 * Container classes typically have "Array" in their name.
 */
template<typename T>
struct IsContainerClass {
    static constexpr bool value = false;
};

// Specialization for QwSubsystemArrayParity and similar array classes
template<>
struct IsContainerClass<QwSubsystemArrayParity> {
    static constexpr bool value = true;
};

template<>
struct IsContainerClass<QwSubsystemArray> {
    static constexpr bool value = true;
};

//==============================================================================
// ARCHITECTURAL VALIDATION CONCEPTS
//==============================================================================

/**
 * @brief Master concept for VQwDataElement derivatives
 * 
 * Enforces that concrete VQwDataElement derivatives properly implement
 * the complete Dual-Operator Pattern for all required methods as specified
 * in copilot-instructions.md
 */
template<typename T>
concept ValidVQwDataElementDerivative = 
    std::is_base_of_v<VQwDataElement, T> &&
    !std::is_abstract_v<T> &&
    ImplementsDualOperatorArithmetic<T> &&
    ImplementsDualOperatorUpdateErrorFlag<T> &&
    ImplementsDualOperatorEventCutsAndDiagnostics<T>;

/**
 * @brief Master concept for VQwHardwareChannel derivatives
 * 
 * VQwHardwareChannel derivatives must satisfy all VQwDataElement requirements
 * plus any additional hardware-specific patterns
 */
template<typename T>
concept ValidVQwHardwareChannelDerivative = 
    std::is_base_of_v<VQwHardwareChannel, T> &&
    !std::is_abstract_v<T> &&
    ValidVQwDataElementDerivative<T>;

/**
 * @brief Master concept for specialized abstract bases
 * 
 * Validates specialized bases like VQwBPM, VQwBCM, VQwClock that provide
 * polymorphic dispatch points for container code
 */
template<typename T>
concept ValidSpecializedBase = 
    std::is_abstract_v<T> &&
    std::is_base_of_v<VQwDataElement, T> &&
    ImplementsSpecializedBasePattern<T>;

/**
 * @brief Master concept for container classes
 * 
 * Container classes should use the Container-Delegation Pattern
 */
template<typename T>
concept ValidContainerClass = 
    ImplementsContainerDelegationPattern<T>;

/**
 * @brief Master concept for polymorphic subsystem classes
 * 
 * Regular subsystem classes should use polymorphic VQwSubsystem operators
 */
template<typename T>
concept ValidPolymorphicSubsystem = 
    std::is_base_of_v<VQwSubsystem, T> &&
    !std::is_abstract_v<T> &&
    ImplementsPolymorphicSubsystemPattern<T>;

/**
 * @brief Unified subsystem validation concept
 * 
 * Validates subsystems using appropriate pattern based on class characteristics
 * Container classes use Container-Delegation Pattern, others use Polymorphic Subsystem Pattern
 */
template<typename T>
concept ValidSubsystemClass = 
    (IsContainerClass<T>::value && ValidContainerClass<T>) ||
    (!IsContainerClass<T>::value && ValidPolymorphicSubsystem<T>);

#endif // QW_CONCEPTS_AVAILABLE

//==============================================================================
// STATIC ASSERTION HELPERS (WORK WITH BOTH C++17 AND C++20)
//==============================================================================

/**
 * @brief Helper macro to validate architectural compliance in concrete classes
 * 
 * Usage: VALIDATE_DATA_ELEMENT_PATTERN(MyConcreteClass);
 * 
 * In C++20: Performs full concept validation for all required methods
 * In C++17: Always passes but documents architectural requirements
 */
#if QW_CONCEPTS_AVAILABLE
#define VALIDATE_DATA_ELEMENT_PATTERN(ClassName) \
    static_assert(QwArchitecture::ValidVQwDataElementDerivative<ClassName>, \
        #ClassName " must implement complete Dual-Operator Pattern for all required methods. " \
        "See copilot-instructions.md for implementation requirements. " \
        "(C++20 concept validation active)"); \
    static_assert(QwArchitecture::HasTypeSpecificUpdateErrorFlag<ClassName>, \
        #ClassName " must implement type-specific UpdateErrorFlag(const " #ClassName "*) " \
        "(C++20 concept validation active)"); \
    static_assert(QwArchitecture::HasPolymorphicUpdateErrorFlag<ClassName>, \
        #ClassName " must implement polymorphic delegator UpdateErrorFlag(const VQwDataElement*) " \
        "(C++20 concept validation active)"); \
    static_assert(QwArchitecture::HasTypeSpecificArithmetic<ClassName>, \
        #ClassName " must implement type-specific arithmetic operators (+=, -=, Sum, Difference, Ratio) " \
        "(C++20 concept validation active)"); \
    static_assert(QwArchitecture::HasPolymorphicArithmetic<ClassName>, \
        #ClassName " must implement polymorphic arithmetic delegators " \
        "(C++20 concept validation active)"); \
    static_assert(QwArchitecture::ImplementsDualOperatorEventCutsAndDiagnostics<ClassName>, \
        #ClassName " must implement SetSingleEventCuts and CheckForBurpFail patterns " \
        "(C++20 concept validation active)");
#else
#define VALIDATE_DATA_ELEMENT_PATTERN(ClassName) \
    /* C++17 mode: Architectural patterns documented but validation disabled */ \
    static_assert(true, #ClassName " architectural requirements documented (upgrade to C++20 for validation)");
#endif

/**
 * @brief Helper macro to validate data handler compliance
 * 
 * Data handlers have different architectural requirements than data elements.
 * They don't need the Dual-Operator Pattern since they handle data processing
 * rather than data storage and arithmetic operations.
 */
#if QW_CONCEPTS_AVAILABLE
#define VALIDATE_DATA_HANDLER_PATTERN(ClassName) \
    static_assert(std::is_base_of_v<VQwDataHandler, ClassName>, \
        #ClassName " must inherit from VQwDataHandler for factory registration. " \
        "(C++20 concept validation active)");
#else
#define VALIDATE_DATA_HANDLER_PATTERN(ClassName) \
    /* C++17 mode: Data handler patterns documented but validation disabled */ \
    static_assert(true, #ClassName " data handler requirements documented (upgrade to C++20 for validation)");
#endif

/**
 * @brief Helper macro to validate unified subsystem compliance
 */
#if QW_CONCEPTS_AVAILABLE
#define VALIDATE_SUBSYSTEM_PATTERN(ClassName) \
    static_assert(QwArchitecture::ValidSubsystemClass<ClassName>, \
        #ClassName " must implement appropriate subsystem pattern. Containers use Container-Delegation, " \
        "regular subsystems use Polymorphic Subsystem patterns. " \
        "(C++20 concept validation active)");
#else
#define VALIDATE_SUBSYSTEM_PATTERN(ClassName) \
    /* C++17 mode: Subsystem patterns documented but validation disabled */ \
    static_assert(true, #ClassName " subsystem requirements documented (upgrade to C++20 for validation)");
#endif

} // namespace QwArchitecture

#endif // __QWCONCEPTS__