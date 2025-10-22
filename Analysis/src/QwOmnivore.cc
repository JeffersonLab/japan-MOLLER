/*!
 * \file   QwOmnivore.cc
 * \brief  Implementation for omnivorous subsystem template class
 */

#include "QwOmnivore.h"

// Register this subsystem with the factory
RegisterSubsystemFactory(QwOmnivore<VQwSubsystemParity>);

// Register this subsystem with the factory
// TODO (wdc) disabled due to extraneous includes
//RegisterSubsystemFactory(QwOmnivore<VQwSubsystemTracking>);
