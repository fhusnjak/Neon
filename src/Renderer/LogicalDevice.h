//
// Created by Filip on 4.8.2020..
//

#ifndef NEON_LOGICALDEVICE_H
#define NEON_LOGICALDEVICE_H

#include <thread>
#include "PhysicalDevice.h"

class LogicalDevice {

public:
	explicit LogicalDevice(const PhysicalDevice& physicalDevice);
	[[nodiscard]] const vk::Device& GetHandle() const
	{
		return m_Handle;
	}
	[[nodiscard]] vk::Queue GetGraphicsQueue() const
	{
		return m_GraphicsQueue;
	}
	[[nodiscard]] vk::Queue GetPresentQueue() const
	{
		return m_PresentQueue;
	}
	[[nodiscard]] vk::Queue GetComputeQueue() const
	{
		return m_ComputeQueue;
	}
	[[nodiscard]] vk::Queue GetTransferQueue() const
	{
		return m_TransferQueue;
	}
private:
	vk::Device m_Handle;
	vk::Queue m_GraphicsQueue;
	vk::Queue m_PresentQueue;
	vk::Queue m_ComputeQueue;
	vk::Queue m_TransferQueue;
};

#endif //NEON_LOGICALDEVICE_H
