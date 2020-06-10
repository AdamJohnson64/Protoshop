#pragma once

class Sample
{
public:
    virtual ~Sample() = default;
    virtual void Render() = 0;
};

Sample* CreateSample_ComputeCanvas();
Sample* CreateSample_DrawingContext();