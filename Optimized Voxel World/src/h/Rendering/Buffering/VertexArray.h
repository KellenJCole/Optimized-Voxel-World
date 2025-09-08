#pragma once

class VertexBuffer;
class VertexBufferLayout;
class VertexArray {
public:
	VertexArray();
	void create();
	~VertexArray();
	unsigned int getId() { return vertexArray_id; }

	void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout);
	void AddBuffer(const unsigned int vb, const VertexBufferLayout& layout);

	void AddBufferAt(const VertexBuffer& vb, const VertexBufferLayout& layout, unsigned int baseAttrib);
	void AddBufferInstancedAt(const VertexBuffer& vb, const VertexBufferLayout& layout,
		unsigned int baseAttrib, unsigned int divisor = 1);

	void Bind() const;
	void Unbind() const;

	void destroy();
private:
	unsigned int vertexArray_id;
};