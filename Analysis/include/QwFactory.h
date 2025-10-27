/*!
 * \file   QwFactory.h
 * \brief  Factory pattern implementation for creating analysis objects
 */

#pragma once

// System headers
#include <cxxabi.h>
#include <cstdlib>
#include <typeindex>

// Qweak headers
#include "QwLog.h"
#include "QwConcepts.h"

// Forward declarations
class VQwSubsystem;
class VQwDataElement;
class VQwDataHandler;

// Exceptions
struct QwException_TypeUnknown {
  QwException_TypeUnknown() { }
};


/**
 *  \class VQwFactory
 *  \ingroup QwAnalysis
 *  \brief Abstract factory base for runtime object creation
 *
 * Enables instantiation of derived types based on runtime string identifiers.
 * Maintains a registry of concrete factory instances that can create objects
 * of specified types. Used throughout the framework to support configuration-
 * driven object creation for subsystems, data handlers, and data elements.
 */
template <class base_t>
class VQwFactory {

  public:

    /// Default virtual destructor
    virtual ~VQwFactory() { }

    /// Create an object of type with name
    static base_t* Create(const std::string& type, const std::string& name) {
      return GetFactory(type)->Create(name);
    }

    /// Dynamic cast of object into type
    static base_t* Cast(base_t* base, const std::string& type) {
      return GetFactory(type)->Cast(base);
    }

    /// Test whether object inherits from type
    static bool InheritsFrom(base_t* base, const std::string& type) {
      return (Cast(base,type) != 0);
    }

    /// Creation of objects by type (pure virtual)
    virtual base_t* Create(const std::string& name) const = 0;
    /// Dynamic cast to type (pure virtual)
    virtual base_t* Cast(base_t* base) const = 0;

  protected:

    /// Map from string to concrete type factories
    static std::map<std::string,VQwFactory*>& GetRegisteredTypes() {
      static std::map<std::string,VQwFactory<base_t>*> theMap;
      return theMap;
    }
    /// List available type factories
    static void ListRegisteredTypes() {
      typename std::map<std::string,VQwFactory<base_t>*>::iterator type;
      for (type = GetRegisteredTypes().begin();
           type != GetRegisteredTypes().end(); type++ )
        QwMessage << type->first << QwLog::endl;
    }

    /// Get a concrete type factory by string
    static VQwFactory* GetFactory(const std::string& type) {
      if (GetRegisteredTypes().find(type) != GetRegisteredTypes().end())
        return GetRegisteredTypes()[type];
      else {
        QwError << "Type " << type << " is not registered!" << QwLog::endl;
        QwMessage << "Available types:" << QwLog::endl;
        ListRegisteredTypes();
        QwWarning << "To register this type, add the following line to the top "
                  << "of the source file:" << QwLog::endl;
        QwWarning << "  REGISTER_SOMETHING_FACTORY(" << type << ");" << QwLog::endl;
        QwWarning << "Ensure that the dynamic library contains the factory object."
                  << QwLog::endl;
        throw QwException_TypeUnknown();
      }
    }

}; // class VQwFactory


/**
 *  \class QwFactory
 *  \ingroup QwAnalysis
 *  \brief Concrete templated factory for creating specific object types
 *
 * Template specialization that provides concrete object creation for a
 * specific derived type. Automatically registers itself in the factory
 * registry during static initialization. Used by RegisterSomethingFactory
 * macros to enable runtime type creation.
 */
template <class base_t, class type_t>
class QwFactory: public VQwFactory<base_t> {

  public:

    /// Constructor which stores type name in list of registered types
    QwFactory(const std::string& type) {
      VQwFactory<base_t>::GetRegisteredTypes()[type] = this;
    }

    /// Concrete type creation
    base_t* Create(const std::string& name) const override {
      return new type_t(name);
    }

    /// Dynamic cast of type
    type_t* Cast(base_t* base) const override {
      return dynamic_cast<type_t*>(base);
    }

}; // class QwFactory


/// Factory type with functionality for data handlers
typedef class VQwFactory<VQwDataHandler> VQwDataHandlerFactory;
/// Factory type with functionality for subsystems
typedef class VQwFactory<VQwSubsystem> VQwSubsystemFactory;
/// Factory type with functionality for data elements
typedef class VQwFactory<VQwDataElement> VQwDataElementFactory;


// TODO It would be nice to be able to define something like
//   template <class type_t>
//   typedef class QwFactory<VQwSubsystem,type_t> QwSubsystemFactory;
// to be able to write e.g.
//   QwSubsystemFactory<QwLumi>
//
// This seems to be functionality that is in C++0x...
//   typedef <class type_t> using QwSubsystemFactory = QwFactory<VQwSubsystem,type_t>;
//
// For now the following will do as well, but adds a layer of inheritance
template <class handler_t>
class QwHandlerFactory: public QwFactory<VQwDataHandler,handler_t> { };
template <class subsystem_t>
class QwSubsystemFactory: public QwFactory<VQwSubsystem,subsystem_t> { };
template <class dataelement_t>
class QwDataElementFactory: public QwFactory<VQwDataElement,dataelement_t> { };



