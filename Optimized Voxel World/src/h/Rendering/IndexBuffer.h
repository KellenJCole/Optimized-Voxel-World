#pragma once

class IndexBuffer {
public:
	IndexBuffer();
	void create(const unsigned int* data, unsigned int count);
	~IndexBuffer();

	void Bind() const;
	void Unbind() const;

	void destroy();

	inline unsigned int GetCount() const { return m_count; }
private:
	unsigned int indexBuffer_id;
	unsigned int m_count;
};