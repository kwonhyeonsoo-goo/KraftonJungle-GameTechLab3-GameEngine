#pragma once

class CStatWindow
{
public:
    void Render();

    void SetObjectCount(unsigned int Count) { ObjectCount = Count; }
    void SetHeapUsage(unsigned int Bytes) { HeapUsageBytes = Bytes; }

private:
    unsigned int ObjectCount = 0;
    unsigned int HeapUsageBytes = 0;
};
