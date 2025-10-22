#pragma once

// System headers
#include <map>

// ROOT headers
#include "Rtypes.h"

// Qweak headers
#include "VQwHardwareChannel.h"

/**
 * \class MQwPublishable_child
 * \ingroup QwAnalysis
 * \brief Mix-in for objects that can publish/request variables via a parent container
 *
 * Enables subsystems or data handlers to request external variables from
 * sibling objects via a parent container, and to publish their own internal
 * variables for external access. Part of the variable publishing framework.
 */
template<class U, class T>
class MQwPublishable_child {

  public:

  /**
   * \brief Default constructor
   * Initializes the child object and sets up self-reference for publishing.
   */
  MQwPublishable_child() {fSelf = dynamic_cast<T*>(this); };
  
  /**
   * \brief Copy constructor
   * @param source Source object to copy from
   */
  MQwPublishable_child(const MQwPublishable_child& source) {fSelf = dynamic_cast<T*>(this);};

  /** \brief Virtual destructor */
  virtual ~MQwPublishable_child() { };
    
    /**
     * \brief Set the parent container for this child object
     * @param parent Pointer to the parent container that manages variable publishing
     */
    void SetParent(U* parent){fParent = parent;};
    
    /**
     * \brief Get the parent container for this child object
     * @return Pointer to the parent container, or nullptr if no parent is set
     */
    U* GetParent() const {return fParent;};


 protected:
    /**
    * \brief Retrieve the variable name from other subsystem arrays
    * Get the value corresponding to some variable name from a different
    * data array.
    * @param name Name of the desired variable
    * @param value Pointer to the value to be filled by the call
    * @return True if the variable was found, false if not found
    */
    Bool_t RequestExternalValue(const TString& name, VQwHardwareChannel* value) const;
    /**
     * \brief Retrieve a pointer to an external variable by name
     * Requests a direct pointer to a variable from sibling subsystems via the parent container.
     * @param name Name of the desired variable
     * @return Pointer to the variable's data element, or nullptr if not found
     */
    const VQwHardwareChannel* RequestExternalPointer(const TString& name) const;
    /**
      * \brief Publish a variable from this child into the parent container.
      * @param name    Variable key to publish under.
      * @param desc    Human-readable description of the variable.
      * @param element Pointer to the data element representing this variable.
      * @return kTRUE if the variable was published; kFALSE on duplicate key or no parent.
      */
    Bool_t PublishInternalValue(const TString name, const TString desc, const VQwHardwareChannel* element) const;


    /// The functions below should be specified in the fully derived classes.
 public:
    /**
     * \brief Publish all variables of the subsystem
     * Called to register all internal variables that this subsystem wants to make
     * available to other subsystems via the publishing framework.
     * @return kTRUE if all variables were successfully published, kFALSE otherwise
     */
    virtual Bool_t PublishInternalValues() const = 0;
    
    /**
     * \brief Try to publish an internal variable matching the submitted name
     * Called when another subsystem requests a variable that hasn't been published yet.
     * Allows for lazy/on-demand publishing of variables.
     * @param device_name Name of the variable being requested
     * @return kTRUE if the variable was found and published, kFALSE otherwise
     */
    virtual Bool_t PublishByRequest(TString device_name) = 0;

 private:
    U* fParent;
    T* fSelf;
};


/**
 * \class MQwPublishable
 * \ingroup QwAnalysis
 * \brief Mix-in for container classes that manage variable publishing
 *
 * Provides the container-side logic for the variable publishing system,
 * including registering published variables, handling external requests,
 * and maintaining mappings between variable names and data elements.
 */
template<class U, class T>
class MQwPublishable {

  public:

    /**
     * \brief Default constructor
     * Initializes empty variable publishing maps.
     */
    MQwPublishable() { };
    
    /**
     * \brief Copy constructor
     * Creates a new container with cleared publishing maps (variables are not copied).
     * @param source Source object to copy from (maps are cleared, not copied)
     */
    MQwPublishable(const MQwPublishable& source) {
      fPublishedValuesDataElement.clear();
      fPublishedValuesSubsystem.clear();
      fPublishedValuesDescription.clear();
    }

    /** \brief Virtual destructor */
    virtual ~MQwPublishable() { };

  public:
    std::vector<std::vector<TString> > fPublishList;
    
    /**
     * \brief Retrieve a variable value from external sources by copying
     * Searches for the named variable in external subsystem arrays and copies
     * its value into the provided data element.
     * @param name Name of the variable to retrieve
     * @param value Pointer to data element that will receive the variable's value
     * @return kTRUE if variable was found and copied, kFALSE otherwise
     */
    Bool_t RequestExternalValue(const TString& name, VQwHardwareChannel* value) const;

    /**
     * \brief Retrieve a direct pointer to an external variable
     * Searches for the named variable in external subsystem arrays and returns
     * a direct pointer to the data element.
     * @param name Name of the variable to retrieve
     * @return Pointer to the variable's data element, or nullptr if not found
     */
    const VQwHardwareChannel* RequestExternalPointer(const TString& name) const;
    
    /**
     * \brief Retrieve an internal variable by name (pointer version)
     * Searches for the named variable among published internal variables and
     * returns a direct pointer to the data element.
     * @param name Name of the variable to retrieve
     * @return Pointer to the variable's data element, or nullptr if not found
     */
    virtual const VQwHardwareChannel* ReturnInternalValue(const TString& name) const;

    /**
     * \brief Retrieve an internal variable by name (copy version)
     * Searches for the named variable among published internal variables and
     * copies its value into the provided data element.
     * @param name Name of the variable to retrieve
     * @param value Pointer to data element that will receive the variable's value
     * @return kTRUE if variable was found and copied, kFALSE otherwise
     */
    Bool_t ReturnInternalValue(const TString& name, VQwHardwareChannel* value) const;

    /**
     * \brief List all published variables with descriptions
     * Prints a summary of all currently published variables and their descriptions
     * to the logging output for debugging and inspection purposes.
     */
    void ListPublishedValues() const;

    /**
     * \brief Publish an internal variable from a subsystem
     * Registers a variable from one of the contained subsystems in the publishing
     * framework, making it available for external access by name.
     * @param name Unique name/key for the variable
     * @param desc Human-readable description of the variable
     * @param subsys Pointer to the subsystem that owns this variable
     * @param element Pointer to the data element representing this variable
     * @return kTRUE if variable was successfully published, kFALSE if name already exists
     */
    Bool_t PublishInternalValue(
        const TString name,
        const TString desc,
        const T* subsys,
        const VQwHardwareChannel* element);

  private:
    /**
     * \brief Try to publish an internal variable on demand
     * Called internally when a variable is requested but not yet published.
     * Iterates through contained subsystems to find and publish the requested variable.
     * @param device_name Name of the variable being requested
     * @return kTRUE if the variable was found and published, kFALSE otherwise
     */
    virtual Bool_t PublishByRequest(TString device_name);

    /// Published values
    std::map<TString, const VQwHardwareChannel*> fPublishedValuesDataElement;
    std::map<TString, const T*>                  fPublishedValuesSubsystem;
    std::map<TString, TString>                   fPublishedValuesDescription;
 
};
