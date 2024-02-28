#pragma once

class IndexBuffer {
public:
	IndexBuffer();
	void create(const unsigned int* data, unsigned int count);
	void update(const unsigned int* data, unsigned int count);
	void clear();
	bool isValid();
	~IndexBuffer();

	void Bind() const;
	void Unbind() const;

	void destroy();

	unsigned int GetCount();
private:
	bool valid;
	unsigned int indexBuffer_id;
	unsigned int m_count;
};