#pragma once

class VertexBuffer {
private:
	unsigned int vertexBuffer_id;
	bool valid;
	unsigned int m_count;
public:
	VertexBuffer();
	void create(const void* data, unsigned int size);
	void update(const void* data, unsigned int size);
	void clear();
	void destroy();
	~VertexBuffer();
	bool isValid();
	void Bind() const;
	void Unbind() const;
	unsigned int GetCount();
};