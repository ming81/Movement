#pragma once

#include <stdio.h>

class OutLogger
{
public:

    explicit OutLogger();
    ~OutLogger();

    void write(const char* str, ...);

private:
    FILE* file;
};

static OutLogger sLog;