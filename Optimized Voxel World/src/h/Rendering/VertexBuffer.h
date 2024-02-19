#pragma once

class VertexBuffer {
private:
	unsigned int vertexBuffer_id;
public:
	VertexBuffer();
	void create(const void* data, unsigned int size);
	void update(const void* data, unsigned int size);
	void destroy();
	~VertexBuffer();

	void Bind() const;
	void Unbind() const;
};