/**
 * \class VQwCloneable
 * \ingroup QwAnalysis
 * \brief Virtual base providing polymorphic copy construction
 *
 * Template base class that enables runtime cloning of derived objects
 * through a common interface. Part of the factory pattern implementation.
 */
template <class base_t>
class VQwCloneable {

  public:

    /// Virtual destructor
    virtual ~VQwCloneable() { }

    /// Get demangled name of this class
    std::string GetClassName() const {
      int status;
      const std::type_index ti = typeid(*this);
      char* name = abi::__cxa_demangle(ti.name(), 0, 0, &status);
      std::string str(name);
      free(name);
      return str;
    }

    /// Abstract clone method when no derived method is defined
    virtual base_t* Clone() const {
      QwError << "Clone() is not implemented for class " << GetClassName() << "!" << QwLog::endl;
      QwMessage << "Modify the class definition of " << GetClassName() << " to:" << QwLog::endl;
      QwMessage << "  class " << GetClassName() << ": "
                << "public MQwSomethingCloneable<" << GetClassName() << ">" << QwLog::endl;
      return 0;
    }

    /// Virtual factory getter
    virtual const VQwFactory<base_t>* Factory() const { return 0; }

}; // class VQwCloneable


/**
 * \class MQwCloneable
 * \ingroup QwAnalysis
 * \brief Mix-in template for concrete cloneable types
 *
 * Implements the curiously recurring template pattern to provide
 * concrete clone functionality for specific derived types.
 * Enables factory-based object creation and copying.
 */
template <class base_t, class type_t>
class MQwCloneable: virtual public VQwCloneable<base_t> {

  public:

    /// Virtual destructor
    ~MQwCloneable() override { };

    /// Concrete clone method
    base_t* Clone() const override {
      return new type_t(static_cast<const type_t&>(*this));
    }

    /// Factory getter
    const VQwFactory<base_t>* Factory() const override { return fFactory; }

    /// Object creation
    static base_t* Create(const std::string& name) {
      if (fFactory) return fFactory->Create(name);
      else return 0;
    }

    /// Object dynamic cast
    static type_t* Cast(type_t* type) {
      if (fFactory) return fFactory->Cast(type);
      else return 0;
    }

  private:

    /// Pointer to the factory of this type
    static const VQwFactory<base_t>* fFactory;

}; // class MQwCloneable

/// Mix-in factory functionality for datahandlers
typedef class VQwCloneable<VQwDataHandler> VQwDataHandlerCloneable;
template <class subsystem_t>
class MQwDataHandlerCloneable: public MQwCloneable<VQwDataHandler,subsystem_t> { };
/// Mix-in factory functionality for subsystems
typedef class VQwCloneable<VQwSubsystem> VQwSubsystemCloneable;
template <class subsystem_t>
class MQwSubsystemCloneable: public MQwCloneable<VQwSubsystem,subsystem_t> { };
/// Mix-in factory functionality for data elements
typedef class VQwCloneable<VQwDataElement> VQwDataElementCloneable;
template <class dataelement_t>
class MQwDataElementCloneable: public MQwCloneable<VQwDataElement,dataelement_t> { };


/// Macros to create and register the data handler factory of type A
/// Note: a call to this macro should be followed by a semi-colon!
/// Includes automatic architectural validation for VQwDataHandler derivatives
#define REGISTER_DATA_HANDLER_FACTORY(A) \
  VALIDATE_DATA_HANDLER_PATTERN(A); \
  template<> inline const VQwDataHandlerFactory* MQwCloneable<VQwDataHandler,A>::fFactory = new QwFactory<VQwDataHandler,A>(#A)

/// Macros to create and register the subsystem factory of type A
/// Note: a call to this macro should be followed by a semi-colon!
/// Includes automatic architectural validation for VQwSubsystem derivatives
#define REGISTER_SUBSYSTEM_FACTORY(A) \
  VALIDATE_SUBSYSTEM_PATTERN(A); \
  template<> inline const VQwSubsystemFactory* MQwCloneable<VQwSubsystem,A>::fFactory = new QwFactory<VQwSubsystem,A>(#A)

/// Macros to create and register the data element factory of type A
/// Note: a call to this macro should be followed by a semi-colon!
/// Includes automatic architectural validation for VQwDataElement derivatives
#define REGISTER_DATA_ELEMENT_FACTORY(A) \
  VALIDATE_DATA_ELEMENT_PATTERN(A); \
  template<> inline const VQwDataElementFactory* MQwCloneable<VQwDataElement,A>::fFactory = new QwFactory<VQwDataElement,A>(#A)
