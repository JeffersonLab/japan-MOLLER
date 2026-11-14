#pragma once
#include <iostream>
#include <cadef.h>
#include <vector>
#include "QwErrorHandling.h"
#include "QwEpicChannel.h"

/**
 * @class EpicHandler
 * @brief EpicChannel context manager. Implemented as a Meyers Singleton
 *        as all EpicChannels need to be created and destroyed during the
 *        lifetime of this resource manager.
 */
class EpicHandler
{
	/*
	 *@var channels
	 *	Vector of unique_ptrs. Use of callbacks requires persistant memory locations
     *  Could be implemented using a std::list instead of vector<unique_ptr>
	*/
	using ChanList = std::vector<std::unique_ptr<EpicChannel>>;
	ChanList channels;
	EpicHandler();
	~EpicHandler();
	EpicHandler(EpicHandler const&) = delete;
	EpicHandler& operator=(EpicHandler const&) = delete;
	EpicHandler(EpicHandler &&) noexcept = delete;
	EpicHandler& operator=(EpicHandler &&) noexcept = delete;
public:
	/**
	 *@brief EpicHandler Instance.
	 *@details Implemented as a Meyers Singleton for
	 *         thread safe creation and destruction
	 */
	static EpicHandler& Instance();
public:
	/**
	 *@brief Searches for an existing IOC instance and
	 *       creates a new instance if not found
	 *@param [in] pv_name Process variable
	 *@param [in] priority level for EPICS dispatch
	 *@details Currently the priority level has no effect.
	 *         See description for ca_create_channel
	 *         https://docs.epics-controls.org/projects/base/en/latest/cadef_h.html
	 *@return  Non-owning observer pointer to the EpicChannel. Will never be NULL.
	 *         EpicChannel* being non-null does not mean it has connected to a EPICS
	 *         channel. See EpicChannel::CheckConnection()
	 */
	EpicChannel* ConnectChannel(char const* pv_name, ::capri priority = CA_PRIORITY_DEFAULT);
	/**
	 *@brief Searches for an existing IOC instance.
	 *@param [in] target Process variable name to search for
	 *@return  Non-owning observer pointer to the EpicChannel if found,
	 *         nullptr otherwise.
	 */
	EpicChannel* FindChannel(std::string_view target);
	/**
	 *@brief Prints the EPICS client status
	 *@param [in] level Level of detail.
	 */
	void GetStatus(unsigned level=0);
	/**
	 *@brief Prints the connection status for all stored IOCs
	 */
	void CheckConnection();
};

