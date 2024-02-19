#pragma once

class InstanceBuffer {
private:
	unsigned int instanceBuffer_id;
public:
	InstanceBuffer();
	void create(unsigned int size);
	void update(const void* data, unsigned int size);
	~InstanceBuffer();

	void Bind() const;
	void Unbind() const;
};