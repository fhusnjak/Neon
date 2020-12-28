#pragma once

#include "Vulkan.h"
#include "VulkanAllocator.h"
#include "VulkanDevice.h"

struct GLFWwindow;

namespace Neon
{
	class VulkanSwapChain
	{
	public:
		VulkanSwapChain() = default;
		~VulkanSwapChain();

		void Init(vk::Instance instance, SharedRef<VulkanDevice>& device);
		void InitSurface(GLFWwindow* windowHandle);
		void Create(uint32* width, uint32* height, bool vsync = false);

		void OnResize(uint32 width, uint32 height);

		void BeginFrame();
		void Present();

		vk::SwapchainKHR GetHandle() const
		{
			return m_Handle;
		}

		uint32 GetImageCount() const
		{
			return static_cast<uint32>(m_Buffers.size());
		}

		vk::Extent2D GetExtent() const
		{
			return {m_Width, m_Height};
		}

		uint32 GetWidth() const
		{
			return m_Width;
		}
		uint32 GetHeight() const
		{
			return m_Height;
		}

		vk::RenderPass GetRenderPass() const
		{
			return m_RenderPass.get();
		}

		vk::Framebuffer GetCurrentFramebuffer() const
		{
			return GetFramebuffer(m_CurrentBufferIndex);
		}
		vk::CommandBuffer GetCurrentDrawCommandBuffer() const
		{
			return GetRenderCommandBuffer(m_CurrentBufferIndex);
		}

		uint32 GetCurrentBufferIndex() const
		{
			return m_CurrentBufferIndex;
		}
		vk::Framebuffer GetFramebuffer(uint32 index) const
		{
			NEO_CORE_ASSERT(index < m_Framebuffers.size(), "Framebuffer index out of range");
			return m_Framebuffers[index].get();
		}
		vk::CommandBuffer GetRenderCommandBuffer(uint32 index) const
		{
			NEO_CORE_ASSERT(index < m_RenderCommandBuffers.size(), "Commandbuffer index out of range");
			return m_RenderCommandBuffers[index].get();
		}

		vk::Format GetColorFormat() const
		{
			return m_ColorFormat;
		}

	private:
		vk::Result AcquireNextImage(vk::Semaphore presentCompleteSemaphore, uint32* imageIndex);
		vk::Result QueuePresent(vk::Queue queue, uint32 imageIndex, vk::Semaphore waitSemaphore = vk::Semaphore{});

		void CreateFramebuffers();
		void CreateDepthStencil();
		void CreateRenderCommandBuffers();
		void FindSurfaceFormatAndColorSpace();

	private:
		vk::Instance m_Instance;
		SharedRef<VulkanDevice> m_Device;
		vk::UniqueSurfaceKHR m_Surface;
		VulkanAllocator m_Allocator;

		vk::Format m_ColorFormat;
		vk::ColorSpaceKHR m_ColorSpace;

		vk::SwapchainKHR m_Handle;
		struct SwapChainBuffer
		{
			vk::Image Image;
			vk::UniqueImageView View;
		};
		std::vector<SwapChainBuffer> m_Buffers;

		vk::Format m_DepthFormat;
		struct
		{
			vk::UniqueImage Image;
			vk::UniqueDeviceMemory DeviceMemory;
			vk::UniqueImageView ImageView;
		} m_DepthStencil;

		std::vector<vk::UniqueFramebuffer> m_Framebuffers;
		vk::UniqueCommandPool m_CommandPool;
		std::vector<vk::UniqueCommandBuffer> m_RenderCommandBuffers;

		struct Semaphores
		{
			vk::UniqueSemaphore ImageAcquired;
			vk::UniqueSemaphore RenderComplete;
		};
		std::vector<Semaphores> m_Semaphores;

		std::vector<vk::UniqueFence> m_WaitFences;

		vk::UniqueRenderPass m_RenderPass;

		uint32_t m_CurrentBufferIndex = 0;

		uint32 m_QueueNodeIndex = UINT32_MAX;
		uint32 m_Width, m_Height;

		uint32 m_CurrentFrame = 0;

		friend class VulkanContext;
	};
} // namespace Neon