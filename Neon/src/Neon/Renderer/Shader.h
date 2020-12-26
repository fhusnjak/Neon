#pragma once

#include "glm/glm.hpp"

namespace Neon
{
	enum class UniformType
	{
		None = 0,
		Int32,
		Uint32,
		Float,
		Float2,
		Float3,
		Float4,
		Matrix3x3,
		Matrix4x4
	};

	struct UniformDecl
	{
		UniformType Type;
		std::ptrdiff_t Offset;
		std::string Name;
	};

	enum class ShaderUniformType
	{
		None = 0,
		Bool,
		Int,
		Float,
		Vec2,
		Vec3,
		Vec4,
		Mat3,
		Mat4
	};

	class Shader : public RefCounted
	{
	public:
		virtual void Reload() = 0;

		virtual void Bind() = 0;

		virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) = 0;
		virtual void SetUniform(const std::string& fullname, float value) = 0;
		virtual void SetUniform(const std::string& fullname, int value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec2& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec3& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec4& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::mat3& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::mat4& value) = 0;
	};
}
