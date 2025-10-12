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
 */
template<typename T>
concept ImplementsContainerDelegationPattern = requires(T& obj, const T& other) {
    // Container should have type-specific operators only
    { obj += other } -> std::same_as<T&>;
    { obj -= other } -> std::same_as<T&>;
    { obj.Sum(other, other) } -> std::same_as<void>;
    // Should NOT have virtual operators with base class signatures
} && !requires(T& obj, const VQwDataElement& base) {
    { obj += base } -> std::same_as<VQwDataElement&>;
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
 */
template<typename T>
concept ValidContainerClass = 
    ImplementsContainerDelegationPattern<T>;

#else
//==============================================================================
// C++17 FALLBACK: EMPTY CONCEPTS (ALWAYS TRUE)
//==============================================================================

// For pre-C++20 compilers, define concept-like templates that always evaluate to true
template<typename T>
constexpr bool HasTypeSpecificUpdateErrorFlag = true;

template<typename T>
constexpr bool HasPolymorphicUpdateErrorFlag = true;

template<typename T>
constexpr bool ImplementsDualOperatorUpdateErrorFlag = true;

template<typename T>
constexpr bool HasTypeSpecificArithmetic = true;

template<typename T>
constexpr bool HasPolymorphicArithmetic = true;

template<typename T>
constexpr bool ImplementsDualOperatorArithmetic = true;

template<typename T>
constexpr bool HasTypeSpecificEventCutsAndDiagnostics = true;

template<typename T>
constexpr bool HasPolymorphicEventCutsAndDiagnostics = true;

template<typename T>
constexpr bool ImplementsDualOperatorEventCutsAndDiagnostics = true;

template<typename T>
constexpr bool SpecializedBaseWithPolymorphicUpdateErrorFlag = true;

template<typename T>
constexpr bool SpecializedBaseWithPolymorphicCheckForBurpFail = true;

template<typename T>
constexpr bool ImplementsSpecializedBasePattern = true;

template<typename T>
constexpr bool ImplementsContainerDelegationPattern = true;

template<typename T>
constexpr bool ValidVQwDataElementDerivative = true;

template<typename T>
constexpr bool ValidVQwHardwareChannelDerivative = true;

template<typename T>
constexpr bool ValidSpecializedBase = true;

template<typename T>
constexpr bool ValidContainerClass = true;

#endif // QW_CONCEPTS_AVAILABLE

//==============================================================================
// STATIC ASSERTION HELPERS (WORK WITH BOTH C++17 AND C++20)
//==============================================================================

/**
 * @brief Helper macro to validate architectural compliance in concrete classes
 * 
 * Usage: VALIDATE_DUAL_OPERATOR_PATTERN(MyConcreteClass);
 * 
 * In C++20: Performs full concept validation for all required methods
 * In C++17: Always passes but documents architectural requirements
 */
#if QW_CONCEPTS_AVAILABLE
#define VALIDATE_DUAL_OPERATOR_PATTERN(ClassName) \
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
#define VALIDATE_DUAL_OPERATOR_PATTERN(ClassName) \
    /* C++17 mode: Architectural patterns documented but validation disabled */ \
    static_assert(true, #ClassName " architectural requirements documented (upgrade to C++20 for validation)");
#endif

/**
 * @brief Helper macro to validate hardware channel compliance
 */
#if QW_CONCEPTS_AVAILABLE
#define VALIDATE_HARDWARE_CHANNEL_PATTERN(ClassName) \
    VALIDATE_DUAL_OPERATOR_PATTERN(ClassName) \
    static_assert(QwArchitecture::ValidVQwHardwareChannelDerivative<ClassName>, \
        #ClassName " must properly inherit from VQwHardwareChannel and implement required patterns. " \
        "(C++20 concept validation active)");
#else
#define VALIDATE_HARDWARE_CHANNEL_PATTERN(ClassName) \
    VALIDATE_DUAL_OPERATOR_PATTERN(ClassName)
#endif

/**
 * @brief Helper macro to validate container delegation pattern
 */
#if QW_CONCEPTS_AVAILABLE
#define VALIDATE_CONTAINER_DELEGATION_PATTERN(ClassName) \
    static_assert(QwArchitecture::ValidContainerClass<ClassName>, \
        #ClassName " must implement Container-Delegation Pattern. Use type-specific operators only " \
        "and delegate to contained objects. Avoid virtual operators with base signatures. " \
        "(C++20 concept validation active)");
#else
#define VALIDATE_CONTAINER_DELEGATION_PATTERN(ClassName) \
    /* C++17 mode: Container patterns documented but validation disabled */ \
    static_assert(true, #ClassName " container requirements documented (upgrade to C++20 for validation)");
#endif

/**
 * @brief Helper macro for specialized abstract bases
 */
#if QW_CONCEPTS_AVAILABLE
#define VALIDATE_SPECIALIZED_BASE_PATTERN(ClassName) \
    static_assert(QwArchitecture::ValidSpecializedBase<ClassName>, \
        #ClassName " specialized abstract base must provide virtual hooks for polymorphic dispatch. " \
        "Must implement UpdateErrorFlag and CheckForBurpFail virtual methods. " \
        "(C++20 concept validation active)");
#else
#define VALIDATE_SPECIALIZED_BASE_PATTERN(ClassName) \
    /* C++17 mode: Specialized base patterns documented but validation disabled */ \
    static_assert(true, #ClassName " specialized base requirements documented (upgrade to C++20 for validation)");
#endif

/**
 * @brief Helper macro for validating individual method groups
 */
#if QW_CONCEPTS_AVAILABLE
#define VALIDATE_ARITHMETIC_PATTERN(ClassName) \
    static_assert(QwArchitecture::ImplementsDualOperatorArithmetic<ClassName>, \
        #ClassName " must implement complete arithmetic Dual-Operator Pattern " \
        "(+=, -=, Sum, Difference, Ratio with both type-specific and polymorphic versions) " \
        "(C++20 concept validation active)");

#define VALIDATE_DIAGNOSTICS_PATTERN(ClassName) \
    static_assert(QwArchitecture::ImplementsDualOperatorEventCutsAndDiagnostics<ClassName>, \
        #ClassName " must implement complete event cuts and diagnostics Dual-Operator Pattern " \
        "(SetSingleEventCuts, CheckForBurpFail with both type-specific and polymorphic versions) " \
        "(C++20 concept validation active)");

#define VALIDATE_UPDATE_ERROR_FLAG_PATTERN(ClassName) \
    static_assert(QwArchitecture::ImplementsDualOperatorUpdateErrorFlag<ClassName>, \
        #ClassName " must implement complete UpdateErrorFlag Dual-Operator Pattern " \
        "(type-specific and polymorphic delegator versions) " \
        "(C++20 concept validation active)");
#else
#define VALIDATE_ARITHMETIC_PATTERN(ClassName) \
    static_assert(true, #ClassName " arithmetic requirements documented (upgrade to C++20 for validation)");

#define VALIDATE_DIAGNOSTICS_PATTERN(ClassName) \
    static_assert(true, #ClassName " diagnostics requirements documented (upgrade to C++20 for validation)");

#define VALIDATE_UPDATE_ERROR_FLAG_PATTERN(ClassName) \
    static_assert(true, #ClassName " UpdateErrorFlag requirements documented (upgrade to C++20 for validation)");
#endif

//==============================================================================
// COMPILE-TIME VALIDATION FUNCTIONS
//==============================================================================

/**
 * @brief Compile-time validation function for any VQwDataElement derivative
 * 
 * This function will trigger concept evaluation and provide detailed error
 * messages if the architectural requirements are not met.
 * 
 * For C++17: Always returns true but documents expected patterns
 * For C++20: Performs full validation
 */
template<typename T>
constexpr bool validate_architectural_compliance() {
#if QW_CONCEPTS_AVAILABLE
    if constexpr (std::is_base_of_v<VQwDataElement, T> && !std::is_abstract_v<T>) {
        static_assert(ValidVQwDataElementDerivative<T>, 
            "Class must implement Dual-Operator Pattern for VQwDataElement derivatives");
        
        if constexpr (std::is_base_of_v<VQwHardwareChannel, T>) {
            static_assert(ValidVQwHardwareChannelDerivative<T>,
                "VQwHardwareChannel derivatives must implement additional requirements");
        }
        
        return true;
    }
    
    // For container classes (not derived from VQwDataElement)
    if constexpr (!std::is_base_of_v<VQwDataElement, T>) {
        // Check if it looks like a container class
        if constexpr (requires(T t, const T& other) { t += other; }) {
            static_assert(ValidContainerClass<T>,
                "Container classes must implement Container-Delegation Pattern");
        }
        return true;
    }
#endif // QW_CONCEPTS_AVAILABLE
    
    return true;
}

/**
 * @brief Information function to check if concepts are active
 */
constexpr bool concepts_available() {
    return QW_CONCEPTS_AVAILABLE;
}

/**
 * @brief Get a string describing the current validation mode
 */
constexpr const char* validation_mode() {
#if QW_CONCEPTS_AVAILABLE
    return "C++20 concepts: Full architectural validation active";
#else
    return "C++17 compatibility: Architectural validation disabled (patterns documented only)";
#endif
}

} // namespace QwArchitecture

#endif // __QWCONCEPTS__